#include <stdio.h>
#include <mpich/mpi.h>
#include "dmtcp.h"

int MPI_Init(int *argc, char ***argv)
{
  printf("DMTCP: MPI_Init!\n");
  return NEXT_FNC(MPI_Init)(argc, argv);
}

int MPI_Comm_size(MPI_Comm comm, int *size)
{
	printf("DMTCP: MPI_Comm_size\n");
	return NEXT_FNC(MPI_Comm_size)(comm, size);
}

int MPI_Comm_rank(MPI_Comm comm, int *rank)
{
	printf("DMTCP: MPI_Comm_rank\n");
	return NEXT_FNC(MPI_Comm_rank)(comm, rank);
}

int MPI_Get_processor_name(char *name, int *resultlen)
{
	printf("DMTCP: MPI_Get_processor_name\n");
	return NEXT_FNC(MPI_Get_processor_name)(name, resultlen);
}

int MPI_Finalize(void)
{
	printf("DMTCP: MPI_Finalize\n");
	return NEXT_FNC(MPI_Finalize)();
}

void dmtcp_event_hook(DmtcpEvent_t event, DmtcpEventData_t *data)
{
  /* NOTE:  See warning in plugin/README about calls to printf here. */
  switch (event) {
  case DMTCP_EVENT_WRITE_CKPT:
    printf("\n*** The plugin %s is being called before checkpointing. ***\n",
	   __FILE__);
    break;
  case DMTCP_EVENT_RESUME:
    printf("*** The plugin %s has now been checkpointed. ***\n", __FILE__);
    break;
  default:
    ;
  }

  /* Call this next line in order to pass DMTCP events to later plugins. */
  DMTCP_NEXT_EVENT_HOOK(event, data);
}
