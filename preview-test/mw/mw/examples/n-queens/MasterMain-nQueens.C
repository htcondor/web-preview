/* WorkerMain-nQueens.C

   The main() for the worker.  Simply instantiate the Worker_nQueens
   class and call go().

*/

#include "MW.h"
#include "Driver-nQueens.h"

int main( int argc, char *argv[] ) 
{
    Driver_nQueens advisor;

    advisor.go( argc, argv );
}
