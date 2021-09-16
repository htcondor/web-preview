#ifndef TASK_FIB_H
#define TASK_FIB_H

#include <stdio.h>
#include "MWTask.h"

/** The task class specific to my Fibonacci example application.

    ...more to come...
*/

class Task_Fib : public MWTask {

 public:
    
    /// Default Constructor
    Task_Fib();

    /** Construct with parameters.
        @param first The first number in the pair of starting "seed" numbers
        @param second The second of the "seed" numbers
        @param seqlen The length of the sequence to make 
    */
    Task_Fib( int first, int second, int seqlen );

    Task_Fib( const Task_Fib& );

    /// Default Destructor
    ~Task_Fib();

    Task_Fib& operator = ( const Task_Fib& );

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

        /// the first number in the fibonacci sequence
    int first;
        /// the second number.
    int second;
        /// How many numbers in the sequence? (incl. the first two)
    int sequencelength;
        /// A pointer to the completed sequence
    int *results;
};

#endif

