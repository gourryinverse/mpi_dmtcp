/* CopyLeft Gregory Price (2017) */

enum MPI_Proxy_Commands
{
  MPIProxy_ERROR = 0,
  MPIProxy_Cmd_Init = 1,
  MPIProxy_Cmd_Finalize = 2,
  MPIProxy_Cmd_Shutdown_Proxy = 0xFFFFFFFF,
};

void mpi_proxy_wait_for_instructions();
