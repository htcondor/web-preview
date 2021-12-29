/* MWTask.C

   The implementation of the MWTask class

*/

#include "MWTask.h"
#include <stdio.h>


MWTask::MWTask() {
	number = -1;
	worker = NULL;
	next   = NULL;
	taskType = MWNORMAL;
}

MWTask::~MWTask() {
  if ( worker )
    if ( worker->runningtask )
      worker->runningtask = NULL;

}

void MWTask::printself( int level ) 
{    
	MWprintf ( level, "  Task %d\n", number);
}
