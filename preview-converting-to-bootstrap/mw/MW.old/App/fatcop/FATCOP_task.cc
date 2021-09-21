#include <assert.h>
#include "MW.h"
#include "global.hh"
#include "FATCOP_task.hh"
#include "pseudocost.hh"

FATCOP_task::FATCOP_task()
{
#ifdef DEBUGMW
  cout<<" #### calling FATCOP_task constructor 1 "<<endl;
#endif

  startingNode = NULL;
  improvedSolution = 0;

  numVars = 0;
  feasibleSolution = NULL;

  numNodesCompleted = 0;
  CPUTime = 0.0;
  numLPPivots = 0;
  nCutTrial = 0;
  nCutSuccess = 0;
  cutTime = 0.0;
  divingHeuristicTrial = 0;
  divingHeuristicSuccess = 0;
  divingHeuristicTime = 0.0;
  
  branTime = 0.0;
  lpsolveTime = 0.0;

  stackDepth = 0;
  branchStack = NULL;
  backtracking = 0;
  numNewCuts = 0;
  newCuts = NULL;
  pseudocosts = NULL;

}


FATCOP_task::FATCOP_task( MIPNode *node, PseudoCostManager *pcm )
{
  startingNode = node;
  pseudocosts = pcm;

  improvedSolution = 0;
  numVars = 0;
  feasibleSolution = NULL;
  numNodesCompleted = 0;
  CPUTime = 0.0;
  numLPPivots = 0;
  nCutTrial = 0;
  nCutSuccess = 0;
  cutTime = 0.0;
  divingHeuristicTrial = 0;
  divingHeuristicSuccess = 0;
  divingHeuristicTime = 0.0;
  branTime = 0.0;
  lpsolveTime = 0.0;

  stackDepth = 0;
  backtracking = 0;
  branchStack = NULL;
  numNewCuts = 0;
  newCuts = NULL;  
}


FATCOP_task::~FATCOP_task()
{
  if( startingNode ) delete startingNode;
  if( branchStack ) delete [] branchStack;
  if( newCuts ) delete [] newCuts;

  if( feasibleSolution ) delete [] feasibleSolution;
  // Don't delete the pseudocosts, you just put the address of the
  //  master or worker PseudoCostManager here...
}


/// Pack the work for this task into the PVM buffer
void FATCOP_task::pack_work( void )
{
  
  startingNode->MWPack( RMC );
  return;
}


/// Unpack the work for this task from the PVM buffer
void FATCOP_task::unpack_work( void )
{
  
  startingNode = new MIPNode;
  startingNode->MWUnpack( RMC );

  return;
}


/// Pack the results from this task into the PVM buffer
void FATCOP_task::pack_results( void )
{
  RMC->pack( &improvedSolution, 1, 1 );
  if( improvedSolution )
    RMC->pack( &solutionValue, 1, 1 );

  RMC->pack( &numNodesCompleted, 1, 1 );  
  RMC->pack( &numLPPivots, 1, 1 );

  RMC->pack( &nCutTrial, 1, 1 );
  RMC->pack( &nCutSuccess, 1, 1 );
  RMC->pack( &cutTime, 1, 1 );

  RMC->pack( &divingHeuristicTrial, 1, 1 );
  RMC->pack( &divingHeuristicSuccess, 1, 1 );
  RMC->pack( &divingHeuristicTime, 1, 1 );
  RMC->pack( &branTime, 1, 1 );
  RMC->pack( &lpsolveTime, 1, 1 );

  RMC->pack( &CPUTime, 1, 1 );
    
  /* 
   * If we are generate lots of nodes on the workers, 
   * it is best to pass them in "compressed" format.
   * If we really go depth-first on the workers,
   * there is a "super compressed" format.  We just pass the stack of
   * branches.
   */
  
  RMC->pack( &stackDepth, 1, 1 );
  RMC->pack( &backtracking, 1, 1 );

  if( stackDepth > 0 )
    {
      double *bestLowerBoundArray = new double [stackDepth];
      double *bestEstimateArray = new double [stackDepth];

      int *stackidxArray = new int [stackDepth];
      char *stackwhichBoundArray = new char [stackDepth];
      double *stackvalArray = new double [stackDepth];
      int *stackbrotheropenArray = new int [stackDepth];

      // Pass stack
      for( int i = 0; i < stackDepth; i++ )
	{
	  bestLowerBoundArray[i] = branchStack[i].openLowerBound;
	  bestEstimateArray[i] = branchStack[i].openEst;
	  
	  stackidxArray[i] = branchStack[i].idx;
	  stackwhichBoundArray[i] = branchStack[i].whichBound;
	  stackvalArray[i] = branchStack[i].val;
	  stackbrotheropenArray[i] = branchStack[i].brotheropen;
	}

      RMC->pack( bestLowerBoundArray, stackDepth, 1 );
      RMC->pack( bestEstimateArray, stackDepth, 1 );

      RMC->pack( stackidxArray, stackDepth, 1 );
      RMC->pack( stackwhichBoundArray, stackDepth, 1 );
      RMC->pack( stackvalArray, stackDepth, 1 );
      RMC->pack( stackbrotheropenArray, stackDepth, 1 );

      delete [] bestLowerBoundArray;
      delete [] bestEstimateArray;
      delete [] stackidxArray; 
      delete [] stackwhichBoundArray;
      delete [] stackvalArray;
      delete [] stackbrotheropenArray;

    }


  RMC->pack( &numNewCuts, 1, 1 );
  for( int i = 0; i < numNewCuts; i++ )
    {
      //XXX Depending on how many cuts you're going to pass,
      //  You may want to optimize this as well.

      // Right now, let's just pass the cuts the simple way.
      newCuts[i]->MWPack( RMC );
    }

#if 0
  // Remember to delete the cuts -- they're already loaded into the LP.
  //but if we do nodepreprocessing, we then keep cuts in workers
  if( !nodepreprocessing ){
    for( int i = 0; i < numNewCuts; i++ )
      {
	delete newCuts[i];  
	newCuts[i] = 0;
      }
  }
#endif
  
  if( pseudocosts )
    {
      int newpseudo = 1;
      RMC->pack( &newpseudo, 1, 1 );
      pseudocosts->MWPackNew( RMC );
    }
  else
    {
      int newpseudo = 0;
      RMC->pack( &newpseudo, 1, 1 );
    }

  // Packing the pseudocosts "clears" them for later use.

  //pack best solution found
  if ( improvedSolution ){
    RMC->pack( &numVars, 1, 1 );
    RMC->pack( feasibleSolution, numVars, 1 );
  }
  
  return;
}
 
/// Unpack the results from this task into the PVM buffer
void FATCOP_task::unpack_results( void )
{
  RMC->unpack( &improvedSolution, 1, 1 );
  if( improvedSolution )
    RMC->unpack( &solutionValue, 1, 1 );


  RMC->unpack( &numNodesCompleted, 1, 1 );
  RMC->unpack( &numLPPivots, 1, 1 );

  RMC->unpack( &nCutTrial, 1, 1 );
  RMC->unpack( &nCutSuccess, 1, 1 );
  RMC->unpack( &cutTime, 1, 1 );

  RMC->unpack( &divingHeuristicTrial, 1, 1 );
  RMC->unpack( &divingHeuristicSuccess, 1, 1 );
  RMC->unpack( &divingHeuristicTime, 1, 1 );
  RMC->unpack( &branTime, 1, 1 );
  RMC->unpack( &lpsolveTime, 1, 1 );

  RMC->unpack( &CPUTime, 1, 1 );
  RMC->unpack( &stackDepth, 1, 1 );
  RMC->unpack( &backtracking, 1, 1 );
  
  if( stackDepth > 0 )
    {
      double *bestLowerBoundArray = new double [stackDepth];
      double *bestEstimateArray = new double [stackDepth];
      int *stackidxArray = new int [stackDepth];
      char *stackwhichBoundArray = new char [stackDepth];
      double *stackvalArray = new double [stackDepth];
      int *stackbrotheropenArray = new int [stackDepth];

      RMC->unpack( bestLowerBoundArray, stackDepth, 1 );
      RMC->unpack( bestEstimateArray, stackDepth, 1 );
      RMC->unpack( stackidxArray, stackDepth, 1 );
      RMC->unpack( stackwhichBoundArray, stackDepth, 1 );
      RMC->unpack( stackvalArray, stackDepth, 1 );
      RMC->unpack( stackbrotheropenArray, stackDepth, 1 );

      branchStack = new MIPBranch [stackDepth];
    
      for( int i = 0; i < stackDepth; i++ )
	{
	  branchStack[i].openLowerBound = bestLowerBoundArray[i];
	  branchStack[i].openEst = bestEstimateArray[i];
	  branchStack[i].idx = stackidxArray[i];
	  branchStack[i].whichBound = stackwhichBoundArray[i];
	  branchStack[i].val = stackvalArray[i];
	  branchStack[i].brotheropen = stackbrotheropenArray[i];
	}

      delete [] bestLowerBoundArray;
      delete [] bestEstimateArray;
      delete [] stackidxArray; 
      delete [] stackwhichBoundArray;
      delete [] stackvalArray;
      delete [] stackbrotheropenArray;

    }

  RMC->unpack( &numNewCuts, 1, 1 );
  newCuts = new CCut * [numNewCuts];
  for( int i = 0; i < numNewCuts; i++ )
    {
      newCuts[i] = new CCut;
      newCuts[i]->MWUnpack( RMC );
    }

  int newpseudo = 0;
  RMC->unpack( &newpseudo, 1, 1 );
  if( newpseudo )
    {
      // This function unpacks the new pseudocosts and updates the working
      //  pseudocost arrays
      pseudocosts->MWUnpackNew( RMC );
    }

  //### qun 
  //unpack best solution found
  if ( improvedSolution ){
    RMC->unpack( &numVars, 1, 1 );
    if( !feasibleSolution ) feasibleSolution = new double[numVars];
    RMC->unpack( feasibleSolution, numVars, 1 );
  }
  
  return;

}


void FATCOP_task::write_ckpt_info( FILE *fp )
{
  startingNode->write( fp );

  return;
}


void FATCOP_task::read_ckpt_info( FILE *fp )
{
  startingNode = new MIPNode;
  startingNode->read( fp );  
  return;
} 


