/* 

These methods will be specific to your application...
You may want to search-replace user_driver with a name for your app.

*/

#include "MW.h"
#include "user_driver.h"
#include "user_worker.h"
#include "user_task.h"
#include <pvm3.h>

user_driver::user_driver() {
}

user_driver::~user_driver() {
}

void user_driver::get_userinfo( int argc, char *argv[] ) {

/* Here (in general) you want to read some information from stdin.
   The things that you will read will come from the file labelled
   as 'input' in your submit file.

   This is also a good place to set the checkpoint frequency
*/

/* You'll want to use the 'set_worker_executable() function to
   tell the MWDriver what executables to use.  If you're running
   a multiple architecture job, you'll have to give it more than
   one.  The calls might look like this:

   set_worker_executable( "a.out.SOLARIS", 0 );
   set_worker_executable( "a.out.LINUX", 1 );

   for machine classes 0 and 1.

   See ../example-app/Driver-fib.C for another example...
*/

}

void user_driver::setup_initial_tasks(int *n_init , MWTask ***init_tasks) {

		/* In general, you'd like something like this: */

		// ( num_tasks should be user_driver data, but you get the idea ):
	int num_tasks = 5;

    *n_init = num_tasks;
    *init_tasks = new MWTask *[num_tasks];

    (*init_tasks)[0] = new user_task; /* add args... */

		/* etc... You basically make init_tasks an array of pointers
		   to tasks and then fill them in. */
}


void user_driver::act_on_completed_task( MWTask *t ) {

		// dynamic casting:
    user_task *tf = dynamic_cast<user_task *> ( t );

		/* Here you'll want to do something based on the results 
		   of that task.  That "something" can be to store the result
		   (you must use the copy constructor, for the task gets 
		   deleted right after this call) or possibly add tasks
		   to do... */

		/* If you are interested in making work steps, then see the
		   work_steps-HOWTO file in this directory. */
}

void user_driver::pack_worker_init_data( void ) {

		/* The following gets unpacked in user_worker::unpack_init_data()

		   Here, one would normally pack up a matrix or some other relevant 
		   starting data... this is a one-time-only send of starting 
		   data for all tasks.  See the MWBaseComm class for 
		   communication details.   You'll simply:

		   MWComm->pack( data array, # items, 1 );
		   //etc...
		*/
}

void user_driver::printresults() {

		/* Just what the name says... */
}

void
user_driver::write_master_state( FILE *fp ) {

		/* This is for logical checkpointing.  Here, we write out
		   to fp all of the internal state of the user_driver. */
}

void 
user_driver::read_master_state( FILE *fp ) {

		/* The reverse of the above function.  This will happen
		   exactly once when restarting from a checkpoint. */
}

MWTask*
user_driver::gimme_a_task() {
		// return an instance of our task:
    return new user_task;
}

