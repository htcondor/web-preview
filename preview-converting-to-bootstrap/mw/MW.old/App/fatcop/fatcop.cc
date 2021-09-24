#include "error.hh"
#include "MW.h"

#include "FATCOP_driver.hh"

//main program


int main( int argc, char *argv[] ) {

  FATCOP_driver advisor;

  // might as well set debugging level here...
  set_MWprintf_level( 75 );
#if defined( DEBUGMW )
  set_MWprintf_level( 75 );
#endif
  

  MWprintf ( 10, "The master is starting.\n" );

  advisor.go( argc, argv );

  return 0;

}
