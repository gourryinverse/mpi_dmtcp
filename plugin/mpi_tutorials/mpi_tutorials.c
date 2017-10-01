#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <mpi.h>
#include "mpi_proxy.h"
#include "dmtcp.h"

int sockfd;
int replay_commands[1024];
int replay_count = 0;
int proxy_started = 0;

void mpi_proxy_wait_for_instructions();


int serial_printf(char * msg)
{
  printf("%s", msg);
  fflush(stdout);
}

int exec_proxy_cmd(int pcmd)
{
  int answer = 0;
  DMTCP_PLUGIN_DISABLE_CKPT();
  serial_printf("PLUGIN: Sending Proxy Command\n");
  printf("Socket: %08x\n", sockfd);
  fflush(stdout);
  write(sockfd, &pcmd, 4);
  serial_printf("PLUGIN: Receiving Proxy Answer - ");
  if (read(sockfd, &answer, 4) < 0)
    serial_printf("****** ERROR READING FROM SOCKET *****\n");
  else
    serial_printf("Answer Received\n");
  DMTCP_PLUGIN_ENABLE_CKPT();
  return answer;
}


// Proxy Setup
void init_proxy()
{
  int i = 0;
  struct sockaddr_in serv_addr;
  
  if (proxy_started)
    return;
  
  serial_printf("PLUGIN: Initialize Proxy Connection\n");

  memset(&serv_addr, '0', sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(31337);
  inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    serial_printf("SOCKET FAILURE\n");
  }
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    serial_printf("CONNECT FAILURE\n");
  }

  for (i = 0; i < replay_count; i++)
  {
    printf("PLUGIN: Replaying command %08x\n", replay_commands[i]);
    exec_proxy_cmd(replay_commands[i]);
    i++;
  }
  proxy_started = 1;
  return;
}

void close_proxy(void)
{
  if (!proxy_started)
    return;
  serial_printf("PLUGIN: Close Proxy Connection\n");
  exec_proxy_cmd(MPIProxy_Cmd_Shutdown_Proxy);
  proxy_started = 0;
  close(sockfd);
}

void add_replay_command(int pcmd)
{
  replay_commands[replay_count] = pcmd;
  replay_count++;
}

/* hello-world */
int MPI_Init(int *argc, char ***argv)
{
  add_replay_command(MPIProxy_Cmd_Init);
  serial_printf("PLUGIN: MPI_Init!\n");
  return exec_proxy_cmd(MPIProxy_Cmd_Init);
}

int MPI_Finalize(void)
{
  add_replay_command(MPIProxy_Cmd_Finalize);
  serial_printf("PLUGIN: MPI_Finalize\n");
  return exec_proxy_cmd(MPIProxy_Cmd_Finalize);
}

void dmtcp_event_hook(DmtcpEvent_t event, DmtcpEventData_t *data)
{
  /* NOTE:  See warning in plugin/README about calls to printf here. */
  switch (event) {
  case DMTCP_EVENT_INIT:
    serial_printf("*** DMTCP_EVENT_INIT\n");
    init_proxy();
    break;
  case DMTCP_EVENT_RESUME:
    serial_printf("*** DMTCP_EVENT_RESUME\n");
    init_proxy();
    break;
  case DMTCP_EVENT_RESTART:
    serial_printf("*** DMTCP_EVENT_RESTART\n");
    init_proxy();
    break;
  case DMTCP_EVENT_THREADS_SUSPEND:
    serial_printf("*** DMTCP_EVENT_THREADS_SUSPEND\n");
    close_proxy();
    break;
  case DMTCP_EVENT_WRITE_CKPT:
    serial_printf("*** DMTCP_EVENT_WRITE_CKPT\n");
    break;
  case DMTCP_EVENT_EXIT:
    serial_printf("*** DMTCP_EVENT_EXIT\n");
    close_proxy();
    break;
  default:
    break;
  }

  /* Call this next line in order to pass DMTCP events to later plugins. */
  DMTCP_NEXT_EVENT_HOOK(event, data);
}
