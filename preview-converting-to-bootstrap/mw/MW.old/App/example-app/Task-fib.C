/* Task-fib.C

   The implementation of the Task_Fib class.

*/

#include "Task-fib.h"
#include "MW.h"

#include "MWRMComm.h"
#include <string.h>

Task_Fib::Task_Fib() {

    first           = -1;
    second          = -1;
    sequencelength  = -1;
    results        = NULL;
}

Task_Fib::Task_Fib( int first, int second, int seqlen ) {

    this->first      = first;
    this->second     = second;
    sequencelength  = seqlen;

    results = NULL;
}

Task_Fib::Task_Fib( const Task_Fib& t ) {
    this->first = t.first;
    this->second = t.second;
    this->sequencelength = t.sequencelength;
    if ( t.results ) {
        this->results = new int[t.sequencelength];
        memcpy( this->results, t.results, t.sequencelength * sizeof( int ) );
    }
    else {
        this->results = NULL;
    }
}

Task_Fib::~Task_Fib() {

        // get rid of results array
    if ( results )
        delete [] results;

}

Task_Fib&
Task_Fib::operator = ( const Task_Fib& rhs ) {

    if ( this == &rhs ) {
        return *this;
    }

    if ( results ) delete [] results;
    first = rhs.first;
    second = rhs.second;
    sequencelength = rhs.sequencelength;
    if ( rhs.results ) {
        results = new int[sequencelength];
        memcpy( results, rhs.results, sequencelength * sizeof( int ) );
    }
    else {
        results = NULL;
    }
    
    return *this;
}

void
Task_Fib::printself( int level ) {
    MWprintf ( level, "n:%d f:%d s:%d l:%d | ", 
			   number, first, second, sequencelength );
    if ( results ) {
        for ( int i=0 ; i<sequencelength ; i++ ) {
            MWprintf ( level, "%d ", results[i] );
        }
    }
    MWprintf ( level, "\n" );
}


void Task_Fib::pack_work( void ) {
    RMC->pack( &first, 1, 1 );
    RMC->pack( &second, 1, 1 );
    RMC->pack( &sequencelength, 1, 1 );
}


void Task_Fib::unpack_work( void ) {
    RMC->unpack( &first, 1, 1 );
    RMC->unpack( &second, 1, 1 );
    RMC->unpack( &sequencelength, 1, 1 );
}


void Task_Fib::pack_results( void ) {
    RMC->pack( results, sequencelength, 1 );
    // Must do this here; we're done with the results now...(on worker side!)
    delete [] results;
    results = NULL;
}


void Task_Fib::unpack_results( void ) {
    // must make results here; this is master side.
    MWprintf (10, "Before allocating memory in unpack_results\n");
    results = new int[sequencelength];
    RMC->unpack( results, sequencelength, 1 );
}

void Task_Fib::write_ckpt_info( FILE *fp ) {
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

void Task_Fib::read_ckpt_info( FILE *fp ) {
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
