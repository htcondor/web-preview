/* Worker-nQueens.C */

#include "Worker-nQueens.h"
#include "Task-nQueens.h"

Worker_nQueens::Worker_nQueens() 
{
	// do this here so that workingtask has type Task_Fib, and not
	// MWTask type.  This class holds info for the task at hand...
	workingTask = new Task_nQueens;
	N = -1;
}

Worker_nQueens::~Worker_nQueens() 
{
	delete workingTask;
}

MWReturn Worker_nQueens::unpack_init_data( void ) 
{
	RMC->unpack ( &N, 1 );
	RMC->unpack ( &partition_factor, 1 );

	return OK;
}

void Worker_nQueens::execute_task( MWTask *t ) 
{
	int permutation[N];
	int i;
	int givenPerm = partition_factor >= N ? 0 : N - partition_factor;
	int index;

	Task_nQueens *tf = (Task_nQueens *) t;
	index = tf->num;

	for ( i = 0; i < givenPerm; i++ )
	{
		permutation[i] = index % N;
		index = index / N;
	}

	tf->results = search_subspace ( permutation, N, givenPerm );
}

MWWorker*
gimme_a_worker ()
{
    return new Worker_nQueens;
}

int *
Worker_nQueens::search_subspace ( int *permutation, int N, int givenPerm )
{
	int *result;

	if ( givenPerm == N )
		return isCorrectPermutation ( permutation, N );

	for ( int i = 0; i < N; i++ )
	{
		permutation[givenPerm] = i;
		result = search_subspace ( permutation, N, givenPerm + 1 );
		if ( result ) return result;
	}

	return NULL;
}

int *
Worker_nQueens::isCorrectPermutation ( int *permutation, int N )
{
	int *res;
	int i, j;

	for ( i = 0; i < N; i++ )
	{
		for ( j = 0; j < 0; j++ )
		{
			if ( i == j ) continue;

			if ( permutation[i] == permutation[j] )
				return NULL;

			if ( permutation[i] + ( j - i ) == permutation[j] )
				return NULL;

		}
	}

	res = new int[N];
	for ( i = 0; i < N; i++ )
	{
		res[i] = permutation[i];
	}
	return res;
}
