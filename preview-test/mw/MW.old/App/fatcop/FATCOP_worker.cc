#include "MW.h"
#include "global.hh"
#include "FATCOP_worker.hh"
#include "FATCOP_task.hh"
#include <lpsolver.h>
#include "cutgen.hh"
#include "branching.hh"
#include "pseudocost.hh"

extern int MWprintf_level;

/// Default Constructor
FATCOP_worker::FATCOP_worker()
{  
#ifdef DEBUGMW
  cout<<" &&&& calling FATCOP_worker constructor"<<endl;
#endif

  workingTask = new FATCOP_task;
  formulation = new CProblemSpec;
  originalformulation = NULL;

  // The rest of this memory is allocated in the worker routines
  lpsolver = NULL;
  lpsolvertouse = -1;
  activelpsolver = UNDEFINEDLPSOLVER;
  bestUpperBound = DBL_MAX;
  oldNumCuts = 0;
  numCuts = 0;
  cuts = NULL;

  branStrategy = 1;
  branTime = 0.0;
  genKnapCut = 1;
  genFlowCut = 1;
  genSKnapCut = 1;
  maxCuts = 1;
  divingHeuristic = 0;
  maxTaskTime = DBL_MAX;
  maxTaskNodes = INT_MAX;
  nodepreprocessing = 0;
  knapGen = NULL;
  flowGen = NULL;
  branchHandler = NULL;
  pseudocosts = NULL;
  
  zlp = -DBL_MAX;
  xlp = NULL;
  dj = NULL;

  cstatRoot = NULL;
  rstatRoot = NULL;

  cstatSavedBasis = NULL;
  rstatSavedBasis = NULL;
  nRowsSavedBasis = -1;

  stackIndex = 0;
  MAX_NODELISTSIZE = 2000;
  branches = new MIPBranch [MAX_NODELISTSIZE];

}



/// Default Destructor
FATCOP_worker::~FATCOP_worker(){
#ifdef DEBUGMW
  cout<<" &&&& calling FATCOP_worker destructor"<<endl;
#endif
  
  if(workingTask) delete workingTask;
  delete formulation;
  if( originalformulation ) delete originalformulation;

  delete lpsolver;

  delete [] xlp;
  delete [] dj;

  delete [] cstatRoot;
  delete [] rstatRoot;

  delete [] cstatSavedBasis;
  delete [] rstatSavedBasis;

  if( genKnapCut + genFlowCut >= 1 )
    {
      for( int i = 0; i < numCuts; i++ )
	delete cuts[i];
      delete [] cuts;
    }

  delete [] branches;

  delete knapGen;
  delete flowGen;
  delete sKnapGen;
  delete branchHandler;
  delete pseudocosts;
}


MWReturn FATCOP_worker::unpack_init_data()
{

  MWprintf( 40, "Begin unpacking worker init data...\n");
  // Unpack the formulation...

  RMC->unpack( &lpsolvertouse, 1, 1 );
  assert( ( lpsolvertouse >= 1 ) && ( lpsolvertouse <= 3 ) );
  if( lpsolvertouse == CPLEX )
    {
      RMC->unpack( cplexLicenseDir );
      MWprintf( 20, "Using CPLEX.  License dir: %s\n", cplexLicenseDir );
    }			    

  formulation->MWUnpack( RMC );

  MWprintf( 80, "Unpacked formulation\n" );
  if( MWprintf_level >= 80 )
    {
      cout << (*formulation) << endl;
    }

  //save it in original formulation
  originalformulation = new CProblemSpec( *formulation );
  
  int ncols = formulation->get_ncols();
  int nrows = formulation->get_nrows();

  RMC->unpack( &ncols, 1, 1 );
  RMC->unpack( &nrows, 1, 1 );

  cstatRoot = new CBasisStatus [ncols];
  rstatRoot = new CBasisStatus [nrows];

  RMC->unpack( cstatRoot, ncols, 1 );
  RMC->unpack( rstatRoot, nrows, 1 );
  
  // Pass it a cutoff value.
  RMC->unpack( &bestUpperBound, 1, 1 );
  
  // Pass all the strategy information that the Worker needs to know about,
  // and set up the 
  RMC->unpack(&branStrategy,1,1);
  RMC->unpack(&genKnapCut,1,1);
  RMC->unpack( &genFlowCut, 1, 1);
  RMC->unpack( &genSKnapCut, 1, 1 );
  RMC->unpack(&maxCuts,1,1);

  RMC->unpack(&divingHeuristic,1,1);  //1: do  0: no

  RMC->unpack(&maxTaskTime,1,1);  
  RMC->unpack(&maxTaskNodes,1,1);  
  RMC->unpack(&nodepreprocessing,1,1);  
  
  int nCuts = 0;
  if( genKnapCut + genFlowCut + genSKnapCut >= 1 )
    {
      // We will use the cuts array to unpack the cuts
      //  loaded to the LP.
      cuts = new CCut * [maxCuts];

      RMC->unpack( &nCuts, 1, 1 ); 

      MWprintf( 30, " Worker %s received %d cuts\n", mach_name, nCuts );

      for( int i = 0; i < nCuts; i++ )
	{
	  //!!! Don't forget to allocated memory for the cuts you're unpacking!!!
	  cuts[i] = new CCut;
	  cuts[i]->MWUnpack( RMC );
	  if( MWprintf_level >= 80 )
	    cout << (*cuts[i]) << endl;
	}

      // Setup to generate cuts
      if( genKnapCut )
	{
	  knapGen = new CKnapsackCutGen ( ncols );
	  MWprintf( 80, "Created knapsack cut generator\n" );
	}
      if( genFlowCut )
	{
	  flowGen = new CFlowCutGen( ncols );      
	  MWprintf( 80, "Created flow cut generator\n" );
	}
      if( genSKnapCut )
	{
	  sKnapGen = new CSKnapsackCutGen ( ncols, ncols );
	  MWprintf( 80, "Created surrogate knapsack cut generator\n" );
	}
    }


  // Load LP.
  // We use CPLEX as an LP solver unless there are "too many"
  //  tokens currently taken.  XXX Right now, use only SOPLEX
  //  so as not to make Michael mad.  :-)

  bool created = 0;
#if defined( CPLEXv6 )
  if( lpsolvertouse == CPLEX )
    {
      lpsolver = new CCPLEXInterface( cplexLicenseDir, created );
      if( ! created )
	{
	  MWprintf( 10, "CPLEX interface creation failed on %s\n", mach_name );
	}
      else
	{
	  MWprintf( 30, "Worker %s using CPLEX LP Interface\n", mach_name );
	  activelpsolver = CPLEX;
	}
    }
#endif

  if( ! created ){
    if( lpsolvertouse == MINOS ){
      //lpsolver = new CMINOSInterface;
      lpsolver = new CSOPLEXInterface; //change it later
      activelpsolver = MINOS;
      MWprintf( 10, " Creating MINOS interface\n" );
    }
    else {
#if defined( SOPLEXv1 )
      {
	lpsolver = new CSOPLEXInterface;
	activelpsolver = SOPLEX;
	MWprintf( 30, " Worker %s using SOPLEX LP Interface\n", mach_name );
      }
#endif
    }
  }
  
  // This copies the formulation into the LPSolver Data Structures
  lpsolver->init( formulation->get_ncols(),
		  formulation->get_nrows(),
		  formulation->get_nnzero(),
		  formulation->get_objsen(),
		  formulation->get_objx(),
		  formulation->get_rhsx(),
		  formulation->get_senx(),
		  formulation->get_colmajor()->get_matbeg(),
		  formulation->get_colmajor()->get_matcnt(),
		  formulation->get_colmajor()->get_matind(),
		  formulation->get_colmajor()->get_matval(),
		  formulation->get_bdl(),
		  formulation->get_bdu(),
		  NULL );

  CLPStatus lpstat = LP_UNKNOWN_ERROR;
  // This loads the problem into the LPSolver.

  if( lpsolver->LPLoad() != 0 )
    {
      MWprintf( 10, "Woah Nellie!!!  You were unable to load the LP.  Exiting...");
      return ABORT;
    }

  lpsolver->LPLoadBasis( cstatRoot, rstatRoot );

  // Allocate memory to hold LP solution
  xlp = new double[ formulation->get_ncols() ];
  dj = new double [formulation->get_ncols() ];
  
  lpstat = lpsolver->LPSolveDual( &zlp, xlp, NULL, NULL, dj );
  int nit = lpsolver->LPGetItCount();
 
  //cout << nit << " iterations necessary to reoptimize LP" << endl;

  if( lpstat != LP_OPTIMAL )
    {
      MWprintf( 10, "Woah Nellie!!!  You were unable to solve the root LP\n" );
      MWprintf( 10, " LP exit status: %d.  Aborting!\n", lpstat );
      return ABORT;
    }

  // Now load the cuts...
  if( nCuts > 0 )
    {
      LoadCutsToLPSolver( lpsolver, nCuts, cuts );      
      lpstat = lpsolver->LPSolveDual( &zlp, xlp, NULL, NULL, dj );
      if( lpstat != LP_OPTIMAL )
	{
	  MWprintf( 10, "Woah Nellie!!!  You were unable to solve the LP after loading cuts.  Aborting.\n" );
	  MWprintf( 10, " LP exit status: %d.  Aborting!\n", lpstat );	  
	  return ABORT;
	}
    }

  // Save this basis...
  cstatSavedBasis = new CBasisStatus [lpsolver->LPGetActiveCols()];
  rstatSavedBasis = new CBasisStatus [lpsolver->LPGetActiveRows()];
  nRowsSavedBasis = lpsolver->LPGetActiveRows();

  lpsolver->LPGetBasis( cstatSavedBasis, rstatSavedBasis );

  if( branStrategy == 0 )
    {
      //XXX Just initialize explicitly now...
      // We get and set all vital information about what
      //  to do with pseudocosts "on the fly"
      int INIT_STRATEGY = 0;
      pseudocosts = new PseudoCostManager( formulation, lpsolver, INIT_STRATEGY );

      // Unpack the working pseudocosts.  (Only if you need them).
      if( branStrategy == 0 )
	pseudocosts->MWUnpackWorking( RMC );
    }

  /// Set up the branch handler. (no priorities now)
  branchHandler = new BranchManager( formulation, branStrategy, pseudocosts,
				     NULL );
  
  // Now you're ready to roll!!!

  MWprintf( 20, "Worker %s solved root LP, zlp = %.6lf\n", mach_name, zlp );

#if 0
  if( !nodepreprocessing ){
    for( int i = 0; i < nCuts; i++ )
      delete cuts[i];
    
    // Set the index back to the top of your array of pointers
    //  for the cuts.
    numCuts = 0;
  }
  else{
#endif
    numCuts = nCuts;
    oldNumCuts = nCuts;
    //}
  
  return OK;
}

void FATCOP_worker::execute_task( MWTask * t )
{
  
#ifdef DEBUGMW
  cout<<" &&&& calling FATCOP_worker execute_task"<<endl;
#endif

  clock_t finishTick;
  double taskTime = 0.0;
  startTick = clock();

  // We have to downcast this task passed to us into a user_task:
  FATCOP_task *tf = dynamic_cast<FATCOP_task *> ( t );

  // Zero out all task related counters
  taskNodes = 0;  
  numLPPivots = 0;
  nCutTrial = 0;
  nCutSuccess = 0;
  cutTime = 0.0;
  divingHeuristicTrial = 0;
  divingHeuristicSuccess = 0;
  divingHeuristicTime = 0.0;
  branchHandler->setBranTime( 0.0 );
  lpsolveTime = 0.0;


  oldNumCuts = numCuts;    
  bool backtrack = false;  
  bool solutionFound = false;
  stackIndex = -1;


#if defined( DEBUG_DFS )
  cout << " Worker got node: " << (*(tf->startingNode) ) << endl;
#endif

  // copy original formulation to working formulation
  if( nodepreprocessing ) {
    formulation->copy( *originalformulation );
    //load bounds to working formulation, not the original!
    //original formulation should be kept intact always
    LoadBoundsToFormulation( tf->startingNode );
  }
  
  // Do reduced cost fixing first, it will help the preprocessing.
  // This is essentially processing the node, you must treat it as such!!!

  LoadBoundsToLPSolver( tf->startingNode );
    
  // Either do one of two things here...
  // 1) Load "root" LP basis, augmented by slacks on new cuts
  // 2) Load the last "good" basis, augemented by slacks on new cuts (if any)
  //  We'll write two different functions.

  // Load the last good basis to the lp solver.
  LoadSavedBasisToLPSolver();

  clock_t t1 = clock();
  CLPStatus sol_status = lpsolver->LPSolveDual( &zlp, xlp, NULL, NULL, dj );
  lpsolveTime += (double) ( clock() - t1 ) / CLOCKS_PER_SEC;
  numLPPivots += lpsolver->LPGetItCount();
  
  int nvars_fixed_by_RC = 0;

  if( LP_OPTIMAL == sol_status && bestUpperBound < hinf )
    {
      //change bounds in both lpsolver and working formulation if do node 
      //preprocessing
      nvars_fixed_by_RC = ReducedCostFixing();    
    }
  
  // XXX node preprocessing
  int nBoundChanges = 0;
  int nCoefChanges = 0;
  int nRowsRemoved = 0;
  if( nodepreprocessing ){
    
    formulation->preprocess( &nBoundChanges, &nCoefChanges, &nRowsRemoved, 0 );

    if( nRowsRemoved != 0 )
      {
	MWprintf( 10, "You promised not to remove any rows from the formulation!\n" );
	assert( 0 );
      }
    
    if( nodepreprocessing >= 2 && nCoefChanges > 0 )
      {
	lpsolver->LPClear();
	
	// This copies the formulation into the LPSolver Data Structures
	MWprintf( 50, "Re-initializing LP with new formulation\n" );
	lpsolver->init( formulation->get_ncols(),
			formulation->get_nrows(),
			formulation->get_nnzero(),
			formulation->get_objsen(),
			formulation->get_objx(),
			formulation->get_rhsx(),
			formulation->get_senx(),
			formulation->get_colmajor()->get_matbeg(),
			formulation->get_colmajor()->get_matcnt(),
			formulation->get_colmajor()->get_matind(),
			formulation->get_colmajor()->get_matval(),
			formulation->get_bdl(),
			formulation->get_bdu(),
			NULL );	
	
	
	// This loads the problem into the LPSolver.

	int loadstat = lpsolver->LPLoad();
	if( loadstat != 0 ) 
	  {
	    MWprintf( 10, " After preprocessing.  Error (%d) loading problem into LP solver\n", loadstat );
	    RMC->exit( 1 );
	  }
    
	//load cuts to lp solver
	if( numCuts > 0 )
	  {
	    MWprintf( 30, "(Re)loading %d cuts to the LPSOLVER\n", numCuts );
	    LoadCutsToLPSolver( lpsolver, numCuts, cuts );   
	  }


	if( ( activelpsolver != SOPLEX ) || ( nCoefChanges == 0 ) )
	  {
	    MWprintf( 30, "Loading Root Basis to LPSolver...\n" );
	    //XXX This may cause a problem if you have purged cuts
	    int nr = lpsolver->LPGetActiveRows();
	    if( nRowsSavedBasis == nr )
	      LoadSavedBasisToLPSolver();
	    else
	      LoadRootBasisToLPSolver();
	  }
	else
	  {
	    MWprintf( 30, "For your SOPLEX safety, we are *not* loading the a basis\nIt may be neither primal nor dual feasible\nYou may experience performance degradation\n" );
	  }

      }
    else //no coefs are modified
      {
	if( nBoundChanges > 0 )
	  {
	    lpsolver->LPSetAllBdl( formulation->get_bdl(), 
				   formulation->get_ncols() );
	    lpsolver->LPSetAllBdu( formulation->get_bdu(), 
				   formulation->get_ncols() );
	    LoadSavedBasisToLPSolver();
	  }
      }
    
  } //end nodepreprocessing
  

  if( nvars_fixed_by_RC > 0 || 
      (nodepreprocessing >= 1 &&  nBoundChanges + nCoefChanges > 0) )
    {

      // See if it is OK to load the saved basis (even with SOPLEX?)
      clock_t t1 = clock();
      sol_status = lpsolver->LPSolveDual( &zlp, xlp, NULL, NULL, dj );
      lpsolveTime += (double) ( clock() - t1 ) / CLOCKS_PER_SEC;
      numLPPivots += lpsolver->LPGetItCount();
    }

  MWprintf( 30, " It took %d iterations and %.2lf seconds to reoptimize the LP and get it ready for the task\n",  
	    numLPPivots, 
	    (double) ( clock() - startTick ) / CLOCKS_PER_SEC );

  // Perhaps this node is infeasible -- check for this.  
  if( sol_status != LP_OPTIMAL || zlp >= bestUpperBound )
    {
      if( sol_status != LP_OPTIMAL )
	{
	  MWprintf( 20, "Root of task not feasible -- (Stat: %d)\n", sol_status );
	  //lpsolver->LPWriteMPS( "/local.wheel/linderot/final" );
	}
      else
	{
	  MWprintf( 20, "Root of task suboptimal -- (%.2lf > %.2lf)\n",
		    zlp, bestUpperBound );
	}

      taskNodes = 1;
      goto DONE;      
    }

  // Set the simplex upper bound so that you can quit early.
  lpsolver->LPSetSimplexUpperBound( bestUpperBound );
  
  // Are we going to try a diving heuristic?
  if( ( divingHeuristic > 0 ) && ( zlp < bestUpperBound ) )
    {
      MWprintf( 30, "Worker %s performing diving heuristic\n", mach_name );
      divingHeuristicTrial = 1;
      divingHeuristicSuccess = 0;

      clock_t startHeurTick = clock();
      solutionFound = diving_heuristic( tf );
      clock_t finishHeurTick = clock();
      divingHeuristicTime +=  (double) (finishHeurTick-startHeurTick)/CLOCKS_PER_SEC;
      MWprintf( 30, "Worker %s completed diving heuristic in %.2f secs\n", 
		mach_name, divingHeuristicTime );

      if( solutionFound )
	{
	  divingHeuristicSuccess = 1;
	}

    }
  
  /*
   * The main loop
   */
  
  do
    {

      backtrack = false;

      CLPStatus lpstat = LP_OPTIMAL;
      bool feasibleLP = false;

      ProcessLoadedLP( lpstat, feasibleLP );

#if defined( DEBUG_DFS )
	  cout << "** zlp: " << zlp << endl;
#endif

      taskNodes++;

      finishTick = clock();
      taskTime = (double) (finishTick-startTick)/CLOCKS_PER_SEC;      

      if( lpstat != LP_OPTIMAL || zlp > bestUpperBound || feasibleLP )
	{
	  if( feasibleLP && zlp < bestUpperBound - EPSILON )
	    {
	      MWprintf( 10, "Worker %s found improved solution of value %.4lf\n", 
			mach_name, zlp );
	      solutionFound = true;
	      bestUpperBound = zlp;	      
	      int nc = originalformulation->get_ncols();
	      tf->numVars = nc;
	      if( NULL == tf->feasibleSolution )
		tf->feasibleSolution = new double[nc];
	      for( int j = 0; j < nc; j++ )
		tf->feasibleSolution[j] = xlp[j];
	    }
	  else if( lpstat == LP_INFEASIBLE )
	    {
	      MWprintf( 80, "Infeasible LP.\n" );

#if 0
	      static int xxx = 0;
	      char jtname[32];
	      sprintf( jtname, "/tmp/%s-%d.mps", "junk", xxx );
	      xxx++;
	      lpsolver->LPWriteMPS( jtname );
#endif
	      backtrack = true;
	    }
	  else if( lpstat == LP_UNBOUNDED )
	    {
	      // Since you used dual simplex, lp will return unbounded
	      //  (at least with CPLEX )
	      MWprintf( 80, "Infeasible LP\n" );	      
	      backtrack = true;
	    }
	  else if( lpstat == LP_OBJ_LIM || zlp > bestUpperBound - EPSILON )
	    {
	      MWprintf( 80, "Cutoff LP\n" );	      
	      backtrack = true;
	    }
	  else
	    {
	      MWprintf( 10, "Unknown LP return status: %d\n", lpstat );
	      backtrack = true;
	    }

	  if( backtrack )
	    {
	      popNode();
	    }
	}
      else
	{

	  // This was a good basis -- save it!

	  SaveCurrentBasis();

	  // Determine branching variable.  (Right now
	  //  we only branch on variables).

	  // Updates all pseudocost information.
	  int branchingIndex = -1;
	  double dndeg = 0.0;
	  double updeg = 0.0;
	  double dnest = zlp;
	  double upest = zlp;
	  branchHandler->select_a_var( xlp, 
				       zlp, 
				       branchingIndex,
				       dndeg,
				       updeg,
				       dnest,
				       upest );

	  if( branchingIndex < 0 )
	    {
	      cout << "Error -- no variable to branch on?" << endl;
	      assert( 0 );
	    }


	  if( dndeg < updeg )
	    {	      
	      // Do the down branch -- push it on the stack

	      double oldval = DBL_MAX;
	      lpsolver->LPGetBdu( &oldval, branchingIndex, branchingIndex );
	      pushNode( branchingIndex, 'U', oldval,
			xlp[branchingIndex], 1,
			zlp, upest );

	      // Update LP formulation
	      lpsolver->LPSetOneBdu( (double) ( (int) xlp[branchingIndex] ), 
				     branchingIndex );	      

	    }
	  else
	    {
	      // Do the up branch, push it on the stack

	      double oldval = -DBL_MAX;
	      lpsolver->LPGetBdl( &oldval, branchingIndex, branchingIndex );
	      
	      pushNode( branchingIndex, 'L',  oldval,
			xlp[branchingIndex], 1, 
			zlp, dnest );
	      
	      // Update LP formulation
	      lpsolver->LPSetOneBdl( (double) ( (int) xlp[branchingIndex] + 1 ),
				     branchingIndex );
	    }
	  
	}
      
#if defined( DEBUG_DFS )
      cout << "stackIndex = " << stackIndex << endl
	   << " taskTime = " << taskTime << endl
	   << " maxTaskTime = " << maxTaskTime << endl
	   << " taskNodes = " << taskNodes << endl
	   << " maxTaskNodes = " << maxTaskNodes << endl;
#endif
      
    } while( stackIndex >= 0 && taskTime < maxTaskTime 
	     && taskNodes < maxTaskNodes 
	     && ! solutionFound );


  // You are done with this task.  Delete un-needed memory and pack up the results...
 DONE:
  finishTick = clock();
  taskTime = (double) (finishTick-startTick)/CLOCKS_PER_SEC;      

  delete tf->startingNode;  tf->startingNode = 0;
	    

  tf->improvedSolution = solutionFound ? 1 : 0;
  tf->solutionValue = bestUpperBound;
  tf->CPUTime = taskTime;
  tf->numNodesCompleted = taskNodes;
  tf->numLPPivots = numLPPivots;
  tf->nCutTrial = nCutTrial;
  tf->nCutSuccess = nCutSuccess;
  tf->cutTime = cutTime;
  tf->divingHeuristicTrial = divingHeuristicTrial;
  tf->divingHeuristicSuccess = divingHeuristicSuccess;
  tf->divingHeuristicTime = divingHeuristicTime;
  tf->branTime = branchHandler->getBranTime();
  tf->lpsolveTime = lpsolveTime;

  tf->stackDepth = stackIndex + 1;
  tf->branchStack = branches;
  tf->backtracking = backtrack ? 1 : 0;

  // Now we always save the cuts on the worker side
  tf->numNewCuts = numCuts - oldNumCuts;

  // Does pointer arithmetic here
  tf->newCuts = cuts + oldNumCuts;

  MWprintf( 60, "Task %d Statistics:\n* %.2lf seconds  (Max: %.2lf)\n* %d nodes  (Max: %d)\n* %d pivots\n* Improved Solution?: %d\nstack depth: %d\n* Backtracking?: %d\n* # new cuts: %d\n", 
	    tf->number, 
	    tf->CPUTime, 
	    maxTaskTime,
	    tf->numNodesCompleted, 
	    maxTaskNodes,
	    tf->numLPPivots, 
	    tf->improvedSolution,
	    tf->stackDepth, 
	    tf->backtracking, 
	    tf->numNewCuts );


  // If you want to pass pseudocosts -- Pack up the pseudocosts...
  if ( branStrategy == 0 )
    {
      tf->pseudocosts = pseudocosts;
      pseudocosts->clearNew();
    }
  else
    {
      tf->pseudocosts = NULL;
    }

  
 
  return;
}

void FATCOP_worker::unpack_driver_task_data( void )
{
  RMC->unpack( &bestUpperBound, 1, 1 );
  RMC->unpack( &maxTaskTime, 1, 1 );
  if( maxTaskTime < 0 )
    maxTaskTime = DBL_MAX;

  RMC->unpack( &maxTaskNodes, 1, 1 );
  if( maxTaskNodes < 0 )
    maxTaskNodes = INT_MAX;
  
  // Unpack all strategy information
  RMC->unpack( &genKnapCut, 1, 1 );
  RMC->unpack( &genFlowCut, 1, 1 );
  RMC->unpack( &genSKnapCut, 1, 1 );
  RMC->unpack( &nodepreprocessing, 1, 1 );
  RMC->unpack( &divingHeuristic, 1, 1 );

  int newiniPCost = 0;
  int newPseudoItlim = 0;
  int newPseudoVarlim = 0;

  RMC->unpack( &newiniPCost, 1, 1 );
  RMC->unpack( &newPseudoItlim, 1, 1 );
  RMC->unpack( &newPseudoVarlim, 1, 1 );
  
  // Now set it in the pseudocost manager
  pseudocosts->setInit( newiniPCost );
  pseudocosts->setItlim( newPseudoItlim );
  branchHandler->setMaxBranVars( newPseudoVarlim );
}

//XXX (Jeff learned the hard way that you had better load the cuts to SOPLEX
//  all at once).    Otherwise it wastes a lot of memory.
//(perhaps he should put this logic in the soplex lp solver).

void FATCOP_worker::LoadCutsToLPSolver( CLPSolverInterface *lps, int n, CCut **c )
{
  
  double *crhs = new double [n];
  char *csen = new char [n];
  int cnz = 0;
  for( int i = 0; i < n; i++ )
    {
      crhs[i] = c[i]->get_rhs();
      //XXX You should write a casting perator, or why was sense
      //    ever an int in the first place.
      int isen = c[i]->get_sense();
      csen[i] = ( isen == LESS_OR_EQUAL ? 'L' : ( isen == EQUAL_TO ? 'E' : 'G' ) );
      cnz += c[i]->get_nnzero();	  
    }

  int *cmatbeg = new int [n+1];
  int *cmatind = new int [cnz];
  double *cmatval = new double [cnz];
      
  int tmp = 0;
  for( int i = 0; i < n; i++ )
    {
      cmatbeg[i] = tmp;
      const int *ix = c[i]->get_ind();
      const double *val = c[i]->get_coeff();
      for( int j = 0; j < c[i]->get_nnzero(); j++ )
	{
	  cmatind[tmp] = ix[j];
	  cmatval[tmp++] = val[j];
	}
    }

  assert( cnz == tmp );
  cmatbeg[n] = cnz;
	
  lps->LPAddRows( n, cnz, crhs, csen, cmatbeg, cmatind, cmatval );      

  delete [] crhs;
  delete [] csen;
  delete [] cmatbeg;
  delete [] cmatind;
  delete [] cmatval;
  
}

void FATCOP_worker::LoadBoundsToLPSolver( MIPNode *node )
{
  lpsolver->LPSetAllBdl( originalformulation->get_bdl(),
			 originalformulation->get_ncols() );
  lpsolver->LPSetAllBdu( originalformulation->get_bdu(), 
			 originalformulation->get_ncols() );
  
  for( int i = 0; i < node->numBounds; i++ )
    {
      if( node->whichBound[i] == 'L' )
	lpsolver->LPSetOneBdl( node->boundVal[i], node->boundIx[i] );
      else if( node->whichBound[i] == 'U' )
	lpsolver->LPSetOneBdu( node->boundVal[i], node->boundIx[i] );
      else
	{
	  cout << "ERROR!  Bound from node is: " << node->whichBound[i] << "?";
	  assert( 0 );
	}
    }
}


void FATCOP_worker::LoadBoundsToFormulation( MIPNode *node )
{
  for( int i = 0; i < node->numBounds; i++ )
    {
      if( node->whichBound[i] == 'L' )
	formulation->acc_bdl()[ node->boundIx[i] ]
	  = node->boundVal[i];
      else if( node->whichBound[i] == 'U' )
	formulation->acc_bdu()[ node->boundIx[i] ]
	  = node->boundVal[i];
      else
	{
	  cout << "ERROR!  Bound from node is: " << node->whichBound[i] << "?";
	  assert( 0 );
	}
    }
  
}


void FATCOP_worker::LoadRootBasisToLPSolver()
{
  int oldm = formulation->get_nrows();
  int newm = lpsolver->LPGetActiveRows();

  if( oldm == newm )
    {
      lpsolver->LPLoadBasis( cstatRoot, rstatRoot );
    }
  else
    {
      assert( newm > oldm );
      CBasisStatus *rstat = new CBasisStatus [newm];
      memcpy( rstat, rstatRoot, oldm * sizeof( CBasisStatus ) );
      for( int i = oldm; i < newm; i++ )
	rstat[i] = ::BASIC;
      lpsolver->LPLoadBasis( cstatRoot, rstat );
      delete [] rstat;
    }

}

void FATCOP_worker::SaveCurrentBasis()
{
  // Store the last "reasonable" basis

  int newm = lpsolver->LPGetActiveRows();
  if( newm == nRowsSavedBasis )
    {
      lpsolver->LPGetBasis( cstatSavedBasis, rstatSavedBasis );
      nRowsSavedBasis = newm;
    }
  else
    {
      delete [] rstatSavedBasis;
      rstatSavedBasis = new CBasisStatus [newm];
      lpsolver->LPGetBasis( cstatSavedBasis, rstatSavedBasis );
      nRowsSavedBasis = newm;
    }

#if defined( JEFF_DEBUG )
  cout << "  Saved basis.  nrows: " << newm << endl;
  int nb = 0;
  for( int i = 0; i < lpsolver->LPGetActiveCols(); i++ )
    if( cstatSavedBasis[i] == BASIC )
      nb++;
  for( int i = 0; i < lpsolver->LPGetActiveRows(); i++ )
    if( rstatSavedBasis[i] == BASIC )
      nb++;
  cout <<  " There are " << nb << " basic variables" << endl;
#endif


}

void FATCOP_worker::LoadSavedBasisToLPSolver()
{
  int oldm = nRowsSavedBasis;
  int newm = lpsolver->LPGetActiveRows();

#if defined( JEFF_DEBUG )
  cout << "Saved basis had: " << nRowsSavedBasis << " rows." 
       << "  New basis has " << newm << " rows" << endl;
#endif
  
  if( oldm == newm )
    {
      lpsolver->LPLoadBasis( cstatSavedBasis, rstatSavedBasis );
    }
  else if( newm > oldm )
    {
      CBasisStatus *rstat = new CBasisStatus [newm];
      memcpy( rstat, rstatSavedBasis, oldm * sizeof( CBasisStatus ) );
      for( int i = oldm; i < newm; i++ )
	rstat[i] = ::BASIC;
      lpsolver->LPLoadBasis( cstatSavedBasis, rstat );
      delete [] rstat;
    }
  else
    {
      // Load the Root basis, then make the rest of the rows basic.      
      int rootm = originalformulation->get_nrows();      
      CBasisStatus *rstat = new CBasisStatus [newm];
      assert( newm >= rootm );
      memcpy( rstat, rstatRoot, rootm * sizeof( CBasisStatus ) );
      for( int i = rootm; i < newm; i++ )
	rstat[i] = ::BASIC;
      lpsolver->LPLoadBasis( cstatSavedBasis, rstat );
      delete [] rstat;	
    }
}

// This one solves LP and adds cuts
void FATCOP_worker::ProcessLoadedLP( CLPStatus &lpstat, bool &feasibleLP )
{

  // Things to detect tailing off
  double zlp2 = -DBL_MAX;
  double zlp1 = -DBL_MAX;
  const double TAIL_OFF_PCT = 0.025;

  // Store the old objective value...
  double oldzlp = zlp;

#if defined( DEBUG_BOUNDS )
  cout << " Bounds loaded to LP Solver..." << endl;
  int nc = formulation->get_ncols();
  double *tbdl = new double [nc];
  double *tbdu = new double [nc];
  lpsolver->LPGetBdl( tbdl, 0, nc - 1 );
  lpsolver->LPGetBdu( tbdu, 0, nc - 1 );

  for( int i = 0; i < nc; i++ )
    cout << "  " << tbdl[i] << " <= " << "x[" << i << "] <= " << tbdu[i] << endl;

  delete [] tbdl;
  delete [] tbdu; 
#endif  

  while( 1 )
    {
      clock_t t1 = clock();
      lpstat = lpsolver->LPSolveDual( &zlp, xlp, NULL, NULL, dj );
      lpsolveTime += (double) ( clock() - t1 ) / CLOCKS_PER_SEC;
      numLPPivots += lpsolver->LPGetItCount();
      
      if( lpstat != LP_OPTIMAL )
	{
#if defined( DEBUG_DFS )
	  MWprintf( 10, " Solution Status: %d\n", lpstat );	 
#endif
	  break;
	}
      else
	{
#if defined( DEBUG_DFS )
	  MWprintf( 10, "LP Value: %.2lf in %d iterations\n", zlp, 
		    lpsolver->LPGetItCount() );
#endif
	}

      if( originalformulation->feasibleSolution( xlp ) )
	{
	  feasibleLP = true;
	  
	  //XXX allow user defined heuristics to improve this solution
	  //add performSearching() here
	  
	  break;
	}


      //XXX allow user defined heuristics to round this solution
      //add performRounding() here
      
      if( ( ( zlp - zlp2 ) / (zlp > 0 ? zlp : -zlp) )  < TAIL_OFF_PCT ) 
	{
	  MWprintf( 70, " Cuts have tailed off.  Stopping node\n" );
	  // Cuts aren't making enough progress -- quit!
	  break;
	}
      else
	{
	  zlp2 = zlp1;
	  zlp1 = zlp;
	}

      clock_t startCutTick = clock();
      int ncutsThisRound = GenerateCuts();
      clock_t finishCutTick = clock();
      cutTime +=  (double) (finishCutTick-startCutTick)/CLOCKS_PER_SEC;

      if( ncutsThisRound == 0 )
	{
	  break;
	}

      nCutSuccess++;
      LoadCutsToLPSolver( lpsolver, ncutsThisRound, &(cuts[numCuts]) );
      numCuts += ncutsThisRound;

      if( (double) (finishCutTick - startTick)/CLOCKS_PER_SEC > maxTaskTime )
	{
	  MWprintf( 20, "Stopping task in cut generation phase\n" );
	  break;
	}

    }

  /* Now you are done processing the node.  Update the pseudocost information.
   *
   *  What should you do if problem is infeasible?  (Skip it, for now...)
   */

  if( lpstat == LP_OPTIMAL )
    {
      if( stackIndex >= 0 )
	{
	  if( pseudocosts )
	    {
	      pseudocosts->updateFromLP( branches[stackIndex].idx,
					 branches[stackIndex].val,
					 oldzlp, zlp,
					 branches[stackIndex].whichBound );
	    }
	}
	
    }

  

}

int FATCOP_worker::GenerateCuts()
{
  
  if( genKnapCut + genFlowCut + genSKnapCut <= 0 )
    return 0;

  int knapcuts = 0;
  int flowcuts = 0;
  int sknapcuts = 0;
  int nthisround = 0;
    
  const int MAXCUTSPERROUND = 25;
  if( maxCuts - numCuts <= MAXCUTSPERROUND )
    {
      
      MWprintf( 10, "Cut pool has filled on machine %s.  Purging!!!\n", mach_name );
      for( int i = 0; i < numCuts; i++ )
	{
	  delete cuts[i];
	  cuts[i] = NULL;
	}

      numCuts = 0;
      oldNumCuts = 0;
    }

  int startingPoint = numCuts;
  nCutTrial++;

  // Create a CLPSolution from xlp.  
  //XXX Inefficient memory copy.
  CLPSolution *sol = new CLPSolution( originalformulation->get_ncols(), xlp, dj );
  
  int nleft = MAXCUTSPERROUND;
  
  
  if( genKnapCut )
    {
      knapcuts = knapGen->generate( originalformulation, sol, nleft, 
				    &(cuts[startingPoint]) );
      nleft -= knapcuts;
      nthisround += knapcuts;
      startingPoint += knapcuts;
    }
  
#if defined( DEBUG_CUTS )
  cout << "Found " << knapcuts << " knapsack inequalities" << endl;
#endif
  
  if ( nthisround >= MAXCUTSPERROUND )
    {
      delete sol;
      return ( nthisround );
    }
  
  if( genFlowCut )
    {
      flowcuts = flowGen->generate( originalformulation, sol, nleft, 
				    &(cuts[startingPoint]) );
      nleft -= flowcuts;
      nthisround += flowcuts;
      startingPoint += flowcuts;
    }
  
#if defined( DEBUG_CUTS )
  cout << "Found " << flowcuts << " flow cover inequalities" << endl;
#endif
  
  if ( nthisround >= MAXCUTSPERROUND )
    {
      delete sol;
      return ( nthisround );
    }
  
  if( genSKnapCut )
    {
      sknapcuts = sKnapGen->generate( originalformulation, sol, nleft, 
				      &(cuts[startingPoint]) );
      nleft -= sknapcuts;
      nthisround += sknapcuts;
      //if want to generate more cuts, should update startingPoint here
    }
  
#ifdef DEBUG_CUTS
  cout << "Found " << sknapcuts << " surrogate knapsack inequalities" << endl;
#endif
  
  if ( nthisround >= MAXCUTSPERROUND )
    {
      delete sol;
      return ( nthisround );
    }
  

  if( sol ) delete sol;
  
  return ( nthisround );
  
}

void FATCOP_worker::pushNode( int idx, char whichBound, double oldval,
			      double val, int brotheropen, 
			      double lowerBound, double est )
			      
{  

  ++stackIndex;
  if( stackIndex >= MAX_NODELISTSIZE )
    {
      MWprintf( 10, "You have overflowed the node stack.  This task will quit\n" );
      assert( stackIndex < MAX_NODELISTSIZE );
    }
  assert( stackIndex >= 0 );

#if defined( DEBUG_DFS )
  cout << "Pushed ix: " << idx << " onto stack pos: "
       << stackIndex << endl;
  cout << " xlp[" << idx << "] = " << val << endl;
#endif

  branches[stackIndex].idx = idx;
  branches[stackIndex].whichBound = whichBound;
  branches[stackIndex].oldval = oldval;
  branches[stackIndex].val = val;
  branches[stackIndex].brotheropen = brotheropen;
  if( brotheropen == 1 )
    {
      branches[stackIndex].openLowerBound = lowerBound;
      branches[stackIndex].openEst = est;
    }
  else
    {
      branches[stackIndex].openLowerBound = DBL_MAX;
      branches[stackIndex].openEst = -DBL_MAX;
    }
  
}


void FATCOP_worker::popNode()
{
  if( stackIndex < 0 )
    return;

  while( branches[stackIndex].brotheropen == 0 )
    {
      // Undo LP bound.

      if( branches[stackIndex].whichBound == 'L' )
	{
	  lpsolver->LPSetOneBdl( branches[stackIndex].oldval,
				 branches[stackIndex].idx );
	}
      else if( branches[stackIndex].whichBound == 'U' )
	{
	  lpsolver->LPSetOneBdu( branches[stackIndex].oldval,
				 branches[stackIndex].idx );
	}
      else
	assert( 0 );

      stackIndex--;
      if( stackIndex < 0 )
	return;
    }

  // Now you must switch to the brother node and push him on the stack.
  if( branches[stackIndex].whichBound == 'L' )
    {

      // Undo lower bound.
      lpsolver->LPSetOneBdl( branches[stackIndex].oldval,
			     branches[stackIndex].idx );

      // Switch to other bound
      double oldval =  DBL_MAX;
      lpsolver->LPGetBdu( &oldval, branches[stackIndex].idx, 
			  branches[stackIndex].idx );
      lpsolver->LPSetOneBdu( (double) ( (int) branches[stackIndex].val ),
			     branches[stackIndex].idx );
      int oldsix = stackIndex;
      stackIndex--;
      pushNode( branches[oldsix].idx, 'U', oldval, branches[oldsix].val, 0, 
		DBL_MAX, -DBL_MAX );
      
    }
  else if( branches[stackIndex].whichBound == 'U' )
    {
      // Undo upper bound
      lpsolver->LPSetOneBdu( branches[stackIndex].oldval,
			     branches[stackIndex].idx );      

      // Set the lower bound
      double oldval =  DBL_MAX;
      lpsolver->LPGetBdl( &oldval, branches[stackIndex].idx, 
			  branches[stackIndex].idx );
      lpsolver->LPSetOneBdl( (double) ( (int) branches[stackIndex].val + 1.0 ),
			     branches[stackIndex].idx );
      int oldsix = stackIndex;
      stackIndex--;
      pushNode( branches[oldsix].idx, 'L', oldval, branches[oldsix].val, 0, 
		DBL_MAX, -DBL_MAX );
    }
  else
    assert( 0 );

#if defined( DEBUG_DFS )
  cout << "Popping idx: " <<  branches[stackIndex].idx 
       << " off the stack."  << endl;
#endif

  LoadSavedBasisToLPSolver();  
}

int FATCOP_worker::ReducedCostFixing()
{
    
  int nFixedVars = 0;
    
  for( int i = 0; i < formulation->nInts; i++ ) {
      
    int idx = (formulation->intConsIndex)[i];
    double lb, ub;
    lpsolver->LPGetBdl( &lb, idx, idx );
    lpsolver->LPGetBdu( &ub, idx, idx );
      
    if( cstatSavedBasis[i] != BASIC ){
	  
      //if bounds has already been fixed
      if( lb > ub - tol ) {
	continue;
      }
	  
      //if nonbasic at lower bound and ...
      if( xlp[idx] < lb + tol &&
	  bestUpperBound - zlp - dj[idx] < tol ){
	
	MWprintf( 80, "upper: %f, zlp: %f, dj: %f \n", 
		  bestUpperBound, zlp, dj[idx]);

	lpsolver->LPSetOneBdu( lb, idx );

	if( nodepreprocessing ){
	  formulation->acc_bdu()[idx] = lb;
	}
	  
	MWprintf( 80, "var %d was fixed at lower bound %f \n", idx, lb);
	nFixedVars++;
      }
	  
      //if nonbasic at upper bound and ...
      if( xlp[idx] > ub - tol &&
	  bestUpperBound - zlp + dj[idx] < tol ){

	MWprintf( 80, "upper: %f, zlp: %f, dj: %f \n", 
		  bestUpperBound, zlp, dj[idx]);

	lpsolver->LPSetOneBdl( ub, idx );
	
	if( nodepreprocessing ){
	  formulation->acc_bdl()[idx] = ub;
	}
	  
	MWprintf( 80, "var %d was fixed at upper bound %f \n", idx, ub);
	nFixedVars++;
      }
    }
  }  
    
  if(  nFixedVars > 0 ){
    MWprintf( 60, "local reduced cost fixing fixed %d vars\n", nFixedVars  );
  }
 
  return nFixedVars;
}
