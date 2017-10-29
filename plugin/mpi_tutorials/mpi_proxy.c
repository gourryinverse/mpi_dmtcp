/* CopyLeft Gregory Price (2017) */

#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/resource.h>
#include "mpi_proxy.h"
#include <mpi.h>

#include "protectedfds.h"

int listfd = 0;

int serial_printf(char * msg)
{
  printf("%s", msg);
  fflush(stdout);
}

void MPIProxy_Return_Answer(int connfd, int answer)
{
  printf("Returned %08x\n", answer);
  fflush(stdout);
  write(connfd, &answer, 4);
  return;
}

void MPIProxy_Init(int connfd)
{
  // TODO: Get argc and argv
  serial_printf("PROXY: MPI_Init - ");
  MPIProxy_Return_Answer(connfd, MPI_Init(NULL, NULL));
}

void MPIProxy_Finalize(int connfd)
{
  serial_printf("PROXY: MPI_Finalize - ");
  MPIProxy_Return_Answer(connfd, MPI_Finalize());
}


void proxy(int connfd)
{
  int init = 0;
  int cmd = 0;
  while (1)
  {
    cmd = 0;
    int rc = read(connfd, &cmd, sizeof(cmd));
    if (rc < 0) {
      perror("PROXY: read");
      continue;
    }
    switch (cmd)
    {
    case MPIProxy_Cmd_Init:
      MPIProxy_Init(connfd);
      break;
    case MPIProxy_Cmd_Finalize:
      MPIProxy_Finalize(connfd);
      break;
    case MPIProxy_Cmd_Shutdown_Proxy:
      serial_printf("PROXY: Shutdown - ");
      MPIProxy_Return_Answer(connfd, 0);
      goto DONE;
    default:
      printf("PROXY: Unknown Command: %d. Exiting.\n", cmd);
      goto DONE;
      break;
    }
  }
DONE:
  return;
}

void launch_or_restart(pid_t pid, int argc, char *argv[])
{
  int i = 0;
  if (pid == 0)
  {
    // child:
    if (strstr(argv[1], "dmtcp_launch"))
    {
      // go ahead and exec into provided arglist
      serial_printf("Starting");
      execvp(argv[1], &argv[1]);
    }
    else if (strstr(argv[1], "dmtcp_restart"))
    {
      serial_printf("Restarting");
      execvp(argv[1], &argv[1]);
    }
    else
    {
      printf("ERROR - NOT A LAUNCH OR RESUME\n");
    }
    exit(1);
  }
  return;
}

// This code is copied from src/util_init.cpp
// FIXME: Remove this when we integrate with DMTCP
static void setProtectedFdBase()
{
  struct rlimit rlim = {0};
  char protectedFdBaseBuf[64] = {0};
  uint32_t base = 0;
  // Check the max no. of FDs supported for the process
  if (getrlimit(RLIMIT_NOFILE, &rlim) < 0) {
    return;
  }
  base = rlim.rlim_cur - (PROTECTED_FD_END - PROTECTED_FD_START) - 1;
  snprintf(protectedFdBaseBuf, sizeof protectedFdBaseBuf, "%u", base);
  setenv("DMTCP_PROTECTED_FD_BASE", protectedFdBaseBuf, 1);
}

int main(int argc, char *argv[])
{
  // 0 is read
  // 1 is write
  int debugPipe[2];

  setProtectedFdBase();
  socketpair(AF_UNIX, SOCK_STREAM, 0, debugPipe);

  pid_t pid = fork();
  if (pid > 0) {
    int status;
    close(debugPipe[1]);
    proxy(debugPipe[0]);
    waitpid(pid, &status, 0);
  } else if (pid == 0) {
    close(debugPipe[0]);
    assert(dup2(debugPipe[1], PROTECTED_MPI_PROXY_FD) ==
           PROTECTED_MPI_PROXY_FD);
    close(debugPipe[1]);
    launch_or_restart(pid, argc, argv);
  } else {
    assert(0);
  }

  return 0;
}
