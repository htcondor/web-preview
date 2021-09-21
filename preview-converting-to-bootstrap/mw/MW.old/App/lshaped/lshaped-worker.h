#ifndef LSHAPED_WORKER_H
#define LSHAPER_WORKER_H

#include "MWWorker.h"
#include "lshaped-task.h"

#include <lpsolver.h>
#include <SPProblem.h>

/** The Worker class created for the SP example

*/

class LShapedWorker : public MWWorker 
{
  
public:
  /// Default Constructor
  LShapedWorker();

  /// Default Destructor
  ~LShapedWorker();

  /**@name Implemented methods.
     
     These are the ones that must be Implemented in order
     to create an application
  */
  //@{
  /// Unpacks the "base" application data from the PVM buffer
  void unpack_init_data( void );

  /// Unpacks the "worker" portion of any task data
  void unpack_driver_task_data( void );
  
  /** Executes the given task.
      Remember to cast the task as type {\ttLShapedTask}
  */
  void execute_task( MWTask * );
  //@}
    
private:

  /// Relative optimality tolerance
  double OPT_TOL;

  /// Holds the stochastic information and "scenario tree"
  SPProblem spprob;
  
  /// The Interface to a LP solver available on the driver
#if defined( SOPLEXv1 )
  CSOPLEXInterface sublpsolver;
#else
#error "There is no appropriate LP Solver defined!"  
#endif
  
  /**@name Subproblem LP formulation
   */

  //@{

  int sncols, snrows, snnzero, sos;
  double *sobj;
  double *srhs;
  double *smatval;
  double *sbdl;
  double *sbdu;
  char *ssense;
  int *smatbeg;
  int *smatind;  

  //@}

  /// Holds the new RHS
  double *snew_rhs;
  
  /// 
  int tsncols;

  /// 
  int tsnrows;

  int tsnnzero;

  double *tsobj;
  
  /**@name Subproblem solution.    
   */

  //@{
  double szlp;
  double *sxlp;
  double *spi;
  double *sdj;
  //@}

  /**@name The technology matrix T for the period.
   */

  //@{
  int tncols;
  int tnrows;
  int tnnzero;
  
  int *tmatbeg;
  int *tmatind;
  double *tmatval;

  //@}

  /// Changes the LP and arrays based on scenario
  int makeLPChange( CLPSolverInterface *lpsolver, ChangeType type,
		    int entry, double val, double *oldval,
		    double matval[], double rhs[]);

  /// Takes the "zeroes" out of the cut
  void sparsify_cut( int old_nz, int *new_nz, 
		     int ind[], double val[] );
  
};

#endif
