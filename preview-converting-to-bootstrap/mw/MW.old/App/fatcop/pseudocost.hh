//---------------------------------------------------------------------------
// Title    :  pseudocost.hh
//
// Author   :  Qun Chen
//
// Purpose  :  pseudocost class 
//
// History  :  When      Who      What
//             07/08/99  Qun      Created.
//             10/13/99  Jeff     Modified for MW.
//
// Copyright:  FATCOP group, UW-Madison
//---------------------------------------------------------------------------

#ifndef PSEUDOCOST_HH
#define PSEUDOCOST_HH

#include "PRINO.hh"
#include "darray.hh"

class CLPSolverInterface;

/**
   This class manages the pseudocosts.
   There is a pseudocost manager at each worker and also at the 
   driver.
 */

class PseudoCostManager
{

  /// For each integer constrainted variable, pCost records pseudocost info.
  struct pCost  
  {
    /// Index of variable in original LP
    int	idx;
    /// Up pseudocost
    double  costUp;
    /// Down pseudocost
    double  costDown;
    /// Number of up branches
    int nBranUp;  
    /// Number of down branches
    int nBranDown;

    /// An initializer
    pCost() {
      idx = -1;
      costUp = 0;
      costDown = 0;
      nBranUp = -1;
      nBranDown = -1;
    };
  
    /// An initializer
    pCost(int i,double cu, double cd, int nbu, int nbd) {
      idx = i;
      costUp = cu;
      costDown = cd;
      nBranUp = nbu;
      nBranDown = nbd;
    };
    
    void print(){
      cout<<"(x"<<idx<<", "<<nBranUp<<"  "<<costUp<<"  "
	  <<nBranDown<<" "<<costDown<<")"<<endl;
      return;
    };
    
    void clear(){
      costUp = 0;
      costDown = 0;
      nBranUp = 0;
      nBranDown = 0;    
    };
    
  };


public:
  
  /// Constructor (on the worker side)
  PseudoCostManager(CProblemSpec* form, CLPSolverInterface *lps,
		    int init_strategy );

  /// Constructor (for the master), where an LP solver is not needed.
  PseudoCostManager( CProblemSpec *form );

  /// Destructor
  ~PseudoCostManager();

  /** Update the pseudocost information from the
      last branch
  */
  void updateFromLP(const int& branchingIdx, const double& oldxlpval,
		    const double& parentLpVal,const double& lpVal,
		    const char& upOrdown );

  /** Merge the pseudocosts from the "new" array into the
      "working" array
   */
  void updateWorking( int ix, int nbu, double uv,
		      int nbd, double dv );
		   
  /**
     Given a list of candidate variables, and the LP solution
     xlp, determine a branching variable.
   */
  void selectVar( const dArray<int>& BranchingCandidates, 
		  const double xlp[], double zlp, int &branchingIndex,
		  double &dndeg, double &updeg );

  /** This uses pseudocosts to compute an estimate of the best
      solution obtainable from a node.

      XXX -- You probably need to pass in zlp when you figure out when to compute 
      this... 
  */
  
  void computeEstimates( const double xlp[], double zlp, int branchingIndex,
			 double &dnest, double &upest );

  /** This one just resets the values of all the "new" pseudocosts,
      so as not to send the same pseudocosts to the master more than once
  */
  void clearNew();

  /**@name Packing Routines.
   */

  //@{
  ///
  void MWPackWorking( MWRMComm * );
  ///
  void MWUnpackWorking( MWRMComm * );
  ///
  void MWPackNew( MWRMComm * );
  ///
  void MWUnpackNew( MWRMComm * );
  //@}

  
  /**@name Checkpointing Routines.
   */
  
  //@{
  ///
  void write( FILE * );
  ///
  void read( FILE * );
  //@}
  
  //
  void setInit( int t ) { way_of_ini = t; }   
  void setItlim( int t ) { itlim = t; }   
  
  //------------------------------------------------------------------
  //               private data member and functions
  //------------------------------------------------------------------
private:

  /** The "blending" of up and down degradations when choosing
      a branching variable.
   */  
  enum {coef1=5, coef2=1};

  /// The number of pivots to do to initialize pseudocosts
  int itlim;

  int way_of_ini; //0: use objective coefficient
                  //1: solve LP explicitly
                  //2: Compute all from scratch

  CProblemSpec* formulation;
  
  /// Array of length ncols containing the "working" pseudocosts
  int nWorkingPCosts;
  pCost *workingPCostArray;

  /// Array of length ncols containing the new pseudocosts
  int nNewPCosts;
  pCost *newPCostArray;

  CLPSolverInterface *lpsolver;
 
  /// Compute initial pseudocosts
  void computeFromScratch( const dArray<int>& iniArray, const double xlp[],
			   double zlp );
  
};

#endif

