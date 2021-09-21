#ifndef FATCOP_WORKER_HH
#define FATCOP_WORKER_HH

#include "MWWorker.h"
#include "FATCOP_task.hh"
#include "PRINO.hh"
#include <lpsolver.h>
#include "lp_minos.h" //should be in lpsolver.h
#include "branching.hh"


class CKnapsackCutGen;
class CFlowCutGen;
class CSKnapsackCutGen;
class BranchManager;
class PseudoCostManager;

class FATCOP_worker : public MWWorker
{
public:
  
  /// Default Constructor
  FATCOP_worker();
  
  /// Default Destructor
  ~FATCOP_worker();
  
  
  //@{
  /// Unpacks the "base" application data from the PVM buffer
  MWReturn unpack_init_data( void );
  
  /** Executes the given task.
      Remember to cast the task as type {\ttDummyTask}
  */
  void execute_task( MWTask * );

  /// Unpacks the best upper bound known by the driver.
  void unpack_driver_task_data( void );
  //@}


private:

  /// working formulation
  CProblemSpec *formulation;

  ///original formulation
  CProblemSpec *originalformulation;
  
  /// The Interface to the LP solver at this node
  CLPSolverInterface *lpsolver;

  /// Which LP solver should this worker use.
  int lpsolvertouse;

  /// Which type of lpsolver is being used.
  int activelpsolver;

  /** The CPLEX license directory.
   */
  char cplexLicenseDir[256];
  
  /// The "cutoff" value for this worker
  double bestUpperBound;

  /// When did the computation begin?
  clock_t startTick;

  /// The number of new and interesting cuts the worker finds.
  int numCuts;

  /// number of cuts a worker has when starting to process a node 
  int oldNumCuts;

  /// An array of pointers to the cuts of the worker
  CCut **cuts;
  
  /**@name Strategy Information.
   */

  //@{

  /// The branching strategy at this node
  int branStrategy;

  ///  How long does our branching strategy task (for this task)
  double branTime;

  /// How long is the (node processing) LP solve time on this task.
  double lpsolveTime;

  /// Do we generate knapsack cuts?
  int genKnapCut;

  /// Do we generate flow cover inequalities
  int genFlowCut;

  /// Do we generate surrogate knapsack inequalities
  int genSKnapCut;

  /// Number of attempts at generating cutting planes (on this task)
  int nCutTrial;

  /// Number of successful attempts to generate cutting planes (on this task)
  int nCutSuccess;

  /// Time taken for cut generation on this task
  double cutTime;

  /// What is the maximum size of the cut pool.
  int maxCuts;

  /// Do we do a diving heuristic at this node
  int divingHeuristic;

  //@{
  int divingHeuristicTrial;
  int divingHeuristicSuccess;
  double divingHeuristicTime;
  //@}

  /// Whether do node preprocessing or not
  int nodepreprocessing;

  /// What is the maximum time (in seconds) to spend at the Slave?
  double maxTaskTime;

  /// What are the maximum number of nodes to evaluate at the slave
  int maxTaskNodes;
    
  //@}


  /**@name Working Information.
   */

  //@{

  /// Objective function value of the working formulation
  double zlp;

  /// The LP solution of the working formulation
  double *xlp;

  /** The reduced costs of the working formulation
      (Used in some heuristics and lifting procedures.
  */
  double *dj;

  /// This is the Column statuses of the basis of the root LP.
  CBasisStatus *cstatRoot;

  /// This is the row statuses of the basis of the root LP.
  CBasisStatus *rstatRoot;

  /// The columns statuses of the last saved basis
  CBasisStatus *cstatSavedBasis;

  /// The row statuses of the last saved basis
  CBasisStatus *rstatSavedBasis;

  /// The number of rows in the last saved basis
  int nRowsSavedBasis;


  /// The number of nodes completed by this task
  int taskNodes;

  /// The number of LP pivots performed by this task.
  int numLPPivots;

  /// You should never have more than this many nodes in the stack
  int MAX_NODELISTSIZE;
  
  /// This is the stack of branches you haven't completed.
  MIPBranch *branches;

  /// The index into the branching stack
  int stackIndex;

  //@}


  /**@name Worker Methods and Classes.

     These are just some helpful functions that we use
     in order to carry out the worker tasks.
   */

  //@{

  /// A Knapsack Cut Generator
  CKnapsackCutGen *knapGen;

  /// A Flow Cover Generator
  CFlowCutGen *flowGen;

  /// A Surrogate Knapsack Cut Generator
  CSKnapsackCutGen *sKnapGen;

  /// The Branch Manager
  BranchManager *branchHandler;

  /// The PseudoCost Manager
  PseudoCostManager *pseudocosts;

  /** Process (fully) the current loaded LP.
      Return the LPStatus and whether or not the node was
      (integer) feasible.
   */
  void ProcessLoadedLP( CLPStatus &lpstat, bool &feasibleLP );
  

  /** Perform reduced cost fixing on the solution in xlp and the "working"
      formulation and return number of fixed vars
  */
  int ReducedCostFixing();
  
  /// Generate cuts -- return the number you generated
  int GenerateCuts();
  
  /// Push a branch on the depth first stack
  void pushNode( int branchingIndex, char whichBound, double oldval,
		 double val, int brotheropen, double lowerBound, double est );

  /** Pop a branch off the depth first stack.
      Updates stack AND the LP formulation
  */
  void popNode();
  
  /** Loads the cuts to the LP solver (all at once).
      (This seems to be important to SOPLEX
  */
  void LoadCutsToLPSolver( CLPSolverInterface *lps, int n, CCut **c );

  /** Load the bounds from node to the LP solver.
   */
  void LoadBoundsToLPSolver( MIPNode *node );


  /** Load the bounds from node to formulation.
   */
  void LoadBoundsToFormulation( MIPNode *node );
  

  /** Saves the current basis
   */
  void SaveCurrentBasis();

  /** Loads the root LP's basis back into the LP solver.
      (Augmented by slacks for cuts that may have been added).
   */
  void LoadRootBasisToLPSolver();

  /** Loads the last saved basis back into the LP solver.
      (Augmented by slacks for cuts that may have been added since)
   */
  void LoadSavedBasisToLPSolver();


  /**  Performs a diving heuristic
   */
  bool diving_heuristic( FATCOP_task * );


  //@}
  

};



#endif

