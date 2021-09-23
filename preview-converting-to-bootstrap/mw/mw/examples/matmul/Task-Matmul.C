/* Task-Matmul.C */

#include "Task-Matmul.h"
#include "MW.h"

#include "MWRMComm.h"
#include <string.h>

Task_Matmul::Task_Matmul() 
{
	startRow = endRow = -1;
	nCols = 0;
	results = NULL;
}

Task_Matmul::Task_Matmul( int strtRow, int edRow, int ncols )
{
	startRow = strtRow;
	endRow = edRow;
	nCols = ncols;

	results = NULL;
}

Task_Matmul::~Task_Matmul() 
{
    if ( results )
        delete [] results;
}

void
Task_Matmul::printself( int level ) 
{
	// Right now nothing
}


void Task_Matmul::pack_work( void ) 
{
	RMC->pack( &startRow, 1, 1 );
	RMC->pack( &endRow, 1, 1 );
	RMC->pack( &nCols, 1, 1 );
}


void Task_Matmul::unpack_work( void ) 
{
	RMC->unpack( &startRow, 1, 1 );
	RMC->unpack( &endRow, 1, 1 );
	RMC->unpack( &nCols, 1, 1 );
}


void Task_Matmul::pack_results( void ) 
{
	for ( int i = startRow; i <= endRow; i++ )
		RMC->pack( results[i-startRow], nCols, 1 );
	delete [] results;
	results = NULL;
}


void Task_Matmul::unpack_results( void ) 
{
	results = new int*[endRow - startRow + 1];
	for ( int i = startRow; i <= endRow; i++ )
	{
		results[i - startRow] = new int[nCols];
    		RMC->unpack( results[i - startRow], nCols, 1 );
	}
}

void Task_Matmul::write_ckpt_info( FILE *fp ) 
{
	// Not checkpoint enabled.
}

void Task_Matmul::read_ckpt_info( FILE *fp ) 
{
	// Not checkpoint enabled.
}
