//---------------------------------------------------------------------------
// Title    :  branching.hh
//
// Author   :  Qun Chen
//
// Purpose  :  branching algorithm class 
//
// History  :  When      Who      What
//             07/08/99  Qun      Created.
//             10/08/99  Jeff     Modified a little
//
// Copyright:  FATCOP group, UW-Madison
//---------------------------------------------------------------------------

#ifndef BRANCHING_HH
#define BRANCHING_HH


//#include "pseudocost.hh"
#include <lpsolver.h>
#include "darray.hh"

class CProblemSpec;
class PseudoCostManager;

class BranchManager
{

public:

  //------------------------------------------------------------------
  //               constructors and destructor
  //------------------------------------------------------------------

  BranchManager(CProblemSpec *form, int br, 
		PseudoCostManager *pcm,
		int *pri );

  virtual ~BranchManager();
  
  
  /** Main function.  

      Given a fractional solution xlp.  Determine on which variable
      to branch.  Also compute (estimates or lower bounds)
      on degradation of objective function value and
      estimate of the optimal solution at both nodes
  */      
  void select_a_var( double *xlp, double zlp, int &branchingIndex,
		    double &dndeg, double &updeg,
		    double &dnest, double &upest );
  

  /// Reset the branching counter.
  void setBranTime( double );

  /// Get the Time taken to branch
  double getBranTime( void );

  /// 
  void setMaxBranVars( int m );

  ///
  int getMaxBranVars();

  //------------------------------------------------------------------
  //               private data members and functions
  //------------------------------------------------------------------
private:

  /// The original problem formulation
  CProblemSpec* formulation;

  /** A Pseudocost manager (does strong branching and pseudocosts).
  */
  PseudoCostManager *pseudocosts;

  /// A priority order on the variables
  int* priority;

  ///  What is the maximum number of variables to consider for branching.
  int maxBranVars;

  /// How long has the branching rule taken?
  double branTime;

  /** Which branching rule?

      \begin{tabular}{cc} \\ \hline
      0 & pseudocost \\
      1 & First fractional \\
      2 & Most fractional \\
      3 & Least fractional \\
      \end{tabular}
   */

  int branchingRule;

  ///
  // PseudoCostManager *PseudoHandler;

  /**@name The branching rules.
   */

  //@{

  /// Use pseudocost information to branch
  void pseuCost( const dArray<int>&, const double xlp[], 
		 double zlp, int &branchingIndex,
		 double &dndeg, double &updeg, double &dnest, double &upest );
  
  /// Just branch on the first fractional.  A stupid rule.  {\tt :-)}
  void naturalOrder( const dArray<int>&, const double xlp[], 
		     double zlp, int &branchingIndex,
		     double &dndeg, double &updeg, 
		     double &dnest, double &upest );

  /// Branch on most fractional variable
  void maxIntInfea( const dArray<int>&, const double xlp[], 
		    double zlp, int &branchingIndex,
		    double &dndeg, double &updeg, 
		    double &dnest, double &upest );

  /// Branch on least fractional variable
  void minIntInfea( const dArray<int>&, const double xlp[], 
		    double zlp, int &branchingIndex,
		    double &dndeg, double &updeg, 
		    double &dnest, double &upest );
  
  
  
  //@}
  
  int findIndex(int);
  
};



#endif

  
