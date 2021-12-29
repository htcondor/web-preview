/* WorkerMain-fib.C

   The main() for the worker.  Simply instantiate the Worker_Fib
   class and call go().

*/

#include "MW.h"
#include "Worker-fib.h"

int main( int argc, char *argv[] ) {

    Worker_Fib graduate_student;

		// Set up debug level here...
	set_MWprintf_level ( 75 );
	
    MWprintf ( 10, "A worker is starting.\n" );
    for ( int i = 1; i < argc; i++ )
	MWprintf(10, " Args %d was %s\n", i, argv[i] );

    graduate_student.go( argc, argv );

}
