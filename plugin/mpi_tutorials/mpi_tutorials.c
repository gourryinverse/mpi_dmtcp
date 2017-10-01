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
  serial_printf("PLUGIN: Sending Proxy Command\n");
  write(sockfd, &pcmd, 4);
  serial_printf("PLUGIN: Receiving Proxy Answer - ");
  if (read(sockfd, &answer, 4) < 0)
    serial_printf("****** ERROR READING FROM SOCKET *****\n");
  else
    serial_printf("Answer Received\n");
  return answer;
}


// Proxy Setup
void init_proxy()
{
  int i = 0;
  struct sockaddr_in serv_addr;
  serial_printf("PLUGIN: Initialize Proxy Connection\n");

  memset(&serv_addr, '0', sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(31337);
  inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
  sockfd = NEXT_FNC(socket)(AF_INET, SOCK_STREAM, 0);
  NEXT_FNC(connect)(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

  for (i = 0; i < replay_count; i++)
  {
    printf("PLUGIN: Replaying command %08x\n", replay_commands[i]);
    exec_proxy_cmd(replay_commands[i]);
    i++;
  }

  return;
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
  case DMTCP_EVENT_WRITE_CKPT:
    serial_printf("*** DMTCP_EVENT_WRITE_CKPT\n");
    exec_proxy_cmd(MPIProxy_Cmd_Shutdown_Proxy);
    close(sockfd);
    break;
  case DMTCP_EVENT_RESTART:
    serial_printf("*** DMTCP_EVENT_RESTART\n");
    init_proxy();
    break;
  case DMTCP_EVENT_RESUME:
    serial_printf("*** DMTCP_EVENT_RESUME\n");
    init_proxy();
    break;
  case DMTCP_EVENT_EXIT:
    serial_printf("*** DMTCP_EVENT_EXIT\n");
    serial_printf("PLUGIN: Close Proxy Connection\n");
    exec_proxy_cmd(MPIProxy_Cmd_Shutdown_Proxy);
    NEXT_FNC(close)(sockfd);
    break;

  case DMTCP_EVENT_PRE_SUSPEND_USER_THREAD:
    //serial_printf("*** DMTCP_EVENT_PRE_SUSPEND_USER_THREAD\n");
    break;
  case DMTCP_EVENT_DRAIN:
    //serial_printf("*** DMTCP_EVENT_DRAIN\n");
    break;
  case DMTCP_EVENT_PRE_EXEC:
    //printf("*** DMTCP_EVENT_PRE_EXEC\n");
    break;
  case DMTCP_EVENT_POST_EXEC:
    //printf("*** DMTCP_EVENT_POST_EXEC\n");
    break;
  case DMTCP_EVENT_ATFORK_PREPARE:
    //printf("*** DMTCP_EVENT_ATFORK_PREPARE\n");
    break;
  case DMTCP_EVENT_ATFORK_PARENT:
    //printf("*** DMTCP_EVENT_ATFORK_PARENT\n");
    break;
  case DMTCP_EVENT_ATFORK_CHILD:
    //printf("*** DMTCP_EVENT_ATFORK_CHILD\n");
    break;
  case DMTCP_EVENT_WAIT_FOR_SUSPEND_MSG:
    //printf("*** DMTCP_EVENT_WAIT_FOR_SUSPEND_MSG\n");
    break;
  case DMTCP_EVENT_THREADS_SUSPEND:
    //printf("*** DMTCP_EVENT_THREADS_SUSPEND\n");
    break;
  case DMTCP_EVENT_LEADER_ELECTION:
    //printf("*** DMTCP_EVENT_LEADER_ELECTION\n");
    break;
  case DMTCP_EVENT_REGISTER_NAME_SERVICE_DATA:
    //printf("*** DMTCP_EVENT_REGISTER_NAME_SERVICE_DATA\n");
    break;
  case DMTCP_EVENT_SEND_QUERIES:
    //printf("*** DMTCP_EVENT_SEND_QUERIES\n");
    break;
  case DMTCP_EVENT_REFILL:
    //printf("*** DMTCP_EVENT_REFILL\n");
    break;
  case DMTCP_EVENT_THREADS_RESUME:
    //printf("*** DMTCP_EVENT_THREADS_RESUME\n");
    break;
  case DMTCP_EVENT_RESUME_USER_THREAD:
    //printf("*** DMTCP_EVENT_RESUME_USER_THREAD\n");
    break;
  case DMTCP_EVENT_THREAD_START:
    //printf("*** DMTCP_EVENT_THREAD_START\n");
    break;
  case DMTCP_EVENT_THREAD_CREATED:
    //printf("*** DMTCP_EVENT_THREAD_CREATED\n");
    break;
  case DMTCP_EVENT_PTHREAD_START:
    //printf("*** DMTCP_EVENT_PTHREAD_START\n");
    break;
  case DMTCP_EVENT_PTHREAD_EXIT:
    //printf("*** DMTCP_EVENT_PTHREAD_EXIT\n");
    break;
  case DMTCP_EVENT_PTHREAD_RETURN:
    //printf("*** DMTCP_EVENT_PTHREAD_RETURN\n");
    break;  

  default:
    break;
  }

  /* Call this next line in order to pass DMTCP events to later plugins. */
  DMTCP_NEXT_EVENT_HOOK(event, data);
}
