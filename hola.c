#include <mpi.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	
	int rank,size,boolean=1,data;
	MPI_Status status;
	
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Request req;

	if(rank==0)
	{
		sleep(3);
		while(1)
		{
			MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &boolean, &status);
			
			if(boolean==1)
			{
				MPI_Recv(&data,1,MPI_INT,MPI_ANY_SOURCE,MPI_ANY_TAG, MPI_COMM_WORLD,&status);
				printf("%d\n",data);
			}
		}
	}
	else
	{
		MPI_Isend(&rank,1,MPI_INT,0,0,MPI_COMM_WORLD,&req);
		rank*=10;
		MPI_Isend(&rank,1,MPI_INT,1,0,MPI_COMM_WORLD,&req);
	}
	
	MPI_Finalize();

	return 0;
}