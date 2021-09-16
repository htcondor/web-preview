#ifndef USER_DRIVER_H
#define USER_DRIVER_H

#include "MWDriver.h"
#include "user_task.h"

/** The driver class derived from the MWDriver class for this application.

*/

class user_driver : public MWDriver {

 public:
    /// Constructor
    user_driver();
    /// Destructor
    ~user_driver();

    /**@name Implemented Methods

       These methods are the ones that *must* be implemented in order
       to create an application
    */

    //@{
    /// Get the info from the user.  Don't forget to get the 
    /// worker_executable!
    void get_userinfo( int argc, char *argv[] );

    /// Set up an array of tasks here
    void setup_initial_tasks( int *, MWTask *** );

    /// What to do when a task finishes:
    void act_on_completed_task( MWTask * );

    /// Put things in the send buffer here that go to a worker
    void pack_worker_init_data( void );

    /// OK, this one doesn't *have* to be...but you want to be able to
    /// tell the world the results, don't you? :-)
    void printresults();
    //@}

        /** @name Checkpointing Methods */
    //@{
    
        /** Write out the state of the master to an fp. */
    void write_master_state( FILE *fp );

        /** Read the state from an fp.  This is the reverse of 
            write_master_state(). */
    void read_master_state( FILE *fp );

        /** That simple annoying function */
    MWTask* gimme_a_task();

    //@}

 private:

		/* Basically, whatever private data you need 
		   for your application... */

};

#endif
