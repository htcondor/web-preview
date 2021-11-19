#ifndef USER_TASK_H
#define USER_TASK_H

#include <stdio.h>
#include "MWTask.h"

/** 

	The definition of a task...

*/

class user_task : public MWTask {

 public:
    
    /// Default Constructor
    user_task();

    /** Construct with parameters.
    */
    user_task( int foo, int bar );

    user_task( const user_task& );

    ~user_task();

    user_task& operator = ( const user_task& );

    /**@name Implemented methods
       
       These are the task methods that must be implemented 
       in order to create an application.
    */
    //@{
    /// Pack the work for this task into the PVM buffer
    void pack_work( void );
    
    /// Unpack the work for this task from the PVM buffer
    void unpack_work( void );
    
    /// Pack the results from this task into the PVM buffer
    void pack_results( void );
    
    /// Unpack the results from this task into the PVM buffer
    void unpack_results( void );
    //@}

        /// dump self to screen:
    void printself( int level = 60 );

    /**@name Checkpointing Implementation 
       These members used when checkpointing. */
    //@{
        /// Write state
    void write_ckpt_info( FILE *fp );
        /// Read state
    void read_ckpt_info( FILE *fp );
    //@}

		/* The rest will be for your app.. */
};

#endif








