/* WorkerMain-Matmul.C

   The main() for the worker.  Simply instantiate the Worker_Matmul
   class and call go().

*/

#include "MW.h"
#include "Worker-Matmul.h"

int main( int argc, char *argv[] ) 
{
    Worker_Matmul graduate_student;

    graduate_student.go( argc, argv );
}
