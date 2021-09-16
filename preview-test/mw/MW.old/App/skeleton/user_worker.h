#ifndef USER_WORKER_H
#define USER_WORKER_H

#include "MWWorker.h"
#include "user_task.h"

/** 

	This is your user-defined worker, that knows how do your
	user-defined tasks...

*/

class user_worker : public MWWorker {

 public:
    /// Default Constructor
    user_worker();

    ~user_worker();

    /**@name Implemented methods.
       
       These are the ones that must be Implemented in order
       to create an application
    */
    //@{
    /// Unpacks the "base" application data from the PVM buffer
    void unpack_init_data( void );
    
    /** Executes the given task.
	 */
    void execute_task( MWTask * );
    //@}
    
 private:


		/* Private stuff for your app... */

};

#endif
