/* Driver-Matmul.C

These methods specific to the matrix multiplication application

*/

#include "MW.h"
#include "Driver-Matmul.h"
#include "Worker-Matmul.h"
#include "Task-Matmul.h"

Driver_Matmul::Driver_Matmul() 
{
	A = B = C = NULL;
	num_rowsA = num_rowsB = num_colsB = 0;
	num_tasks = 0;
}

Driver_Matmul::~Driver_Matmul() 
{
	if ( A ) delete []A;
	if ( B ) delete []B;
	if ( C ) delete []C;
}

MWReturn Driver_Matmul::get_userinfo( int argc, char *argv[] ) 
{
	int i, j, numarches;
	char exec[_POSIX_PATH_MAX];

	/* File format of the input file (which is stdin)
	     # arches worker_executables ...

         num_rowsA num_rowsB num_colsB
	 A B C
	*/

	scanf ( "%d", &numarches );
	RMC->set_num_arches( numarches );
	for ( i=0 ; i<numarches ; i++ ) 
	{
		scanf ( "%s", exec );
		if ( i == 1 )
			RMC->set_worker_attributes( i, exec, 
				"((Arch==\"INTEL\") && (Opsys==\"SOLARIS26\"))" );
		else
			RMC->set_worker_attributes( i, exec, 
				"((Arch==\"INTEL\") && (Opsys==\"LINUX\"))" );
	} 
	
	scanf ( "%d %d %d", &num_rowsA, &num_rowsB, &num_colsB );

	A = new int*[num_rowsA];
	for ( i = 0; i < num_rowsA; i++ )
		A[i] = new int[num_rowsB];

	B = new int*[num_rowsB];
	for ( i = 0; i < num_rowsB; i++ )
		B[i] = new int[num_colsB];

	C = new int*[num_rowsA];
	for ( i = 0; i < num_rowsA; i++ )
		C[i] = new int[num_colsB];

	for ( i = 0; i < num_rowsA; i++ )
		for ( j = 0; j < num_rowsB; j++ )
			scanf ( "%d ", &A[i][j] );

	for ( i = 0; i < num_rowsB; i++ )
		for ( j = 0; j < num_colsB; j++ )
			scanf ( "%d ", &B[i][j] );

	set_checkpoint_frequency ( 100 );

	set_target_num_workers( 40 );

	num_tasks = num_rowsA % partition_factor == 0 ? num_rowsA / partition_factor : num_rowsA / partition_factor + 1;

	return OK;
}

MWReturn Driver_Matmul::setup_initial_tasks(int *n_init , MWTask ***init_tasks) 
{

	int i;
	int startRow, endRow;

/* Basically we have to tell MW Layer of the number of tasks and what they are */

	*n_init = num_tasks;
	*init_tasks = new MWTask *[num_tasks];

/* And now we make the Task_Matmul instances.  They will basically contain the 
   rows to operate upon.
*/

	startRow = 0;
	endRow = num_rowsA < partition_factor ? num_rowsA - 1 : partition_factor - 1;
	for ( i = 0 ; i < num_tasks ; i++ ) 
	{
		(*init_tasks)[i] = new Task_Matmul( startRow, endRow, num_colsB );
		startRow = endRow + 1;
		endRow = endRow + partition_factor > num_rowsA-1 ? num_rowsA-1 : endRow + partition_factor;
	}

	return OK;
}



MWReturn Driver_Matmul::act_on_completed_task( MWTask *t ) 
{

	int i, j;

#ifdef NO_DYN_CAST
	Task_Matmul *tf = (Task_Matmul *) t;
#else
	Task_Matmul *tf = dynamic_cast<Task_Matmul *> ( t );
#endif

	for ( i = tf->startRow; i <= tf->endRow; i++ )
		for ( j = 0; j < num_colsB; j++ )
			C[i][j] = tf->results[i - tf->startRow][j];
	
	return OK;
}

MWReturn Driver_Matmul::pack_worker_init_data( void ) 
{
	// One has to pass the entire A and B matrices to the workers.
	int i;

	RMC->pack( &num_rowsA, 1 );
	RMC->pack( &num_rowsB, 1 );
	RMC->pack( &num_colsB, 1 );

	for ( i = 0; i < num_rowsA; i++ )
		RMC->pack ( A[i], num_rowsB );

	for ( i = 0; i < num_rowsB; i++ )
		RMC->pack ( B[i], num_colsB );

	return OK;
}

void Driver_Matmul::printresults() 
{
	MWprintf ( 10, "The resulting Matrix is as follows\n");

	for ( int i = 0; i < num_rowsA; i++ )
	{
		for ( int j = 0; j < num_colsB; j++ )
			MWprintf ( 10, "%d ", C[i][j] );

		MWprintf ( 10, "\n" );
	}
}

void
Driver_Matmul::write_master_state( FILE *fp ) 
{
	// This is not checkpoint enabled.
}

void 
Driver_Matmul::read_master_state( FILE *fp ) 
{
	// This is not checkpoint enabled.
}

MWTask*
Driver_Matmul::gimme_a_task() 
{
	/* The MWDriver needs this.  Just return a Task_Matmul instance. */
	return new Task_Matmul;
}

MWDriver*
gimme_the_master () 
{
    return new Driver_Matmul;
}
