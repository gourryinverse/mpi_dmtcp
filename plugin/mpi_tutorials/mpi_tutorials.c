#include <stdio.h>
#include <mpich/mpi.h>
#include "dmtcp.h"

/* hello-world */
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

/* mpi-send-and-receive: send_recv.c, ping_pong.c, ring.c */
// MPI_Abort
// MPI_Send
// MPI_Recv

/* dynamic-receiving-with-mpi-prob-and-mpi-status: probe.c, check_status.c */
/* point-to-point-communication-application-random-walk */
// Structures: 
//		MPI_Status
// MPI_Probe
// MPI_Get_count
// MPI_Barrier		(check_status.c only)

/* mpi-broadcast-and-collective-communication: my_bcast, compare_bcast.c */
// nothing new needed for my_bcast.c
// MPI_Bcast

/* mpi-scatter-gather-and-allgather: avg.c, */
// MPI_Scatter
// MPI_Gather
// MPI_Allgather

/* performing-parallel-rank-with-mpi: random_rank.c, tmpi_rank.c */
// MPI_Type_size

/* mpi-reduce-and-allreduce: reduce_avg.c, reduce_stddev.c */
// MPI_Reduce
// MPI_Allreduce

/* introduction-to-groups-and-communicators: split.c, groups.c  */
// Structures:
// 		MPI_Group
// MPI_Comm_group
// MPI_Group_incl
// MPI_Comm_create_group
// MPI_Group_free
// MPI_Comm_free



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
