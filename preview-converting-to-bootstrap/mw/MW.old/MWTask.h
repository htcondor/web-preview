#ifndef MWTASK_H
#define MWTASK_H

#include "MW.h"
#include "MWWorkerID.h"
#include "CommRM/MWRMComm.h"
#include <stdio.h>

/** 
    This the class that represents a unit of work. 

    The task consits of two main components.  The "work" to be done,
    and the "result" of the completed task.
    In order to create an application, the user must specify methods
    for packing and unpacking both the "work" and "result" portions
    of the task.
    The user is not responsible for initializing the send
    or for packing the task's number    

    When the task is being serviced by a worker, it contains a link
    to the ID of that instance of the worker.


    @see MWDriver
    @see MWWorker
    @author Mike Yoder, modified by Jeff Linderoth and Jean-Pierre Goux
*/

class MWTask
{
 public:   

  /// Default constructor
  MWTask();
  
  /// Default Destructor
  virtual ~MWTask();

  /// The task's number
  int number;

	  /** @name Time and usage data */
	  //@{
	  /** The time (wall clock) that it takes to run the 'execute_task'
		  function on the worker's side. */
  double working_time;
	  /** The amount of user+system time taken by this task, measured
		  from start to finish of the 'execute_task' function. */
  double cpu_time;
	  //@}

	  /**@name Packing and Unpacking 

		 The user must pack and unpack the contents of the class
		 so that it can be sent to the worker and back.  These 
		 functions will make RMC->pack() and RMC->unpack() calls.
   */
  //@{

  /// Pack the work portion of the task
  virtual void pack_work( void ) = 0;

  /// Unpack the work portion of the task
  virtual void unpack_work( void ) = 0;

  /// Pack the result portion of the task
  virtual void pack_results( void ) = 0;

  /// Unpack the result portion of the task
  virtual void unpack_results( void ) = 0;

  /// Dump this task to the screen
  virtual void printself( int level = 60 );

	  //@}

  /**@name Checkpointing Utilities

     These two functions are used when checkpointing.  Simply put, 
     they write/read the state of the task to this file pointer 
     provided. */

      //@{

      /** Write the state of this task out to a FILE* */
  virtual void write_ckpt_info( FILE *fp ) {};
      /** Read the state of this task from a FILE* (overwriting
          any existing state */
  virtual void read_ckpt_info( FILE *fp ) {};


      //@}

  /**@name List management 

     The task also has pointers that aid for managing the list
     of tasks to be done
   */

  //@{
  /// A pointer to the worker ID executing this task (NULL if none)
  MWWorkerID *worker;

  /// A pointer to the next task in the list (NULL if none)
  MWTask *next;
  //@}

  static MWRMComm * RMC;
  
};

#endif


