#ifndef FATCOP_TASK_HH
#define FATCOP_TASK_HH

#include "MWTask.h"

#include "mipnode.hh"
#include "cuts.hh"

class PseudoCostManager;

class FATCOP_task : public MWTask
{
public:

  FATCOP_task();  
  FATCOP_task( MIPNode *startnode, PseudoCostManager *cpm );
  ~FATCOP_task();
  
  // FATCOP_task& operator = ( const FATCOP_task& );
  
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
  
  /**@name Checkpointing Implementation 
     These members used when checkpointing. */
  //@{
  /// Write state
  void write_ckpt_info( FILE *fp );
  /// Read state
  void read_ckpt_info( FILE *fp );
  //@}
  
  /**@name The "work" portion of a task consists of a 
      "limit" and a set of bounds.  Everything else
      is passed at initialization.
  */

  /// The starting MIP for the subtree rooted at this node.
  MIPNode *startingNode;

  /* I'm not sure what to do about the Basis, but if you
     wish, and you know one, you could send a goood basis here
  */

  //@}

  /**@name The Result portion of the task.
   */

  //@{
  /// Was an improved solution found?
  int improvedSolution;

  /** Solution Value.  Right now we don't care about the ACTUAL solution,
      but if you did, you would pass it back here.
  */
  double solutionValue;

  /// Redundant, -- this is the size of the solution vector
  int numVars;

  /// The primal feasible solution value
  double *feasibleSolution;

  /// Number of nodes completed by this task
  int numNodesCompleted;

  /// CPU time used in completing this task
  double CPUTime;

  /** Number of LP pivots done by this task.  (We should include
      strong branching pivots too
  */
  int numLPPivots;


  /** Cutting plane success information.
   */
  //@{
  int nCutTrial;
  int nCutSuccess;
  double cutTime;
  //@}

  //@{
  int divingHeuristicTrial;
  int divingHeuristicSuccess;
  double divingHeuristicTime;
  //@}

  ///
  double branTime;

  /// 
  double lpsolveTime;

  /** Stack of branches.

      Since you are ALWAYS going to go depth first on the
      workers, then you can greatly reduce the amount of information
      that you pass back.  Pass the tree in "compressed" form.
      Just pass the stack of branches, and "act_on_results"
      must create the new nodes. 

      XXX bandwidth versus CPU time -- Perhaps the workers
          should compute the actual nodes nad pass them back
	  in "whole" form...

   */

  MIPBranch *branchStack;

  /// How deep is the stack?
  int stackDepth;

  /// Were you backtracking upon completion?
  int backtracking;

  /// Number of new and important cuts found by this task
  int numNewCuts;

  /// Array of pointers to new and Important Cuts
  CCut **newCuts;

  /** A pointer to the pseudocost manager, so that the
      new pseudocosts can be passed from the worker and
      unpacked by the master if necessary.

      Note -- Be very careful when starting from a checkpoint.
      This address will not be valid (or even written) on the master,
      so tasks that are restarted after checkpoint either lose their
      pseudocosts, or you need to "seed" the restarted tasks with the address
      of the new pseudocost manager.

      Right now, I have not seeded the restarted tasks, so this information 
      is lost.
   */
  PseudoCostManager *pseudocosts;
  
};

#endif
