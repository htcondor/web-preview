/* WorkerMain-fib.C

   The main() for the worker.  Simply instantiate the Worker_Fib
   class and call go().

*/

#include "MW.h"
#include "Driver-fib.h"

int main( int argc, char *argv[] ) {

    Driver_Fib advisor;

		// might as well set debugging level here...
	set_MWprintf_level( 75 );

    MWprintf ( 10, "The master is starting.\n" );

    advisor.go( argc, argv );

}
