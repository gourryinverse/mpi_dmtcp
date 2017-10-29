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

//#define SDEBUG

struct VirtualMPIInfo
{
  int world_rank;
  char * processor_name;
  int name_len;
};

struct VirtualMPIInfo *virtinfo;

int replay_commands[1024];
int replay_count = 0;
int proxy_started = 0;
bool proxy_inited = false;

void mpi_proxy_wait_for_instructions();

int serial_printf(char * msg)
{
#ifdef SDEBUG
  printf("%s", msg);
  fflush(stdout);
#endif
}

int Receive_Int_From_Proxy(int connfd)
{ 
    int retval;
    int status;
    status = read(connfd, &retval, sizeof(int));
    // TODO: error check
    return retval;
}

int Send_Int_To_Proxy(int connfd, int arg)
{
    int status = write(connfd, &arg, sizeof(int));
    // TODO: error check
    return status;
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

void setup_virtualization_info(bool islaunch)
{
  int status = 0;

  if (islaunch)
  {
    int world_rank = 0;
    virtinfo = (struct VirtualMPIInfo *)malloc(sizeof(virtinfo));
    // On launch, we need to record this info in order
    // to successfully restart
    status = MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    // TODO: Error check
    virtinfo->world_rank = world_rank;
  }
  // On restart, we just need to set our proxy up with the 
  // correct virtual values
  serial_printf("SET RANK\n");
  Send_Int_To_Proxy(PROTECTED_MPI_PROXY_FD, MPIProxy_Cmd_Set_CommRank);
  Send_Int_To_Proxy(PROTECTED_MPI_PROXY_FD, virtinfo->world_rank);
  status = Receive_Int_From_Proxy(PROTECTED_MPI_PROXY_FD);
  // TODO: error checking

  serial_printf("DONE SET\n");
}

// Proxy Setup
void restart_proxy()
{
  int i = 0;
  int retval = 0;
  
  // We're resuming from a restart, set it all up
  serial_printf("PLUGIN: Restart - Initialize Proxy Connection\n");
  // Replay any replay packets, this catchs INIT on restart
  for (i = 0; i < replay_count; i++)
  {
    printf("PLUGIN: Replaying command %08x\n", replay_commands[i]);
    exec_proxy_cmd(replay_commands[i]);
    i++;
  }
  proxy_started = 1;
  
  // Then configure the proxy with the cached virtual info.
  serial_printf("PLUGIN: Configuring Proxy\n");
  setup_virtualization_info(false);
  return;
}

void close_proxy(void)
{
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
  int retval = 0;
  add_replay_command(MPIProxy_Cmd_Init);
  serial_printf("PLUGIN: MPI_Init!\n");
  retval = exec_proxy_cmd(MPIProxy_Cmd_Init);
  proxy_started = true;
  proxy_inited = true;
  setup_virtualization_info(true);
}

int MPI_Comm_size(int group, int *world_size)
{
  int status = 0;
  Send_Int_To_Proxy(PROTECTED_MPI_PROXY_FD, MPIProxy_Cmd_Get_CommSize);
  Send_Int_To_Proxy(PROTECTED_MPI_PROXY_FD, group);
  status = Receive_Int_From_Proxy(PROTECTED_MPI_PROXY_FD);
  if (!status) // success
    *world_size = Receive_Int_From_Proxy(PROTECTED_MPI_PROXY_FD);
  return status;
}

int MPI_Comm_rank(int group, int *world_rank)
{
  int status = 0;
  Send_Int_To_Proxy(PROTECTED_MPI_PROXY_FD, 
                        MPIProxy_Cmd_Get_CommRank);
  Send_Int_To_Proxy(PROTECTED_MPI_PROXY_FD, group);
  status = Receive_Int_From_Proxy(PROTECTED_MPI_PROXY_FD);
  if (!status) // success
    *world_rank = Receive_Int_From_Proxy(PROTECTED_MPI_PROXY_FD);
  return status;
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
    break;
  case DMTCP_EVENT_RESUME:
    serial_printf("*** DMTCP_EVENT_RESUME\n");
    break;
  case DMTCP_EVENT_THREADS_RESUME:
    serial_printf("*** DMTCP_EVENT_THREADS_RESUME\n");
    if (data->resumeInfo.isRestart) {
      restart_proxy();
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
    free(virtinfo);
    close_proxy();
    break;
  case DMTCP_EVENT_PRE_EXEC:
    serial_printf("*** DMTCP_EVENT_PRE_EXEC\n");
  case DMTCP_EVENT_POST_EXEC:
    serial_printf("*** DMTCP_EVENT_POST_EXEC\n");
  case DMTCP_EVENT_ATFORK_PREPARE:
    serial_printf("*** DMTCP_EVENT_ATFORK_PREPARE\n");
  case DMTCP_EVENT_ATFORK_PARENT:
    serial_printf("*** DMTCP_EVENT_ATFORK_PARENT\n");
  case DMTCP_EVENT_ATFORK_CHILD:
    serial_printf("*** DMTCP_EVENT_ATFORK_CHILD\n");
  case DMTCP_EVENT_WAIT_FOR_SUSPEND_MSG:
    serial_printf("*** DMTCP_EVENT_WAIT_FOR_SUSPEND\n");
  case DMTCP_EVENT_LEADER_ELECTION:
    serial_printf("*** DMTCP_EVENT_LEADER_ELECTION\n");
  case DMTCP_EVENT_DRAIN:
    serial_printf("*** DMTCP_EVENT_DRAIN\n");
  case DMTCP_EVENT_REGISTER_NAME_SERVICE_DATA:
    serial_printf("*** DMTCP_EVENT_REGISTER_NAME_SERVICE_DATA\n");
  case DMTCP_EVENT_SEND_QUERIES:
    serial_printf("*** DMTCP_EVENT_SEND_QUERIES\n");
  case DMTCP_EVENT_PRE_SUSPEND_USER_THREAD:
    serial_printf("*** DMTCP_EVENT_PRE_SUSPEND_USER_THREAD\n");
  case DMTCP_EVENT_RESUME_USER_THREAD:
    serial_printf("*** DMTCP_EVENT_RESUME_USER_THREAD\n");
  case DMTCP_EVENT_THREAD_START:
    serial_printf("*** DMTCP_EVENT_THREAD_START\n");
  case DMTCP_EVENT_THREAD_CREATED:
    serial_printf("*** DMTCP_EVENT_THREAD_CREATED\n");
  case DMTCP_EVENT_PTHREAD_START:
    serial_printf("*** DMTCP_EVENT_PTHREAD_START\n");
  case DMTCP_EVENT_PTHREAD_RETURN:
    serial_printf("*** DMTCP_EVENT_PTHREAD_RETURN\n");
		//printf("**** EVENT: %d\n ***", event);
		fflush(stdout);
  default:
    break;
  }

  /* Call this next line in order to pass DMTCP events to later plugins. */
  DMTCP_NEXT_EVENT_HOOK(event, data);
}
