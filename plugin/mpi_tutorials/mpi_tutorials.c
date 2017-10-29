#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <mpi.h>
#include "mpi_proxy.h"
#include "dmtcp.h"
#include "protectedfds.h"

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
  fflush(stdout);
  if (write(PROTECTED_MPI_PROXY_FD, &pcmd, 4) < 0) {
    perror("PLUGIN: read error");
    serial_printf("****** ERROR WRITING TO SOCKET *****\n");
  }
  serial_printf("PLUGIN: Receiving Proxy Answer - ");
  if (read(PROTECTED_MPI_PROXY_FD, &answer, 4) < 0) {
    perror("PLUGIN: read error");
    serial_printf("****** ERROR READING FROM SOCKET *****\n");
  }
  else {
    serial_printf("Answer Received\n");
  }
  return answer;
}


// Proxy Setup
void init_proxy(bool restart)
{
  int i = 0;
  
  if (proxy_started && !restart)
    return;
  
  serial_printf("PLUGIN: Initialize Proxy Connection\n");

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
    init_proxy(false);
    break;
  case DMTCP_EVENT_RESUME:
    serial_printf("*** DMTCP_EVENT_RESUME\n");
    break;
  case DMTCP_EVENT_THREADS_RESUME:
    serial_printf("*** DMTCP_EVENT_THREADS_RESUME\n");
    if (data->resumeInfo.isRestart) {
      init_proxy(true);
    }
    break;
  case DMTCP_EVENT_RESTART:
    serial_printf("*** DMTCP_EVENT_RESTART\n");
    break;
  case DMTCP_EVENT_THREADS_SUSPEND:
    serial_printf("*** DMTCP_EVENT_THREADS_SUSPEND\n");
    break;
  case DMTCP_EVENT_REFILL:
    serial_printf("*** DMTCP_EVENT_EVENT_REFILL\n");
		break;
  case DMTCP_EVENT_WRITE_CKPT:
    serial_printf("*** DMTCP_EVENT_WRITE_CKPT\n");
    break;
  case DMTCP_EVENT_EXIT:
    serial_printf("*** DMTCP_EVENT_EXIT\n");
    close_proxy();
    break;
  case DMTCP_EVENT_PRE_EXEC:
  case DMTCP_EVENT_POST_EXEC:
  case DMTCP_EVENT_ATFORK_PREPARE:
  case DMTCP_EVENT_ATFORK_PARENT:
  case DMTCP_EVENT_ATFORK_CHILD:
  case DMTCP_EVENT_WAIT_FOR_SUSPEND_MSG:
  case DMTCP_EVENT_LEADER_ELECTION:
  case DMTCP_EVENT_DRAIN:
  case DMTCP_EVENT_REGISTER_NAME_SERVICE_DATA:
  case DMTCP_EVENT_SEND_QUERIES:
  case DMTCP_EVENT_PRE_SUSPEND_USER_THREAD:
  case DMTCP_EVENT_RESUME_USER_THREAD:
  case DMTCP_EVENT_THREAD_START:
  case DMTCP_EVENT_THREAD_CREATED:
  case DMTCP_EVENT_PTHREAD_START:
  case DMTCP_EVENT_PTHREAD_RETURN:
		//printf("**** EVENT: %d\n ***", event);
		fflush(stdout);
  default:
    break;
  }

  /* Call this next line in order to pass DMTCP events to later plugins. */
  DMTCP_NEXT_EVENT_HOOK(event, data);
}
