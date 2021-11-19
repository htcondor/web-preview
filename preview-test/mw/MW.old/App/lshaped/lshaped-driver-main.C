#include "MW.h"
#include "lshaped-driver.h"
#include <iostream.h>

int main( int argc, char *argv[] )
{
  LShapedDriver ldriver;
  
  cout << "Here we go!  argv[1] = " << argv[1] << endl;

  ldriver.go( argc, argv );
  
}
