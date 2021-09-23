#ifndef MWWORKERID_H
#define MWWORKERID_H
#include "MW.h"
#include <string.h>
#include <sys/time.h>

#define NO_VAL -1

/// A forward reference; needed to define the MWWorkerID class.
class MWTask;

/**
   This is a list of the states in which a worker can be.
 */
enum MWworker_states { 
    /// Some initialization; not ready for work
    INITIALIZING, 
    /// Waiting for the master to send work
    IDLE, 
    /// Benchmarking, or doing application initialization on the initial data
    BENCHMARKING,
    /// Working actively on its task
    WORKING, 
    /// The machine has been suspended
    SUSPENDED,
    /// This worker has finished life.
    EXITED
};

/**
   This class keeps an identification of the worker application
   useful for the driver class.  It also keeps statistical information
   that is useful to the stats class.  This information will be used at
   the end of a run.
 */
class MWWorkerID  {

        // allow access to worker's data...makes my life easier.
    friend class MWStatistics;

public:

    /// Default constructor
    MWWorkerID();
    
    /// Default destructor
    virtual ~MWWorkerID();
    
		/// Return primary id of this worker
	int get_id1() { return id1; };

		/// Return secondard id of this worker
	int get_id2() { return id2; };

		/** Return the virtual id.  This is an int starting at zero and
			increasing by one with each worker.  When a worker dies, this
			number can be reused by a worker that joins us. */
	int get_vid() { return virtual_id; };

		/// Set primary id
	void set_id1( int i ) { id1 = i; };

		/// Set secondary id
	void set_id2( int i ) { id2 = i; };

    /// Set the machine's name
    void set_machine_name( char * );

    /// Get the current machine information
    void get_machine_info();

    /// Returns a pointer to the machine's name
    char *machine_name();
    
	/// Set this worker's arch class
	void set_arch ( int a ) { arch = a; };

	/// Get this worker's arch class
	int get_arch() { return arch; };

		/// Mark this worker so that when the task completes it will exit
	void mark_for_removal() { doomed = TRUE; };

		/// returns TRUE if mark_for_removal was called previously
	int is_doomed() { return doomed; };

		/// 
	void set_bench_result( double bres );

		///
	double get_bench_result() { return bench_result; };

    /// The task running on this worker.  NULL if there isn't one.
    MWTask *runningtask;
    
    /// The next worker ID in the list of available workers.  NULL otherwise
    MWWorkerID *next;

	/// The state of the worker.
	enum MWworker_states currentState();

	/// Set the state of the worker
	void setState ( MWworker_states st ) { state = st; };
    
		/** Print out a description of this worker.
			@param level The debug level to use */
    virtual void printself( int level = 40 ); 

    /**@name Statistics Collection Calls

       These methods are called when events happen to workers.
    */

    //@{

        /// Returns true if this worker is idle.
    bool isIdle() { return (state==IDLE); }

        /// Returns true if this worker is suspended.
    bool isSusp() { return (state==SUSPENDED); }

    /** This should be called when the master becomes aware of the 
        existence of this worker. */
    void started();
    /// Called when the worker is doing the benchmarking task
    void benchmark();
    /// Called when the worker has done the benchmarking task
    void benchmarkOver();
    /// Called when this worker gets a task.
    void gottask( int tasknum );
    /// Called when this worker just finished the task it was working on.
    void completedtask( double wall_time = 0.0, double cpu_time = 0.0 );
    /// Called when the worker becomes suspended
    void suspended();
    /// Called when the worker resumes.
    void resumed();
    /// Send flowers...this is called when the worker dies.
    void ended();

    //@}

    /**@name Checkpointing Call

	   Yes, each instance of a MWWorkerID needs to be able to checkpoint
	   itself.  Why?  It has to store statistics information on itself.
	   It never has to read them in, though, because stats that we write
	   out here wind up being read in by the stats class when we restart.
    */

    //@{

		/// Return the relevant stats info for this worker.
	void ckpt_stats( double *up, double *working, 
					 double *susp, double *cpu );

	//@}
	
		/** @name Worker Information
			This is now collected from condor_status.  It's the work of 
			one of JP's students... 
			
			Note that this Arch is different from arch.  This one is 
			what condor claims its arch is to the outside world. 
		*/
		//@{

  ///
    char Arch[64];

  ///
    char OpSys[64];

  ///
    double CondorLoadAvg;

  ///
    double LoadAvg;

  ///
    int Memory;

  ///
    int Cpus;

  ///
    int VirtualMemory;

  ///
    int Disk;

  ///
    int KFlops;

  ///
    int Mips;

 private:

    int check_for_int_val(char* name, char* key, char* value);
    double check_for_float_val(char* name, char* key, char* value);
    int check_for_string_val(char* name, char* key, char* value);

		//@}

		/** @name Private Data... */
		//@{
	
		/// The machine name of the worker
    char mach_name[64];
	
		/// A "primary" identifier of this machine.  (Was pvmd_tid)
    int id1;
    
		/// A "secondary" identifier of this machine.  (Was user_tid)
    int id2;
    
		/** A "virtual" number for this worker.  Assigned starting at 
			0 and working upwards by one.  Also, when a worker dies, 
			this number can get taken over by another worker */
	int virtual_id;

		/// This worker's arch class
	int arch;   // (should be just a 0, 1, 2, etc...)

		/// TRUE if marked for removal, FALSE otherwise	
	int doomed;  

		/// The results of the benchmarking process.
	double bench_result;

		/// The state of this worker.
    enum MWworker_states state;

		/** @name Time information 
		    Note that all time information is really stored as a double.
		    We use gettimeofday() internally, and convert the result to 
		    a double.  That way, we don't have to mess around with the
		    struct timeval... */
		//@{
		/// The time that this worker started.
	double start_time;
	
		/// The total time that this worker ran for.
	double total_time;

		/// The time of the last 'event'.
	double last_event;
	
		/// The time spent in the suspended state
	double total_suspended;

		/// The time spent working
	double total_working;

		/// The cpu usage while working
	double cpu_while_working;
		//@}
		//@}

	bool isBenchMark;

		/** A helper function...struct timeval->double. */
	double timeval_to_double( struct timeval t ) {
		return (double) t.tv_sec + ( (double) t.tv_usec * (double) 1e-6 );
	}

		/** @name Virtual Id Helpers */
		//@{
		/// Set virtual id
	void set_vid( int i ) { virtual_id = i; };
		/// vids[i] is 1 if virtual id i is taken, 0 if not.
	static int *vids;  /* Max of 1024 workers - enough for now! */
		/// Returns the lowest available virtual id; also sets it as used.
	int get_next_vid();
		/// Returns a virtual id to the pool
	void release_vid( int vid ) { vids[vid] = 0; };
		//@}

		/** @name A Special Printf
			Used to make output for John Bent's ulv program... */
		//@{
		/** The log printf - use it to print ONE log event.  It will 
			automagically add the "..." delimiter for you. */
	void lprintf ( int vid, double now, char *fmt, ... );
		/// The name of the ulv log file
	static char *ulv_filename;
		/// The log file fp.
	static FILE *lfp;
		//@}
};


#endif
