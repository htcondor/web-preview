#ifndef FATCOP_DRIVER_HH
#define FATCOP_DRIVER_HH

#include "MW.h"
#include "MWDriver.h"
#include <lpsolver.h>
#include "PRINO.hh"
#include "mipnode.hh"

// Forward declaration
class cutPool;
class PseudoCostManager;

class FATCOP_driver : public MWDriver
{
public:

  /// The status that is passed back to GAMS?
  enum   Status
  {
    // lp solver failure     
    LPSOLVER_ERROR = 10,
    /// loaded problem is unbounded. 
    UNBOUNDED,
    // loaded problem is infeasible. 
    INFEASIBLE,
    // no integer solution. 
    INT_INFEASIBLE,
    // int solution found, but not proved optimal due to node limit. 
    INT_SOLUTION,
    // no int solution found, program stop due to node limit
    NO_INT_SOLUTION,
    // loaded problem has been solved with a proved optimal
    SOLVED,
    // memory used up. 
    MEMORY_ERROR,
    //error loading problem
    // LOAD_ERROR
    //uninitialized
    UNINITIALIZED
  } ;

  //constructors and destructor
  FATCOP_driver();
  ~FATCOP_driver();
  
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
  
  /// OK, this one doesn't *have* to be...but you want to be able to
  /// tell the world the results, don't you? :-)
  void printresults();

  /// Packs up the best upper bound
  void pack_driver_task_data( void );
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

  // ###qun
  /** Set options*/
  void setR( double optcRel ){ relativeGap = optcRel; return; } 
  void setA( double optcAct ){ absoluteGap = optcAct; return; } 
  void setNodelim( int nodlim ){ maxNodes = nodlim; return; } 
  void setReslim( double reslim ){ maxTime = reslim; return; }

  /** inquiry results: */
  Status status(){ return st; };
  double value() { 
    if(revobj) return -bestUpperBound;
    return bestUpperBound;
  };
  int nodes( )   { return nNodes; };
  int pivots( )  { return lpPivots; };
  double solutime( ){ return solutionTime;};
  
  double getBestPos() { return bestUpperBound - getStopTol();};
  double getFinalGap(){ return getStopTol()/bestUpperBound;};

#if defined( GAMS_INTERFACE )
  void   getResults( double *zlp, double xlp[], 
		     double pi[], double slack[], double dj[],
		     int cs[], int rs[] ){
    //load the best solution
    if ( sol_lb && sol_ub ){
      lpsolver->LPSetAllBdl( sol_lb, formulation->get_ncols() );
      lpsolver->LPSetAllBdu( sol_ub, formulation->get_ncols() );
    }
   
    //if cuts were generated
    if(lpsolver->LPGetActiveRows()>formulation->get_nrows()){
      
      double* npi    = new double[lpsolver->LPGetActiveRows()];
      double* nslack = new double[lpsolver->LPGetActiveRows()];
      
      
      assert (LP_OPTIMAL == lpsolver->LPSolvePrimal( zlp, xlp,
						     npi, nslack, dj) );
    
      for(int i = 0; i<formulation->get_nrows(); i++){
	pi[i] = npi[i];
	slack[i] = nslack[i];
      }
    
      delete [] npi;
      delete [] nslack;
    }
    else{
      assert (LP_OPTIMAL == lpsolver->LPSolvePrimal( zlp, xlp,
						     pi, slack, dj) );
    }
    return;
  }
#endif  
	
  //@} 




private:

  /// A counter of the number of tasks that have reported
  int nTasksReported;


  /**@name MIP Options.
   */

  //@{

  /** Node selection strategy on the master.

  default 0: best estimate 1: depth first, 2:best bound, 3: deepest node first
  4: hybrid of best bound and deepest node first

  */
  int seleStrategy;   


  /** For hybrid strategy.  If the number of tasks is more than TASK_HI_THRESHOLD,
      the program switches node selection rule to deepest first.  The program
      will switch back after the number of tasks is less than TASK_LOW_THRESHOLD;
  */
  int TASK_HI_THRESHOLD;
  int TASK_LOW_THRESHOLD;

  int current_seleStrategy;
       
  /** The Branching Strategy 
      default 0: pseudocost
      1: natural order, 
      2: max int infea
      3: min Int Infea
   */
  int branStrategy;

  ///
  double branTime;

  double lpsolveTime;

  /**
     What presolve level do we use AT THE ROOT!
     0: no presolve, 1: simple presolve, 2:advanced presolve
   */
  int presolLev;    

  /** 
      Should we generate cuts?
      This can be altered dynamically.
  */
  //@{
  int genKnapCut;
  int genFlowCut;
  int genSKnapCut;

  int nCutTrial;
  int nCutSuccess;
  double cutTime;
  int lastCutSuccess;

  //@}


  /**
     What is the maximum number of cuts that is ever to be in *any* cutpool.
     (Jeff) -- Is this right?
   */
  int maxCuts;

  /** do node preprocessing?
      This is dynamically configurable through the course of the run.
  */

  //@{

  int nodepreprocessing;
  
  int preprocTrial;
  int preprocSuccess;

  double preprocTime;

  //@}
  
  /**
     To what absolute gap should the problem be solved?
   */
  double absoluteGap;

  /**
     To what relative gap should the problem be solved?
   */
  double relativeGap;

  /** A function that returns the stopping tolerance,
      We don't care to find a solution this (absolute) amount more.
   */
  double getStopTol( void );
    
  /**
     If using pseduocosts, how should they be initialized?
     0: generate initial pseudocost using objective coefficients
     1: generate initial pseudocost by solving LPs explicitly
     1: generate all fractional variables from scratch
  */
  int iniPseuCost; 

  /// A limit to the number of iterations to perform in strong branching
  int pseudoItlim;

  /// A limit on the number of variables to compute pseudocosts for
  int pseudoVarlim;

  //@{

  /// Perform the diving heuristic?
  int divingHeuristic;  

  /// What does thse user want you to do?
  int originalDivingHeuristic;

  int divingHeuristicTrial;
  int divingHeuristicSuccess;
  double divingHeuristicTime;
  //@}

  //whether perform heuristics before bb
  int preHeuristics; 

  /**
     What "type" of checkpointing?
       0 -- None
       1 -- By nodes
       2 -- By time
   */
  int checkpointType;

  /** What is the checkpoint frequency
   */  
  int checkpointFrequency;

  /** After how many nodes should FATCOP write its status to the screen? */
  int logFrequency;

  /// What is the maximum number of nodes we wish to explore
  int maxNodes;
  
  /// What is the maximum time we wish to explore?
  double maxTime;

  /// This is to keep track of the (running) wall clock time across checkpoints.
  double cumulativeTime;

  /// A "tick" to keep track of the time
  time_t start_time;

  /// What is the maximum number of nodes allowed in the pool before we stop
  double maxPoolMem;

  /// What is the target number of workers
  int maxWorkers;

  /// What is the "chunk" size time in the slave?
  double maxTimeInSlave;

  /// What would the user like the chunk time to be
  double userMaxTimeInSlave;

  /// What is the maximum number of nodes to evaluate on a slave
  int maxNodeInSlave;

  /// What would the user like the maximum number of nodes to be
  int userMaxNodeInSlave;

  /// What is the number of architectures that you are running on?
  int t_num_arches;

  /// This function sets an option.
  void setOption( char *s );
  
  //@}

  /**@name IO Functions.
   */

  //@{

  /// Load the problem from an MPS file
  void load(char*); 

  /// Load the problem from a matrix
  void load(int objsense, int n, int m, int nz, 
	    int*& matbeg, int*& matind, int*& matcnt, double*& matval, 
	    CColType*& ctype, double*& obj, double*& lb, double*& ub, 
	    double*& rhsx, char*& senx );

  //@}

  /// Read the option file.
  void readOption(char* = "fatcop.opt");

  /**@name LPSolver Management
   */

  //@{
  /// alternative lpsolver
  ///If use up CPLEX license, the program can use either SOPLEX 
  ///or MINOS as alternative solver
  int altlpsolver;

  /// An LP Solver for solving the root LP.
  CLPSolverInterface *lpsolver;

  /// The number of cplex licenses available at a site.
  //XXX  (If many sites participating, this needs thought...
  int numCplexLicenses;

  /// The number of CPLEX licenses to leave open
  int numCplexLeaveOpen;

  /// number of cplex copies used by this job on workers
  int mycplexcopies;

  /// max number of cplex copies allowed to be used by workers
  int maxcplexcopies;
  
  /// What is the cplex pointer file?
  char cplexPointerFile[256];

  /// Where is the Cplex License directory
  char cplexLicenseDir[256];

  //@}

  /// The original formulation after preprocessing
  CProblemSpec *formulation;

  /// A pointer to the cutpool
  cutPool *cPool;

  /// A pointer to the master's pseudocost manager...
  PseudoCostManager *pseudocosts;

  /** A method that creates nodes given a starting point
      and a depth first stack
   */
  void createNodesFromStack( MIPNode ** &newNodes,
			     int &numNewNodes,
			     const MIPNode *startNode,
			     int stackDepth,
			     const MIPBranch *branchStack,
			     int backtracking );

  
  //XXX What to do about the pseudocosts...


  /**@name Variables holding status of solution progress
   */

  //@{

  /// Value of the best solution found so far
  double bestUpperBound;

  /// Returns the value of the best lower bound.
  double getBestLowerBoundVal();

  /// The total number of nodes explored.
  int nNodes;

  /// The number of nodes the last time information was written to the screen.
  int lastlognNodes;

  /// The total number of LP pivots performed.
  int lpPivots;

  /// The sum total of the CPU times on the workers...
  double solutionTime;

  
  //@}


  /**@name Root LP solution information.

     We store and keep this information so that we can do reduced
     cost fixing with respect to the root whenever we get a new solution.
   */

  //@{

  /// Objective function value at root
  double zlpRoot;

  /// Array containing primal solution at root
  double *xlpRoot;

  /// Array containing reduced costs at root
  double *djRoot;

  /// Array containing column basis status at root
  CBasisStatus *cstatRoot;

  /// Array containing row basis status at root
  CBasisStatus *rstatRoot;

  //@}

  // ###qun
  
  /// Required for gams interface
  /// MIP solve status
  Status st;
  
  ///MIP solution

  double *feasibleSolution;

#if 0
  double* sol_lb;
  double* sol_ub;
#endif
  
  ///objective sense reversed?
  int revobj; //1: yes, 0:no


};

#endif
  
  
  
