/* Task-nQueens.C */

#include "Task-nQueens.h"
#include "MW.h"

#include "MWRMComm.h"
#include <string.h>

Task_nQueens::Task_nQueens() 
{
	N = -1;
	num = 0;
	results = NULL;
}

Task_nQueens::Task_nQueens( int n, int Num )
{
	N = n;
	num = Num;
	results = NULL;
}

Task_nQueens::~Task_nQueens() 
{
    if ( results )
        delete [] results;
}

void
Task_nQueens::printself( int level ) 
{
	// Right now nothing
}


void Task_nQueens::pack_work( void ) 
{
	RMC->pack( &N, 1, 1 );
	RMC->pack( &num, 1, 1 );
}


void Task_nQueens::unpack_work( void ) 
{
	RMC->unpack( &N, 1, 1 );
	RMC->unpack( &num, 1, 1 );
}


void Task_nQueens::pack_results( void ) 
{
	int res;
	if ( results ) res = 1;
	else res = 0;
	RMC->pack ( &res, 1, 1 );

	if ( res == 1 )
	{
		RMC->pack( results, N, 1 );
		delete [] results;
		results = NULL;
	}
}


void Task_nQueens::unpack_results( void ) 
{
	int res;
	RMC->unpack ( &res, 1, 1 );

	if ( res == 1 )
	{
		results = new int[N];
    		RMC->unpack( results, N, 1 );
	}
}

void Task_nQueens::write_ckpt_info( FILE *fp ) 
{
	// Not checkpoint enabled.
}

void Task_nQueens::read_ckpt_info( FILE *fp ) 
{
	// Not checkpoint enabled.
}
