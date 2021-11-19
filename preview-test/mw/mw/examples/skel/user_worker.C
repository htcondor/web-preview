/* 

   The .C file for the user_worker class 

*/

#include "user_worker.h"
#include "user_task.h"
#include "pvm3.h"

user_worker::user_worker() {
    // do this here so that workingtask has type Task_Fib, and not
    // MWTask type.  This class holds info for the task at hand...
    workingTask = new user_task;

/* The idea is to keep one task in the worker, and that
   task get reused over and over.  It doesn't have to be
   this way, but it's a wee bit faster... */
}

user_worker::~user_worker() {
    delete workingTask;
}

void user_worker::unpack_init_data( void ) {
    
/* This unpacks the stuff packed in user_driver::pack_worker_init_data().
   Normally, you would do something here like unpack a big matrix
   or get *some* type of starting data... See the MWBaseComm class for
   more information on communication.   You'll want to:

   MWComm->unpack( args... )
*/
}

void user_worker::execute_task( MWTask *t ) {

    // We have to downcast this task passed to us into a user_task:
    user_task *tu = dynamic_cast<user_task *> ( t );
    
/* Now that we've got the task, take it and solve the problem! 
   The solution goes back into the task class. */
}
