/* 
 *	This is the master's main function. The whole starts from this point
 *	This was done on 2/2/00 to make MW pure Object Oriented.
 *
 *	Author:
 *		Sanjeev. R. Kulkarni
 */



#include <stdio.h>
#include "MWDriver.h"

extern MWDriver* gimme_the_master();

int main ( int argc, char *argv[] )
{
	MWDriver *driver;
	driver = gimme_the_master ( );
	driver->go ( argc, argv );
}
