#ifndef LSHAPED_TASK_H
#define LSHAPED_TASK_H

#include "MWTask.h"

/** The task class specific our stochastic LP solver

*/

enum { FEASIBILITY_CUT, OPTIMALITY_CUT };

class LShapedTask : public MWTask 
{

public:
    
  /// Default Constructor
  LShapedTask();

  ///Construct with parameters.
    
  LShapedTask( int it, int beg, int end, int nc, 
	       const double xlp[], double theta, int tix );

  /// Default Destructor
  ~LShapedTask();

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

  /**@name Checkpointing functions.
   */
  //@{

  /// Write checkpoint info for the task
  void write_ckpt_info( FILE *fp );

  /// Read checkpoint info for the task
  void read_ckpt_info( FILE *fp );

  //@}

  /**@name Work portion.

    Here is the work portion of the Task
   */
  
  //@{
  
  /// What iteration was this cut for
  int iteration;
  
  /// Beginning index of scenarios for which to solve
  int startIx;  

  /// Ending index of scenarios for which to solve
  int endIx;

  /// Number of columns in "x"
  int mncols;

  /// Stage one solution "x"
  double *mxlp;

  /// The estimate of the second stage costs for this set of scenarios
  double theta;

  /// Theta's index in the multicut method.
  int thetaIx;

  //@}

  /**@name Result portion.

    Here is the result portion of the Task
   */

  //@{

  /// Solution time for the whole task
  double taskTime;  

  /// The number of cuts returned.
  int ncuts;

  /// The number of nonzeros in the cuts
  int cutnnzero;

  /** An array indicating whether the LP for a given scenario 
    was feasible or not -- whether you are returning a feasibility
    or optimality cut
    */
  int *cutType;

  //@{
  int *cutbeg;
  int *cutind;
  double *cutval;
  double *cutrhs;
  //@}

  /** The amount by which the current approximation to 2nd 
      stage costs is in error
  */
  double error;

  /** This is the amount of "second stage cost" that these
      scenarios add to the solution
  */
  double sscost;

  //@}

};

#endif

