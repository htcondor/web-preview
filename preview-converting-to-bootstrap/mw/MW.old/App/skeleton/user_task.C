/* 

   The implementation of the Task_Fib class.

*/

#include "user_task.h"
#include "MW.h"

user_task::user_task() {
		/* construct default task */
}

user_task::user_task( int foo, int bar ) {
		/* More interesting constructor... */
}

user_task::user_task( const user_task& t ) {
		/* Copy constructor */
}

user_task::~user_task() {
		/* Destruct (obviously) */
}

user_task&
user_task::operator = ( const user_task& rhs ) {
		/* = operator */
}

void
user_task::printself( int level ) {
		/* It's handy to be able to print a task: */
	MWprintf ( level, "Hello, world, from %d\n", number );
		/* etc... */
}


void user_task::pack_work( void ) {
/* Send some data to the worker that identifies a task.  
   See the MWBaseComm class for communication facilities.  You'll
   simply want to do:
   
   MWComm->pack( array foo, #items, 1 );
   // etc.

   The data should come from the internal task data...
*/
}


void user_task::unpack_work( void ) {
/* Receive the same data as above; you're in the worker this time.
   Use the reverse MWComm->unpack() function... Take the data off 
   the wire and put it into this task. */
}


void user_task::pack_results( void ) {
/* We're in the worker.  Use the MWComm->pack() function, and 
   send back your results. You may want to delete any
   results that were dynamically allocated for the task, 
   since you're now done with them.  */
}


void user_task::unpack_results( void ) {
/* You're on the master side now; unpask the results 
   packed above.  */
}

void user_task::write_ckpt_info( FILE *fp ) {
/* In the master, each task should be able to checkpoint itself.
   This is because we walk down the todo list and running list
   and have to write them out to disk.  */
}

void user_task::read_ckpt_info( FILE *fp ) {
/* Reading from a checkpoint.  Reverse of above. */
}
