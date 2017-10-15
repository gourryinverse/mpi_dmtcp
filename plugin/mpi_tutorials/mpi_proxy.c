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
#include <sys/types.h>
#include "mpi_proxy.h"
#include <mpi.h>

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
  // TODO get instruction
  // TODO Communication mechanism
  // TODO dispatch
  int init = 0;
  int cmd = 0;
  while (1)
  {
    cmd = 0;
    read(connfd, &cmd, 4);
    fflush(stdout);
    switch(cmd)
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
      serial_printf("PROXY: Unknown Command. Exiting.\n");
      goto DONE;
      break;
    }
  }
DONE:
  return;
}

void fork_and_call(int argc, char *argv[])
{
  int pid = fork();
  int i = 0;
  if (pid == 0)
  {
    // child:
    if (!strcmp(argv[1], "dmtcp_launch"))
    {
      // go ahead and exec into provided arglist
      execvp(argv[1], &argv[1]);
    }
    else if (!strcmp(argv[1], "dmtcp_restart"))
    {
      //execvp("dmtcp_restart", &argv[2]);
    }
    else
    {
      printf("ERROR - NOT A LAUNCH OR RESUME\n");
    }
    exit(1);
  }
  exit(0);
  return;
}

int main(int argc, char *argv[])
{
  int connfd = 0;
  struct sockaddr_in serv_addr;

  memset(&serv_addr, '0', sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(31337);
  
  if ((listfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("listen failed\n");
    exit(0);
  }
  int reuse = 1;
  setsockopt(listfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
  
  if (bind(listfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("bind failed\n");
    close(listfd);
    exit(0);
  }
  
  serial_printf("listening\n");
  listen(listfd, 1);

  // now that the socket is ready, go ahead and fork
  fork_and_call(argc, argv);
  while (1)
  {
    pid_t pid;
    connfd = accept(listfd, (struct sockaddr*)NULL, NULL); 
    
    pid = fork();
    if (pid == 0)
    {
      proxy(connfd);
      close(connfd);
      exit(0);
    }
  }
  close(listfd);
  return 0;
}


