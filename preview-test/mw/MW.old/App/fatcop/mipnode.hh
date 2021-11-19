//---------------------------------------------------------------------------
// Title    :  mipnode.hh
//
// Author   :  Jeff Linderoth
//
// Purpose  :  Defines a node of the search tree 
//
// History  :  When      Who      What
//             10/05/99  Qun/Jeff Created.
//
// Copyright:  FATCOP group, UW-Madison and ANL/MCS
//---------------------------------------------------------------------------


#ifndef MIPNODE_HH
#define MIPNODE_HH
#include <iostream.h>
#include <stdio.h>

class MWRMComm;

class MIPNode
{
public:
  MIPNode();
  virtual ~MIPNode();

  /// Depth of this node in the branch and bound tree
  int depth;

  /** A lower bound on the optimal solution obtainable from this node.
      (Usually the parents LP value)
  */
  double bestLowerBound;

  /** An estimate on the optimal solution obtainable from this node
   */
  double bestEstimate;

  /// The number of bounds defining this node
  int numBounds;

  /// Indices
  int *boundIx;

  /// 'L' if the lower bound is set 'U' if the upper bound is set
  char *whichBound;

  /// Array holding the values to which the bounds are set
  double *boundVal;

  /// Pack one node.
  void MWPack( MWRMComm * );

  /// Unpack one node.
  void MWUnpack( MWRMComm * );

  friend ostream& operator<< (ostream&, const MIPNode&);  

  /** Checkpointing
   */
  ///
  void write( FILE * );
  ///
  void read( FILE * );

};


/** This is the structure for holding the changes made to the active LP
    formulation.
 */
struct MIPBranch
{ 
  /// which integer varible.
  int   idx;
  /// Which bound has been changed -- 'L' or 'U'
  char   whichBound;  
  /// The old value of the bound (when popping off the stack).
  double oldval;      
  /// value of LP solution.
  double val;         
  /// Is the node's "brother" open?
  int brotheropen;   
  
  /// Lower bound of brother (if open)
  double openLowerBound;

  /// Estimate of best solution at from brother (if open)
  double openEst;

  MIPBranch()
    { idx=-1; whichBound = 'X'; oldval=0.0; val=0.0; brotheropen=1;
    openLowerBound=0.0; openEst=0.0; }
    
  MIPBranch(int i, char w, double ov, double v, int bo,
	    double olb, double oe )
    { idx=i; whichBound = w; oldval = ov; val = v; brotheropen = bo;
    openLowerBound = olb; openEst = oe; }
  
  void printBranch() const{
    if(whichBound=='U'){
      cout<<"x"<<idx;
      cout<<"<=";
      cout<< ( (double) ( (int) val ) ) <<endl;
    }
    else if(whichBound=='L'){
      cout<<"x"<<idx;
      cout<<">=";
      cout<< ( (double) ( (int) val + 1.0 ) ) <<endl;
    }
  };
    
} ;

#endif


