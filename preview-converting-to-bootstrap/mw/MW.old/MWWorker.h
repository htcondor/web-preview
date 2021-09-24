#ifndef MWWORKER_H
#define MWWORKER_H

#include <stdlib.h>
#include <string.h>
#include "MWTask.h"
#include <sys/time.h>  /* for struct timeval */

/**  This is the worker class that performs the tasks in the
    opportunistic condor environment.  It is an oppressed worker class
    in that in simply executes the tasks given to it and reports the
    results back to the master.  {\tt :-)}

    Capitalist stooges who wish to create an application
    must derive a class from this class, and implement the following
    two methods

    \begin{itemize}
    \item unpack_init_data()
    \item execute_task()
    \end{itemize}

    @see MWDriver
    @see MWWorker
    @author Mike Yoder, modified by Jeff Linderoth and Jean-Pierre Goux */

class MWWorker
{

public:
    
  /// Default constructor
  MWWorker();  

  /// Default Destructor
  virtual ~MWWorker();

  /// Giddyap!
  void go( int argc, char *argv[] );

#ifdef INDEPENDENT
  MWReturn ind_benchmark ( );

  MWReturn worker_mainloop_ind ();
#endif

	  /// Our RM / Comm class.  Used here only for communication.
  static MWRMComm * RMC;

 protected:

  /// The task ID of the master - used for sending messages.
  int master;

  /// The task instance that a worker will use for packing/unpacking 
  /// information from the master
  MWTask *workingTask;

  /// The name of the machine the worker is running on.
  char mach_name[64];

  /** 
      Here we might in the future pack some useful information about
      the specific machine on which we're running.  Right now,
      all workers are equal, and we pass only the hostname.

      There must be a "matching" unpack_worker_initinfo() in
      the MWDriver class.
  */
  virtual void pack_worker_initinfo() {};

  /**
     This unpacks the initial data that is sent to the worker
     once the master knows that he has started.

     There must be a "matching" pack_worker_init_data() in
     the MWDriver class derived for your application.
     
   */
  virtual MWReturn unpack_init_data() = 0;

  /** This function performs the actions that happen
      once the Worker finds out there is a task to do.
      You will need to cast the MWTask * to a pointer of the Task type
      derived for your application.  For example

      \begin{verbatim}
      Task_Fib *dt = dynamic_cast<Task_Fib *> ( t );
      assert( dt );     
      \end{verbatim}    

   */
  virtual void execute_task( MWTask * ) = 0;

	  /** Run a benchmark, given an MWTASK */
  virtual double benchmark ( MWTask * ) { return 0.0; };

  /**
    If you have some driver data that you would like to use on the
    worker in order to execute the task, you should unpack it here.
   */
  virtual void unpack_driver_task_data( void ) {};
  
  /**
     This is run before the worker_mainloop().  It prints a message 
     that the worker has been spawned, sends the master an INIT
     message, and awaits a reply from the master, upon which 
     it calls  unpack_init_data().
    
  */
  MWReturn worker_setup(); 

  /**
     This sits in a loop in which it asks for work from the master
     and does the work.  Maybe we should name this class GradStudent.
     {\tt :-)}
   */
  void worker_mainloop();

  /// Die!!!!!!
  void suicide();

 private:
	  /** Given a timeval, return a double */
  double timeval_to_double ( struct timeval t ) {
	  return (double) t.tv_sec + ( (double) t.tv_usec * (double) 1e-6 );
  }

};

#endif
