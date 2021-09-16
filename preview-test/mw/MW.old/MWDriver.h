#ifndef MWDRIVER_H
#define MWDRIVER_H

#include "MWTask.h"
#include "MWWorker.h"
#include "MWStats.h"
#include "CommRM/MWRMComm.h"

/// This is the "key" by which the task list can be managed.
typedef double MWKey;

/** The ways in which tasks may be added to the list.
*/
enum MWTaskAdditionMode {
  /// Tasks will be added at the end of the list
  ADD_AT_END,
  /// Tasks will be added at the beginning
  ADD_AT_BEGIN,
  /// Tasks will be added based on their key (low keys before high keys)
  ADD_BY_KEY 
};

/** The ways in which tasks may be removed from the list.
*/
enum MWTaskRetrievalMode {
  /// Task at head of list will be returned.
  GET_FROM_BEGIN,
  /// Task with lowest key will be retrieved
  GET_FROM_KEY 
};

/** The suspension policy to use - What do we do when it happens? */
enum MWSuspensionPolicy {
		/// Normally do nothing unless there are idle workers.
	DEFAULT,
		/** Always reassign the task; move it to the front of the 
			todo list */
	REASSIGN 
};

/** Tasks are always assigned to he first "IDLE" machine.  By ordering
    the machine list, we can implement a number of scheduling policies.
    Insert your own favorite policy here.
*/
enum MWMachineOrderingPolicy {

  /// The machines are ordered simply as they become available
  NO_ORDER,
  
  /** Machines are ordered by the "benchmark" result.  Larger benchmark results
      go first, so if using time to complete a task as the benchmark, you
      should return 1/time as the benchmark value.
  */
  BY_USER_BENCHMARK,

  /// Machines are ordered by KFLOPS reported by Condor.
  BY_KFLOPS
  
};


/** This class is responsible for managing an application in an 
    opportunistic environment.  The goal is to be completely fault - 
    tolerant, dealing with all possiblities of host (worker) problems.  
    To do this, the MWDriver class manages a set of tasks and a set 
    of workers.  It monitors messages about hosts coming up and 
    going down, and assigns tasks appropriately.

	This class is built upon some sort of resource management and 
	message passing lower layer.  Previously, it was built directly 
	on top of Condor - PVM, but the interface to that has been 
	abstracted away so that it can use any facility that provides 
	for resource management and message passing.  See the abstract
	MWRMComm class for details of this lower layer.  When interfacing
	with this level, you'll have use the RMC object that's a static
	member of the MWDriver, MWTask, and MWWorker class.  

    To implement an application, a user must derive a class from this
    base class and implement the following methods:

    \begin{itemize}
    \item get_userinfo()
    \item setup_initial_tasks()
    \item pack_worker_init_data()
    \item act_on_completed_task()
    \end{itemize}

    Similar application dependent methods must be implemented
    for the "Task" of work to be done and the "Worker" who performs
    the tasks.
    
    @see MWTask
    @see MWWorker
    @author Mike Yoder, Jeff Linderoth, and Jean-Pierre Goux
*/


class MWDriver {

 public:
	
		/// Default constructor
	MWDriver();
	
		/** Destructor - walks through lists of tasks & workers and
			deletes them. */
	
	virtual ~MWDriver();
	
		/** This method runs the entire fault-tolerant
			application in the condor environment.  What is *really* does
			is call setup_master(), then master(), then printresults(), 
			and then ends.  See the other functions	for details. 
		*/
	void go( int argc, char *argv[] );
	
		/** 
			This version of go simply calls go(0, NULL).
		*/
	void go() { go ( 0, NULL ); };
	
		/**
		   Prints the Results.  It does not do anything useful now.
		*/
	virtual void printresults() {
		MWprintf ( 10, "YEAH!  You finished!\n" );
	}

		/** A static instance of our Resource Management / Communication
			class.  It's a member of this class because that way derived
			classes can use it easily; it's static because there should
			only be one instance EVER.  The instance of RMC in the MWTask
			class is actually a pointer to this one... */
	static MWRMComm * RMC;


	
 protected:
	
		/**@name A. Pure Virtual Methods
		   
		   These are the methods from the MWDriver class that a user 
		   {\bf must} reimplement in order to have to create an application. 
		*/
	
		//@{
		/** This function is called to read in all information
			specific to a user's application and do any initialization on
			this information.
		*/
	virtual MWReturn get_userinfo( int argc, char *argv[] ) = 0; 
	
		/** This function must return a number n > 0 of pointers
			to Tasks to "jump start" the application.
			
			The MWTasks pointed to should be of the task type derived
			for your application
		*/
	virtual MWReturn setup_initial_tasks( int *n, MWTask ***task ) = 0;
	
		/** 
			This function performs actions that happen
			once the Driver receives notification of a completed task.  
			You will need to cast the MWTask * to a pointer of the Task type 
			derived for your application.  For example
			
			\begin{verbatim}
			Task_Fib *dt = dynamic_cast<Task_Fib *> ( t );
			assert( dt );     
			\end{verbatim}
		*/
	virtual MWReturn act_on_completed_task( MWTask * ) = 0;
	
		/**
		   A common theme of Master-Worker applications is that there is 
		   a base amount of "initial" data defining the problem, and then 
		   just incremental data defining "Tasks" to be done by the Workers.
		   
		   This one packs all the user's initial data.  It is unpacked 
		   int the worker class, in unpack_init_data().
		*/
	virtual MWReturn pack_worker_init_data( void ) = 0;
	
		/**
		   This one unpacks the "initial" information sent to the driver
		   once the worker initializes. 
		   
		   Potential "initial" information that might be useful is...
		   \begin{itemize}
		   \item Information on the worker characteristics  etc...
		   \item Information on the bandwith between MWDriver and worker
		   \end{itemize}
		   
		   These sorts of things could be useful in building some 
		   scheduling intelligence into the driver.
		   
		*/
	virtual void unpack_worker_initinfo( MWWorkerID *w ) {};
	
		/**
		   OK, This one is not pure virtual either, but if you have some 
		   "driver" data that is conceptually part of the task and you wish
		   not to replicate the data in each task, you can pack it in a
		   message buffer by implementing this function.  If you do this, 
		   you must implement a matching unpack_worker_task_data()
		   function.
		*/
	
	virtual void pack_driver_task_data( void ) {};
	
		//@}
	
	
		/**@name B. Task List Management
		   
		   These functions are to manage the list of Tasks.  MW provides
		   default useful functionality for managing the list of tasks.
		   
		*/
	
		//@{
	
		/// Add a task to the list
	void addTask( MWTask * );
	
		/** Add a bunch of tasks to the list.  You do this by making
			an array of pointers to MWTasks and giving that array to 
			this function.  The MWDriver will take over memory 
			management for the MWTasks, but not for the array of 
			pointers, so don't forget to delete [] it! */
	void addTasks( int, MWTask ** );
	
		/// Sets the function that MWDriver users to get the "key" for a task
	void set_task_key_function( MWKey (*)( MWTask * ) );
	
		/// Set the mode you wish for task addition.  See LABELXXX
	int set_task_add_mode( MWTaskAdditionMode );
	
		/// Set the mode you wish for task retrieval.  See LABELXXX
	int set_task_retrieve_mode( MWTaskRetrievalMode );

  		///   Sets the machine ordering policy.  See LABELXXX
        int set_machine_ordering_policy( MWMachineOrderingPolicy );

		/// (Mostly for debugging) -- Prints the task keys in the todo list
	int print_task_keys( void );
	
		/// This sorts the task list by the key that is set
	int sort_task_list( void );
	
		/** This deletes all tasks in the task list with a key worse than 
			the one specified */
	int delete_tasks_worse_than( MWKey );
	
		/// returns the number of tasks on the todo list.
	int get_number_tasks();
	
		//@}
	
		/** @name C. Worker Policy Management */
		//@{
		/** Sets the desired number of workers.  This can be called at any 
			time, but a call with a value less than the preceding call 
			will have no effect.  If iWantThisMany is -1, it imples that
			you want as many workers as possible. */
	void set_target_num_workers( int iWantThisMany );
	
		/** Sets the desired number of workers.  This takes an array of 
			integers, which represents the number of desired machines for
			each arch class.  */
	void set_target_num_workers( int *iWantThisany, int num_arches );

		/** Set the policy to use when suspending.  Currently 
			this can be either DEAFULT or REASSIGN */
	void set_suspension_policy( MWSuspensionPolicy );
	
		//@}

		/**@name D. Event Handling Methods
		   
		   In the case that the user wants to take specific actions
		   when notified of processors going away, these methods
		   may be reimplemented.  Care must be taken when
		   reimplementing these, or else things may get messed up.
		   
		   Probably a better solution in the long run is to provide 
		   users hooks into these functions or something. 
		   
		   Basic default functionality that updates the known
		   status of our virtual machine is provided. 
		   
		*/
	
		//@{
	
		/** Here, we get back the benchmarking
			results, which tell us something about the worker we've got.
			Also, we could get some sort of error back from the worker
			at this stage, in which case we remove it. */
	virtual MWReturn handle_benchmark( MWWorkerID *w );

        /** This is what gets called when a host goes away.  We figure out
            who died, remove that worker from our records, remove its task
            from the running queue (if it was running one) and put that
            task back on the todo list. */
	
	virtual void handle_hostdel();
	
		/** Implements a suspension policy.  Currently either DEFAULT or
			REASSIGN, depending on how suspensionPolicy is set. */
	virtual void handle_hostsuspend();
	
		/** Here's where you go when a host gets resumed.  Usually, 
			you do nothing...but it's nice to know...*/
	virtual void handle_hostresume();
	
		/** We do basically the same thing as handle_hostdel().  One might 
			{\em think} that we could restart something on that host; 
			in practice, however, it means that the host has gone down, too.
			We put that host's task back on the todo list.
		*/
	virtual void handle_taskexit();
		//@}
	
		/** @name E. Checkpoint Handling Functions
			
			These are logical checkpoint handling functions.  They are
			virtual, and are *entirely* application-specific.  In them, the
			user must save the "state" of the application to permanent
			storage (disk).  To do this, you need to:
			
			\begin{itemize}
			 \item Implement the methods write_master_state() and
			    read_master_state() in your derived MWDriver app.
			 \item Implement the methods write_ckpt_info() and 
			    read_ckpt_info() in your derived MWTask class.
			\end{itemize}
			
			Then MWDriver does the rest for you.  When checkpoint() is
			called (see below) it opens up a known filename for writing.
			It passes the file pointer of that file to write_master_state(), 
			which dumps the "state" of the master to that fp.  Here 
			"sate" includes all the variables, info, etc of YOUR
			CLASS THAT WAS DERIVED FROM MWDRIVER.  All state in
			MWDriver.C is taken care of (there's not much).  Next, 
			checkpoint will walk down the running queue and the todo
			queue and call each member's write_ckpt_info().  

			Upon restart, MWDriver will detect the presence of the 
			checkpoint file and restart from it.  It calls 
			read_master_state(), which is the inverse of 
			write_master_state().  Then, for each task in the 
			checkpoint file, it creates a new MWTask, calls 
			read_ckpt_info() on it, and adds it to the todo queue.
			
			We start from there and proceed as normal.
			
			One can set the "frequency" that checkpoint files will be 
			written (using set_checkpoint_frequency()).  The default
			frequency is zero - no checkpointing.  When the frequency is
			set to n, every nth time that act_on_completed_task gets 
			called, we checkpoint immediately afterwards.  If your
			application involves "work steps", you probably will want to 
			leave the frequency at zero and call checkpoint yourself
			at the end of a work step.
			
		*/
		//@{
	
		/** This function writes the current state of the job to disk.  
			See the section header to see how it does this.
			@see MWTask
		*/
	void checkpoint();
	
	
		/** This function does the inverse of checkpoint.  
			It opens the checkpoint file, calls read_master_state(), 
			then, for each task class in the file, creates a MWTask, 
			calls read_ckpt_info on it, and adds that class to the
			todo list. */
	void restart_from_ckpt();
	
	
		/** This function sets the frequency with with checkpoints are
			done.  It returns the former frequency value.  The default
			frequency is zero (no checkpoints).  If the frequency is n, 
			then a checkpoint will occur after the nth call to 
			act_on_completed_task().  A good place to set this is in
			get_userinfo().
			@param freq The frequency to set checkpoints to.
			@return The former frequency value.
		*/
	int set_checkpoint_frequency( int freq );
	
		/** Set a time-based frequency for checkpoints.  The time units
			are in seconds.  A value of 0 "turns off" time-based 
			checkpointing.  Time-based checkpointing cannot be "turned 
			on" unless the checkpoint_frequency is set to 0.  A good
			place to do this is in get_userinfo().
			@param secs Checkpoint every "secs" seconds
			@return The former time frequency value.
		*/
	int set_checkpoint_time( int secs );
	
		/** Here you write out all 'state' of the driver to fp.
			@param fp A file pointer that has been opened for writing. */
	virtual void write_master_state( FILE *fp ) {};
	
		/** Here, you read in the 'state' of the driver from fp.  Note
			that this is the reverse of write_master_state().
			@param fp A file pointer that has been opened for reading. */
	virtual void read_master_state( FILE *fp ) {};
	
		/** It's really annoying that the user has to do this, but
			they do.  The thing is, we have to make a new task of the
			user's derived type when we read in the checkpoint file.
			\begin{verbatim}
			    MWTask* gimme_a_task() {
			        return new <your derived task class>;
			    }
			\end{verbatim}
		*/
	virtual MWTask* gimme_a_task() { return NULL; };

		//@}

		/**   @name Benchmarking
			  We now have a user-defined benchmarking phase. */
		//@{
		/** register the task that will be used for benchmarking. */
	void register_benchmark_task( MWTask *t ) { bench_task = t; };

		/** get the benchmark task */
	MWTask * get_benchmark_task() { return bench_task; };
		//@}

 private:
	
		/** @name Main Internal Handling Routines */
		//@{

		/** This method is called before master_mainloop() is.  It does 
			some setup, including calling the get_userinfo() and 
			create_initial_tasks() methods.  It then figures out how 
			many machines it has and starts worker processes on them.
			@param argc The argc from the command line
			@param argv The argv from the command line
			@return This is the from the user's get_userinfo() routine.
			 If get_userinfo() returns OK, then the return value is from 
			 the user's setup_initial_tasks() function.
		*/
	MWReturn master_setup( int argc, char *argv[] );  
	
		/** This is the main controlling routine of the master.  It sits
			in a loop that accepts a message and then (in a big switch 
			statement) calls routines to deal with that message.  This loop
			ends when there are no jobs on either the running or todo queues.
			It is probably best to see the switch staement yourself to see
			which routines are called when. */
	MWReturn master_mainloop();      
	
		/**
		   unpacks the initial worker information, and sends the
		   application startup information (by calling pure virtual
		   {\tt pack_worker_init_data()}
		   
		   The return value is taken as the return value from the user's 
		   {\tt pack_worker_init_data()} function.
		   
		*/
	MWReturn worker_init( MWWorkerID *w );
	
		/** 
			This routine sets up the list of initial tasks to do on the 
			todo list.  In calls the pure virtual function 
			{\tt setup_initial_tasks()}.
			@return Is taken from the return value of
			{\tt setup_initial_tasks()}.
		*/
	MWReturn create_initial_tasks();  
	
		/**
		   Act on a "completed task" message from a worker.
		   Calls pure virtual function {\tt act_on_completed_task()}.
		   @return Is from the return value of {\tt act_on_completed_task()}.
		*/
	MWReturn handle_worker_results( MWWorkerID *w );
	
		/** We grab the next task off the todo list, make and send a work
			message, and send it to a worker.  That worker is marked as 
			"working" and has its runningtask pointer set to that task.  The
			worker pointer in the task is set to that worker.  The task
			is then placed on the running queue. */
	void send_task_to_worker( MWWorkerID *w );
	
		/**  After each result message is processed, we try to match up
			 tasks with workers.  (New tasks might have been added to the list
			 during processing of a message).  Don't send a task to
			 "nosend", since he just reported in.
		*/
	void rematch_tasks_to_workers( MWWorkerID *nosend );
	
		/** A wrapper around the lower level's hostaddlogic.  Handles
			things like counting machines and deleting surplus */
	void call_hostaddlogic();

		/// Kill all the workers
	void kill_workers();
	
		/** This is called in both handle_hostdelete and handle_taskexit.
			It removes the host from our records and cleans up 
			relevent pointers with the task it's running. */
	void hostPostmortem ( MWWorkerID *w );

		//@}
	
		/**@name Internal Task List Routines 
		   
		   These methods and data are responsible for managing the 
		   list of tasks to be done
		*/
	
		//@{
	
		/// This puts a (generally failed) task at the beginning of the list
	void pushTask( MWTask * );
	
		/// Get a Task.  
	MWTask *getNextTask();
	
		/// This puts a task at the end of the list
	void putOnRunQ( MWTask *t );
	
		/// Removes a task from the queue of things to run
	MWTask * rmFromRunQ ( int jobnum );
	
		/// Print the tasks in the list of tasks to do
	void printRunQ();
	
		/** Add one task to the todo list; do NOT set the 'number' of
			the task - useful in restarting from a checkpoint */
	void ckpt_addTask( MWTask * );
	
		/// returns the worker this task is assigned to, NULL if none.
	MWWorkerID * task_assigned( MWTask *t );
	
		/// Returns true if "t" is still in the todo list
	bool task_in_todo_list( MWTask *t );

#ifdef INDEPENDENT
		/** The control panel that controls the execution of the
		 independent mode. */
	void ControlPanel ( );	

	MWReturn master_mainloop_ind ( int buf_id, int sending_host);

	int outstanding_spawn;

#endif
	
		/** A pointer to a (user written) function that takes an MWTask
			and returns the "key" for this task.  The user is allowed to 
			change the "key" by simply changing the function
		*/
	MWKey (*task_key)( MWTask * );
	
		/// Where should tasks be added to the list?
	MWTaskAdditionMode addmode;
	
		/// Where should tasks by retrived from the list
	MWTaskRetrievalMode getmode;

  /** A pointer to the function that returns the "key" by which machines are ranked.
      Right now, we offer only some (hopefully useful) default functions that are 
      set through the machine_ordering_policy
   */

        MWKey (*worker_key)( MWWorkerID * );
	

        MWMachineOrderingPolicy machine_ordering_policy;

		/** MWDriver keeps a unique identifier for each task -- 
			here's the counter */
	int task_counter;
	
		/// Is the list sorted by the current key function
	bool listsorted;
	
		/// The head of the list of tasks to do
	MWTask *todo;

 public:
	/// This is Jeff's nasty addition so that he can get access
	///  to the tasks on the master
	MWTask *get_todo_head() { return todo; }
 private:
	
		/// The tail of the list of tasks to do
	MWTask *todoend;
	
		/// The head of the list of tasks that are actually running
	MWTask *running;
	
		/// The tail of the list of tasks that are actually running
	MWTask *runningend;
	
		//@}
	
		/**@name Worker management methods
		   
		   These methods act on the list of workers
		   (or specifically) ID's of workers, that the
		   driver knows about.
		   
		*/
	
		//@{
	
		/// Adds a worker to the list of avaiable workers
	void addWorker ( MWWorkerID *w );
	
		/// Looks up information about a worker given its task ID
	MWWorkerID * lookupWorker( int tid );
	
		/// Removes a worker from the list of available workers
	MWWorkerID * rmWorker ( int tid );

 public:	
		/// Prints the available workers
	void printWorkers();
 private:

		/// The head of the list of workers.
	MWWorkerID *workers;

		/// Here's where we store what should happen on a suspension...
	MWSuspensionPolicy suspensionPolicy;

  /// Based on the ordering policy, place w in the worker list appropriately
        void sort_worker_list();
public:
		/// Counts the existing workers
	int numWorkers();
	
		/// Counts the number of workers in the given arch class
	int numWorkers( int arch );

  /// Returns the value (only) of the best key in the Todo list 
  MWKey return_best_todo_keyval( void );

  /// Returns the best value (only) of the best key in the Running list.
  MWKey return_best_running_keyval( void );  

private:
	
		//@}
	
		/** @name Checkpoint internal helpers... */
		//@{

		/** How often to checkpoint?  Task frequency based. */
	int checkpoint_frequency;
	
		/** How often to checkpoint?  Time based. */
	int checkpoint_time_freq;
		/** Time to do next checkpoint...valid when using time-based 
			checkpointing. */
	long next_ckpt_time;
	
		/** The number of tasks acted upon up to now.  Used with 
			checkpoint_frequency */
	int num_completed_tasks;
	
		/** The benchmark task. */
	MWTask *bench_task;
	
		/** The name of the checkpoint file */
	const char *ckpt_filename; // JGS
	
		//@}

		/** The instance of the stats class that takes workers and later
			prints out relevant stats... */
	MWStatistics stats;
	
};

///  This is the function allowing the workers to be sorted by KFlops
MWKey kflops( MWWorkerID * );

/// This is the function allowing the workers to be sorted by benchmarkresult
MWKey benchmark_result( MWWorkerID * );


#endif


