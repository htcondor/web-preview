/* WorkerMain-Matmul.C

   The main() for the worker.  Simply instantiate the Worker_Matmul
   class and call go().

*/

#include "MW.h"
#include "Driver-Matmul.h"

int main( int argc, char *argv[] ) 
{
    Driver_Matmul advisor;

    advisor.go( argc, argv );
}
