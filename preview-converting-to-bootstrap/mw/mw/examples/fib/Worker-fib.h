#ifndef WORKER_FIB_H
#define WORKER_FIB_H

#include "MWWorker.h"
#include "Task-fib.h"

/** The Worker class created for my fibonacci example application.

    ...more to come...

*/

class Worker_Fib : public MWWorker {

 public:
    /// Default Constructor
    Worker_Fib();

    /// Default Destructor
    ~Worker_Fib();

    /**@name Implemented methods.
       
       These are the ones that must be Implemented in order
       to create an application
    */
    //@{
    /// Unpacks the "base" application data from the PVM buffer
    MWReturn unpack_init_data( void );
    
    /** Executes the given task.
        Remember to cast the task as type {\ttDummyTask}
    */
    void execute_task( MWTask * );
    //@}

		/** Benchmarking. */
	double benchmark( MWTask *t );

 private:

    /**  Given two seed numbers (firstseed and secondseed), fill the array
         pointed at by sequence to contain the first SEQUENCELENGTH numbers
         in the related fibonacci series.  Pretty simple, really.
         @param firstseed The first of the pair to make the sequence
         @param secondseed The second of the pair to begin the sequence
         @param sequence A pointer to an int array that will hold the 
                         completed fibonacci sequence.
         @param len The length of sequence.
    */
    void get_fib ( int firstseed, int secondseed, int *sequence, int len );

};

#endif
