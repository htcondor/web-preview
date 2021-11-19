#ifndef LSHAPED_DRIVER_H
#define LSHAPED_DRIVER_H

#include "MWDriver.h"
#include "lshaped-task.h"
#include "lshaped-driverstats.h"

#include <lpsolver.h>
#include <SPProblem.h>
#include <fstream.h>

/**
   How should the "chunk size be determined"
*/
enum MWLShapedChunkSizeType
{
  TARGET_TIME,
  PERCENTAGE_OF_SCENARIOS,
  EXPLICIT_NUMBER_OF_SCENARIOS,
  // XXX Dynamic not implemented
  DYNAMIC 
};

/**
   How do you wish to checkpoint?
*/
enum MWLShapedCPType
{
  TIME,
  ITERATIONS
};


/** The driver class derived from the MWDriver class for the L-shaped method.

    @author Jeff Linderoth
*/
class LShapedDriver : public MWDriver 
{
public:
  /// Constructor
  LShapedDriver();
  /// Destructor
  ~LShapedDriver();

  /**@name Implemented Methods
    
    These methods are the ones that *must* be implemented in order
    to create an application
    
    */

  //@{
  /// Get the info from the user.  Don't forget to get the 
  /// worker_executable!
  MWReturn get_userinfo( int argc, char *argv[] );

  /// Set up an array of tasks here
  MWReturn setup_initial_tasks( int *, MWTask *** );

  /// What to do when a task finishes:
  MWReturn act_on_completed_task( MWTask * );

  /// Put things in the send buffer here that go to a worker
  MWReturn pack_worker_init_data( void );

  /// For packing the "worker" portion of the task data.
  // (Mine is empty)
  // void pack_driver_task_data( void );

  /// OK, this one doesn't *have* to be...but you want to be able to
  /// tell the world the results, don't you? :-)
  void printresults();

  /// For checkpointing
  void write_master_state( FILE *fp );

  /// For checkpointing
  void read_master_state( FILE *fp );

  /// For checkpointing
  MWTask *gimme_a_task();


  //@}



private:

  /**@name Configuration.

     Parameters controlling behavior, instance file names and such
   */

  struct Config
  {
    /// Sets all the defaults
    Config();
    /// An (empty) destructor
    ~Config();

    /**
       The number of different architectures.  (16 arches max)

       The order in which the arches appear in the input file, must match the
       order in which they appear in the submission file.
    */
    int num_arches;

    /** This controls the synchronicity of the algorithm.
	A new iteration will start whenever this percentage
	of "chunks" report back with cuts.
    */
    double start_new_it_percent;

    /** This controls how the driver breaks up the scenarios
	into tasks.  There are currently three options

	\begin{itemize}
	\item TARGET_TIME -- Based on the solution of the initial
	LP relaxation, the driver attempts to give each worker a
	number of scenarios that will have it work for a specific
	amount of time.  Each worker is given THE SAME number of
	scenarios (right now).
	\item PERCENTAGE_OF_SCENARIOS -- Each tasks consists of the
	given percentage of the total number of scenarios
	\item EXPLICIT_NUMBER_OF_SCENARIOS -- Each task consists 
	of exactly this number of scenarios (except the "last" task
	in a work cycle
	\end{itemize}

	Someday, there will be an option...
	\begin{itemize}
	\item DYNAMIC -- The chunk size will vary depending on the
	the number of available resources or the convergence 
	behvior exhibited so far
	\end{itemize}

    */

    MWLShapedChunkSizeType chunk_size_type;

    /** The target chunk time (in seconds) 
	if chunk_size_type == TARGET_TIME */
    double target_chunk_size_time;

    /// The chunk size if chunk_size_type == PERCENTAGE_OF_SCENARIOS
    double percent_scenarios_per_chunk;

    /// The chunk size if chunk_size_type == EXPLICIT_NUMBER_OF_SCENARIOS
    int explicit_chunk_size;


    /** To what tolerance should the relative upper and 
	lower bounds converge */
    double convergeTol;

    /** How many consecutive iterations must a cut be inactive before it is
	deleted?
    */
    int cutDelBnd;
        
    /// Maximum number of Master Iterations
    static const int MAX_ITERATIONS = 5000;

    /// The maximum number of cuts to load to the LP at one time
    int maxCuts;
    
    /**@name Instance information.
     */
    
    //@{
    
    char input_file[256];
    
    char core_filename[256];
    char time_filename[256];
    char stoch_filename[256];

    /// Not implemented yet
    int sample_input;

    /// Not implemented yet
    int sample_size;

    //@}

    /**@name Checkpoint information.

       checkpoint_type is one of either TIME or ITERATIONS
     */
    MWLShapedCPType checkpoint_type;

    //@{
    /// How many iterations between checkpoints if checkpoint_type == ITERATIONS
    int it_checkpoint_frequency;

    /// Time (in seconds) between checkpoints id checkpoint_type == TIME
    double time_checkpoint_frequency;

    //@}

    /// The "debugging level" for the MWprintf() function.
    int debug_level;

    /** The "target" number of hosts.  
	If {\tt target\_num\_hosts == -1}, get as many as possible.  
	If {\tt target\_num\_hosts == -2}, set to the maximum number
	that you will ever likely need	
    */
    int target_num_hosts;
	
    
  } config;

  /// Reads the configuration file
  bool read_input( FILE * );

  /// The iteration log.
  ofstream itlog;

  /// A class that holds the Stochastic Programming Problem information
  SPProblem spprob;
  
  /// The Interface to a LP solver available on the driver
#if defined( SOPLEXv1 )
  CSOPLEXInterface masterlpsolver;
#else
#error "There is no appropriate LP Solver defined!"  
#endif
  
  /**@name Master LP representation
   */

  //@{
  
  // tmncols == ncols + 1 (The last column will be theta)
  int mncols;
  int tmncols;
  int mnrows;
  int mnnzero;
  int mos;
  double *mobj;
  double *mrhs;
  double *mmatval;
  double *mbdl;
  double *mbdu;
  char *msense;
  int *mmatbeg;
  int *mmatind;
  
  //@}

  /**@name Master LP solution.
   */

  //@{

  double mzlp;
  double *mxlp;
  /// Used to measure norm of last step length
  double *oldmxlp;
  double *mpi;

  //@}

  /**@name Cut Buffer Data Structures.
   */

  //@{

  /// Number of cuts in the cut buffer
  int cutbufn;

  ///
  int cutbufmaxn;

  /// 
  int cutbufnz;

  /// 
  int cutbufmaxnz;

  ///
  int *cutbufmatbeg;

  ///
  int *cutbufmatind;

  /// 
  double *cutbufmatval;

  /// 
  char *cutbufsense;

  ///
  double *cutbufrhs;

  /// Writes the cutbuffer
  void write_cutbuffer( FILE *fp );

  /// Reads the cutbuffer
  void read_cutbuffer( FILE *fp );

  //@}

  /**@name Master control methods and Data.
   */

  //@{

  /// Have we converged?
  bool converged;

  /// Structure to hold information about iterations
  struct isstruct
  {

    /// Constructor
    isstruct();

    /// Destructor
    ~isstruct();

    /// Number of work chunks completed for this iteration
    int numChunksCompleted;

    /// Number of work chunks in this iteration
    int numChunksPerIt;

    /// What was the first stage cost of this solution
    double fscost;

    /// What are the sum of the second stage costs of this solution
    double sscost;

    /// Total error in second stage approximation
    double error;

    /// Is the first stage solution feasible?
    bool feasible;

    /// Number of cuts found on this iteration
    int numCuts;

    /// Start a new iteration
    void startNew( int tpi, double fsc );

    /** Update iteration with task information.  
	Returns true if iteration is completed
    */
    bool update( LShapedTask *lst );

  };

  struct isstruct *iterationStatus;

  /// Writes the iterations status
  void write_iterationStatus( FILE *fp );

  /// Reads the iteration status
  void read_iterationStatus( FILE *fp );

  /** The maximum number of "scenario clumps".
      (It's the maximum number of extra columns -- ever).
  */
  int maxNumTheta;

  /// (Current) number of tasks per iteration
  int tasksPerIt;

  /// (Current) number of scenarios assigned to a task  
  int chunkSize;

  /** Returns the current number of scenarios that are in a task "chunk"
      (Right now simply returns {\tt chunkSize} */
  int numScenariosPerTask();

  /** A method to set the number of scenarios to include in a task.  
      It also sets the variable {\tt tasksPerIt}

      XXX For the dynamic method, this should also update the LP?
   */
  void setNumScenariosPerTask( int );

  /// Current "master" iteration
  int iteration;

  /// The value of a feasible solution to the problem (best upper bound)
  double feasibleSolutionValue;

  /// The lower bound (solution to the master LP)
  double lowerBound;

  /** This is to fix a "bug" in MWDriver about tasks getting rescheduled      
      (I think this has been resolved)
   */
  int iterationFirstTaskIx;
  int iterationLastTaskIx;

  /** Process any cuts in the returned task.  
      Adds them to the cut buffer and updates theta's bounds if necssary 
  */
  void processCuts( LShapedTask *lst );

  /// Should we start another iteration
  bool startNewIteration( int currentIt, int itJustReported );

  //@}

  /**@name Cut cleanup methods and data. 
   */

  //@{

  /** A counter for the number of consecutive rounds that a cut has
      been inactive.
   */
  int *slackCounter;

  /// The method that deletes cuts
  CLPStatus DeleteSomeRows();

  /** A counter for the maximum number of cuts ever in the LP
   */
  
  int maxNumCuts;

  /** Update the slackCounter array after deleting some rows.
      This will only work with SOPLEX!  The array specifies how the
      rows have been permuted upon the last deletion.
   */
  
  void PushSlackCounter( int, int [] );

  //@}

  /** This solves the "expected value equivalent" to get a good starting
      point for the procedure
  */
  CLPStatus Crash();

  /// A method to try to "clean up" the LP to fix numerical issues.
  CLPStatus ResolveNumericalIssues();

  /// A class to keep track of some stats.
  LShapedDriverStats lshapedStats;

};

#if defined( TEST_TASKLIST_METHODS )
/**@name Task List Methods.

   These global methods that return the various "keys" of the tasks
   By which the driver wants to "rank" them.
*/

//@{

MWKey taskKey1( MWTask * );
MWKey taskKey2( MWTask * );
MWKey taskKey3( MWTask * );

//@}
#endif

#endif
