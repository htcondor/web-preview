/* WorkerMain-nQueens.C

   The main() for the worker.  Simply instantiate the Worker_nQueens
   class and call go().

*/

#include "MW.h"
#include "Worker-nQueens.h"

int main( int argc, char *argv[] ) 
{
    Worker_nQueens graduate_student;

    graduate_student.go( argc, argv );
}
