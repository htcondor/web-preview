/* 
 *	This is the master's main function. The whole starts from this point
 *	This was done on 2/2/00 to make MW pure Object Oriented.
 *
 *	Author:
 *		Sanjeev. R. Kulkarni
 */



#include <stdio.h>
#include "MWWorker.h"
extern MWWorker* gimme_a_worker();

int main ( int argc, char *argv[] )
{
	MWWorker *worker;
	worker = gimme_a_worker ( );
	worker->go ( argc, argv );
}
