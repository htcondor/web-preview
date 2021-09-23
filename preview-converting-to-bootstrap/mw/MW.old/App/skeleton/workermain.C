/* 

   The main() for the worker.  Simply instantiate the Worker_Fib
   class and call go().

*/

#include "MW.h"
#include "user_worker.h"

int main() {

    user_worker graduate_student;

		// Set up debug level here...(output goes to 'error' in submit file)
	set_MWprintf_level ( 75 );
	
    MWprintf ( 10, "A worker is starting.\n" );

    graduate_student.go();

}
