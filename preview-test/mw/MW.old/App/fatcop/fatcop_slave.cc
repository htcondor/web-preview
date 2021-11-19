#include "error.hh"
#include "MW.h"
#include "FATCOP_worker.hh"

int main( int ac, char *av[] ) {

  FATCOP_worker graduate_student;

  // Set up debug level here...(output goes to 'error' in submit file)
  set_MWprintf_level ( 60 );

#if defined( DEBUGMW )
  set_MWprintf_level ( 75 );
#endif
  
	
  MWprintf ( 10, "A worker is starting.\n" );

  graduate_student.go( ac, av );

  return 0;
  
}
