/* lshaped-worker-main.C

   The main() for the worker.  

*/

#include "MW.h"
#include "lshaped-worker.h"
#include <iostream.h>

int main() 
{
  LShapedWorker lworker;
  
  cout << "A worker is starting" << endl;

  lworker.go();

}
