/* MWTask.C

   The implementation of the MWTask class

*/

#include "MWTask.h"
#include <stdio.h>

#ifndef UNDEFINED
#define UNDEFINED -1
#endif

#ifdef INDEPENDENT
extern MWRMComm *GlobalRMComm;
MWRMComm *MWTask::RMC = GlobalRMComm;
#endif

MWTask::MWTask() {
	number = UNDEFINED;
	worker = NULL;
	next   = NULL;
#ifdef INDEPENDENT
	RMC = GlobalRMComm;
#endif
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




