/* Worker-Matmul.C */

#include "Worker-Matmul.h"
#include "Task-Matmul.h"

Worker_Matmul::Worker_Matmul() 
{
	// do this here so that workingtask has type Task_Fib, and not
	// MWTask type.  This class holds info for the task at hand...
	workingTask = new Task_Matmul;
	A = B = NULL;
	num_rowsA = num_rowsB = num_colsB = 0;
}

Worker_Matmul::~Worker_Matmul() 
{
	delete workingTask;
	if ( A ) delete []A;
	if ( B ) delete []B;
}

MWReturn Worker_Matmul::unpack_init_data( void ) 
{
	int i;

	RMC->unpack ( &num_rowsA, 1 );
	RMC->unpack ( &num_rowsB, 1 );
	RMC->unpack ( &num_colsB, 1 );

	A = new int*[num_rowsA];
	for ( i = 0; i < num_rowsA; i++ )
		A[i] = new int[num_rowsB];

	B = new int*[num_rowsB];
	for ( i = 0; i < num_rowsB; i++ )
		B[i] = new int[num_colsB];

	for ( i = 0; i < num_rowsA; i++ )
		RMC->unpack ( A[i], num_rowsB );

	for ( i = 0; i < num_rowsB; i++ )
		RMC->unpack ( B[i], num_colsB );


	return OK;
}

void Worker_Matmul::execute_task( MWTask *t ) 
{
	int i, j, k;

	Task_Matmul *tf = (Task_Matmul *) t;

	tf->results = new int*[tf->endRow - tf->startRow + 1];
	for ( i = tf->startRow; i <= tf->endRow; i++ )
	{
		tf->results[i - tf->startRow] = new int[tf->nCols];
		for ( k = 0; k < num_colsB; k++ )
			tf->results[i - tf->startRow][k] = 0;

		for ( k = 0; k < num_rowsB; k++ )
			for ( j = 0; j < num_colsB; j++ )
				tf->results[i - tf->startRow][j] += A[i][k] * B[k][j];
	}
}

MWWorker*
gimme_a_worker ()
{
    return new Worker_Matmul;
}
