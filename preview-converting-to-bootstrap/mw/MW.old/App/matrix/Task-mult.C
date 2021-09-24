/* Task-Mult.C

   The implementation of the Task_Mult class.

*/

#include "Task-Mult.h"
#include "MW.h"

#include "MWRMComm.h"
#include <string.h>

Task_Mult::Task_Mult() {

    length        = 0;
    breadth	  = 0;
    array1        = NULL;
    array2  	  = NULL;
    results       = NULL;
}

Task_Mult::Task_Mult( int len, int br, int **Aarray, int **Barray ) {

    length 	= len;
    breadth 	= br;
    array1	= Aarray;
    array2	= Barray;
    results	= NULL;
}

Task_Mult::~Task_Mult() {

    if ( array1 ) 
    	delete [] array1;
    if ( array2 ) 
    	delete [] array2;
    if ( results )
        delete [] results;

}

void Task_Mult::pack_work( void ) {

    RMC->pack( &length, 1, 1 );
    RMC->pack( &br, 1, 1 );

    for ( int i = 0; i < length; i++ )
    {
	RMC->pack ( &array1[i], breadth, 1 );
    }
    for ( int i = 0; i < length; i++ )
    {
	RMC->pack ( &array2[i], breadth, 1 );
    }
}


void Task_Mult::unpack_work( void ) {
    int i;

    RMC->unpack( &breadth, 1, 1 );
    array1 = new int*[len];
    array2 = new int*[len];
    for ( i = 0; i < len; i++ )
    {
    	array1[i] = new int[breadth];
    	array2[i] = new int[breadth];
    }
    for ( i = 0; i < len; i++ )
    {
    	RMC->unpack ( &array1[i], breadth, 1 );
    }
    for ( i = 0; i < len; i++ )
    {
    	RMC->unpack ( &array2[i], breadth, 1 );
    }
}


void Task_Mult::pack_results( void ) {
    RMC->pack( results, sequencelength, 1 );
    // Must do this here; we're done with the results now...(on worker side!)
    delete [] results;
    results = NULL;
}


void Task_Mult::unpack_results( void ) {
    // must make results here; this is master side.
    MWprintf (10, "Before allocating memory in unpack_results\n");
    results = new int[sequencelength];
    RMC->unpack( results, sequencelength, 1 );
}

void Task_Mult::write_ckpt_info( FILE *fp ) {
    /* here's the format:
       <first> <second> <sequencelength> <results[0]>  ... \n
       or
       <first> <second> <sequencelength> 0\n
       if there are no results.
    */

    fprintf ( fp, "%d %d %d ", first, second, sequencelength );
    if ( results ) {
        for ( int i=0 ; i<sequencelength ; i++ ) {
            fprintf ( fp, "%d ", results[i] );
        }
    }
    else {
        fprintf ( fp, "0" );
    }

    fprintf ( fp, "\n" );
}

void Task_Mult::read_ckpt_info( FILE *fp ) {
        // remove old info, if exists:
    if ( results ) delete [] results;

        /* see above format... */
    fscanf( fp, "%d %d %d ", &first, &second, &sequencelength );
    int reszero;
    fscanf( fp, "%d", &reszero );
    if ( reszero != 0 ) {
        results = new int[sequencelength];
        results[0] = reszero;
        for ( int i=1 ; i<sequencelength ; i++ ) {
            fscanf( fp, "%d ", &results[i] );
        }
    }
    else {
        results = NULL;
    }
}
