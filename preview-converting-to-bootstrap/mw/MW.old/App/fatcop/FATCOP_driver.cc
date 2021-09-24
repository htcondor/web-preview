#include <iomanip.h>
#include <fstream.h>
#include <stdlib.h>
#include "global.hh"
#include "String.hh"
#include "FATCOP_driver.hh"
#include "FATCOP_worker.hh"
#include "FATCOP_task.hh"

#include "cutpool.hh"
#include "pseudocost.hh"

#if defined( GAMS_INTERFACE )
#include "global_gams.hh"
#endif

/*
 * global key functions.  Used for doing different selection strategies
 */

MWKey dfp(MWTask* t)
{
  FATCOP_task* tf = dynamic_cast<FATCOP_task *> ( t );
  assert( tf );
  return (MWKey) tf->startingNode->depth;
}

MWKey bb(MWTask* t)
{
  FATCOP_task* tf = dynamic_cast<FATCOP_task *> ( t );
  assert( tf );
  return (MWKey) tf->startingNode->bestLowerBound;
}

MWKey pc(MWTask* t)
{
  FATCOP_task* tf = dynamic_cast<FATCOP_task *> ( t );
  assert( tf );
  return (MWKey ) tf->startingNode->bestEstimate;
}

MWKey deepest_first(MWTask* t)
{
  FATCOP_task* tf = dynamic_cast<FATCOP_task *> ( t );
  assert( tf );
  return (MWKey ) tf->startingNode->depth;
}


FATCOP_driver::FATCOP_driver()
{

#ifdef DEBUGMW
  cout<<" $$$$ calling FATCOP_driver constructor "<<endl;
#endif
  
  set_task_add_mode(ADD_BY_KEY);
  set_task_retrieve_mode(GET_FROM_BEGIN);
  set_task_key_function(&bb);
  sort_task_list();

  set_target_num_workers( -1 );

  nTasksReported = 0;
  seleStrategy = 0;
  TASK_HI_THRESHOLD = 2000;
  TASK_LOW_THRESHOLD = 1000;
  current_seleStrategy = 0;

  branStrategy = 0;
  branTime = 0.0;
  lpsolveTime = 0.0;
  presolLev = 0;

  genKnapCut = 0;
  genFlowCut = 0;
  genSKnapCut = 0;

  nCutTrial = 0;
  nCutSuccess = 0;
  cutTime = 0.0;
  lastCutSuccess = 0;

  maxCuts = 1000;

  absoluteGap = 0.0;
  relativeGap = 0.0;
  iniPseuCost = 0;
  pseudoItlim = 50;
  pseudoVarlim = -1;

  divingHeuristic = 0;
  originalDivingHeuristic = 0;
  divingHeuristicTrial = 0;
  divingHeuristicSuccess = 0;
  divingHeuristicTime = 0.0;

  checkpointType = 0;
  checkpointFrequency = INT_MAX;

  logFrequency = 0;
  maxNodes = INT_MAX;
  maxTime = DBL_MAX;
  cumulativeTime = 0.0;
  start_time = time( NULL );
  maxPoolMem = INT_MAX;
  maxWorkers = INT_MAX;
  maxTimeInSlave = DBL_MAX;
  userMaxTimeInSlave = DBL_MAX;
  maxNodeInSlave = INT_MAX;
  userMaxNodeInSlave = INT_MAX;
  nodepreprocessing = 0;

  t_num_arches = 0;
  
  altlpsolver = SOPLEX;
  lpsolver = NULL;
  numCplexLicenses = 0;
  numCplexLeaveOpen = 100;
  mycplexcopies = 0;
  maxcplexcopies = 100;
  cplexPointerFile[0] = '\0';
  cplexLicenseDir[0] = '\0';
  formulation = NULL;
  cPool = NULL;
  pseudocosts = NULL;
  bestUpperBound = DBL_MAX;
  nNodes = 0;
  lastlognNodes = 0;
  lpPivots = 0;
  solutionTime = 0.0;
  
  zlpRoot = -DBL_MAX;
  xlpRoot = NULL;
  djRoot = NULL;
  
  cstatRoot = NULL;
  rstatRoot = NULL;
  
  // Qun -- GAMS stuff...
  st =  UNINITIALIZED;
  feasibleSolution = NULL;

  //sol_lb = NULL;
  //sol_ub = NULL;
  revobj = 0;
}


FATCOP_driver::~FATCOP_driver(){
#ifdef DEBUGMW
  cout<<" $$$$ calling FATCOP_driver destructor "<<endl;
#endif

  delete lpsolver;
  delete formulation;
  delete cPool;

  delete [] xlpRoot;
  delete [] djRoot;
  delete [] cstatRoot;
  delete [] rstatRoot;

  if( feasibleSolution ) delete [] feasibleSolution;

}

#if ! defined( GAMS_INTERFACE )
MWReturn FATCOP_driver::get_userinfo( int argc, char *argv[] )
{

  MWReturn status = OK;

#ifdef DEBUGMW
  cout<<" $$$$ calling FATCOP_driver get_userinfo "<<endl;
#endif
  
  if( argc < 2 || argc > 3 )
    {
      cerr << "Usage:  fatcop-driver <MPSfile> <Parameter file>  (optional)" << endl;
      return ABORT;
    }

  if( argc == 3 )
    {
      readOption( argv[2] );
    }
  else
    {
      readOption();
    }

  // Tell MW about the checkpoint frequency
  if( checkpointType == 1 )
    set_checkpoint_frequency( checkpointFrequency );
  if( checkpointType == 2 )
    set_checkpoint_time( checkpointFrequency );

  // Create the LP solver so that you can read the MPS file

  bool created = 0;
#if defined( CPLEXv6 )
  lpsolver = new CCPLEXInterface( cplexLicenseDir, created );
#endif
#if defined( SOPLEXv1 )
  if( ! created )
    {
      MWprintf( 10, "Unable to initialize CPLEX interface -- creating SOPLEX interface\n" );
      lpsolver = new CSOPLEXInterface;
    }
#endif

 if( NULL == lpsolver ){
    st = MEMORY_ERROR;
    return ABORT;
  }

  // This will set up everything.
  load( argv[1] );
  
  //make sure the MIP has been read in
  //check whether rootMip has been setup
  if( NULL == formulation ){
    cerr<<"No MIP was from MPS file."<<endl;
    return ABORT;
  }
    
  // XXX perform heuristics before starting?
  if(preHeuristics){
    // bestUpperBound = performPreHeu();
  }
  
 
  // Set up the cutpool on the master
  if( ( genKnapCut + genFlowCut + genSKnapCut > 0 ) && maxCuts>0) {
    cPool = new cutPool(maxCuts);
    if( NULL == cPool ){
      st = MEMORY_ERROR;
      return ABORT;
    }  
  }

  // Setup the branching strategy
  if( branStrategy == 0 )
    {
      pseudocosts = new PseudoCostManager( formulation );
      if( NULL == pseudocosts ){
	st = MEMORY_ERROR;
	return ABORT;
      }
    }

  //setup searching rule 
  switch(seleStrategy){
  case 0:  //peudocost estimation
    set_task_key_function(&pc);
    current_seleStrategy = 0;
    break;
  case 1:  //depth first
    set_task_key_function(&dfp);
    current_seleStrategy = 1;
    break;
  case 2: case 4:  //2: best first, 4: hybrid strategy
    set_task_key_function(&bb);
    current_seleStrategy = 2;
    break;
  case 3:  //deepest first
    set_task_key_function(&deepest_first);
    current_seleStrategy = 3;
    break;
  default:
    cerr<<"wrong searching algorithm."<<endl;
    cerr<<"use default searching algorithm: Best bound!."<<endl;
    set_task_key_function(&bb);
    current_seleStrategy = 2;
    break;
  }

  sort_task_list();
  
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

  // This loads the problem into the LPSolver.
  if( ! lpsolver->LPLoad() )
    {      
      xlpRoot = new double [formulation->get_ncols()];
      djRoot = new double [formulation->get_ncols()];
      cstatRoot = new CBasisStatus [formulation->get_ncols()];
      rstatRoot = new CBasisStatus [formulation->get_nrows()];

      if( NULL == xlpRoot   || NULL == djRoot || 
	  NULL == cstatRoot || NULL == rstatRoot ){
	st = MEMORY_ERROR;
	return ABORT;
      }      

      //XXX Could put barrier solve (with crossover) here.
      CLPStatus sol_status = lpsolver->LPSolvePrimal( &zlpRoot, 
						      xlpRoot,
						      NULL, NULL, 
						      djRoot );
      lpsolver->LPGetBasis( cstatRoot, rstatRoot );

      if ( sol_status != LP_OPTIMAL )
	{
	  MWprintf( 10, "Oh man!!! Unable to solve root LP.  packing up\n" );
	  MWprintf( 10, "Sol status = %d\n", sol_status );
	  
	  if ( sol_status == LP_INFEASIBLE ){
	    cout << "MIP-- is infeasible\n" << flush;
	    st = INFEASIBLE;
	  }
	  else if ( sol_status == LP_UNBOUNDED ){
	    cout << "MIP-- is unbounded\n" << flush;
	    st =  UNBOUNDED;
	  }
	  else{
	    cout << "An error occurred during the call to LP solver" << flush;
	    st =  LPSOLVER_ERROR;
	  }

	  status = ABORT;
	}
      else
	{
	  lpPivots += lpsolver->LPGetItCount();
	  MWprintf( 10, " Root LP: %.4lf.  Solved in %d pivots\n",
		    zlpRoot, lpPivots );

	}
    }

  //actually it might be 1, but we do not count the master's copy  
  mycplexcopies = 0; 

  return status;
}
#else // YOu have compiled for the GAMS Interface...
MWReturn FATCOP_driver::get_userinfo( int argc, char *argv[] )
{

  MWReturn status = OK;
  
  cntrec cntinfo;
  
  if (argc<2) {
    fprintf(stderr,"No command line argument found\n");
    exit(1);
  }
  
  gfinit();
  
  gfrcnt(TRUE,FALSE,&cntinfo,argv[1]);
  
  gfopst(); //open status file
  
  prIntroInfo();
  
  m = cntinfo.kgv[1];
  n = cntinfo.kgv[2];
  nz = cntinfo.kgv[3];
  objvar = cntinfo.kgv[7];
  itlim = cntinfo.kgv[8];
  reslim = cntinfo.xgv[1];
  ginf = cntinfo.xgv[10];
  gminf = cntinfo.xgv[11];
  
  bool useopt = cntinfo.lgv[2];
  if (cntinfo.lgv[3])
    objsen = 1;   /* minimization */
  else
    objsen = -1;  /* maximization */
  
  isamip = cntinfo.lgv[4];
  
  bestLowerBound = iolib.cutoff;
  

  if (isamip) {
    optcRel = cntinfo.xgv[8];
    optcAct = cntinfo.xgv[7];
    priorities = (cntinfo.kmv[9]!=0);
    nints = cntinfo.kmv[1]+cntinfo.kmv[2];
    nodlim = cntinfo.kmv[6];
    int nososvars = cntinfo.kmv[4]+cntinfo.kmv[5];
    if(nososvars>0){
      cout<<"FATCOP is not able to process SOS type variables"<<endl
	  <<"You can either redefine the variables or choose another mip solver"<<endl;
      exit(0);
    }
    
  } 
  
  
  fprintf(gfiosta," \n \n");
  fprintf(gfiosta,"------------------------------------------------------------------------\n");
  fprintf(gfiosta,"CONTROL FILE STATISTICS\n");
  fprintf(gfiosta,"------------------------------------------------------------------------\n");
  fprintf(gfiosta,"Rows : %d, columns  : %d\n",m,n);
  fprintf(gfiosta,"Nonzeros           : %d\n", nz);
  fprintf(gfiosta,"Iteration limit    : %d\n", itlim);
  fprintf(gfiosta,"Resource  limit    : %e\n", reslim);
  fprintf(gfiosta,"Objective variable : %d\n",objvar);
  fprintf(gfiosta,"Objective sense    : %s\n",
	  ((objsen == 1) ? "minimize" : "maximize"));
  
  if (isamip) {
    fprintf(gfiosta,"Binary variables   : %d\n",cntinfo.kmv[1]);
    fprintf(gfiosta,"Integer variables  : %d\n",cntinfo.kmv[2]);
    fprintf(gfiosta,"OPTCA : %e , OPTCR : %e\n",optcAct,optcRel);
    fprintf(gfiosta,"Priorities         : %s\n",
	    (priorities ? "yes" : "no"));
    fprintf(gfiosta,"Node limit         : %d\n",nodlim);
  }
  
  fprintf(gfiosta," \n \n");
  
  
  //1. allocate memory for the matrix
  allocate_memory();
  
  
  //2. read option file
  char optionfile[80];
  
  if (useopt) { 
    gfstct("START");
    gfopti(optionfile);
    get_options(optionfile);
    gfstct("STOPC");
  }
  
  
  //3. read matrix
  readat(gfiolog, gfiosta);
  
  
  //4. set options
  mipWork.setR(optcRel);
  mipWork.setA(optcAct);
  //mipWork.setPriority(xpri,nints);
  mipWork.setNodelim(itlim);
  mipWork.setReslim(reslim);
  //mipWork.setItlim(itlim);
  
  
  //5. reformulate the problem
  reformulate();
  
  
  //6. load the problem to mip solver
  loadToMipSolver();
  message("fatcop loaded the problem");



  //make sure the MIP has been read in
  //check whether rootMip has been setup
  if(NULL==formulation){
    cerr<<"No MIP was from GAMS file."<<endl;
    st = UNINITIALIZED;
    return ABORT;
  }

  // Set up the cutpool on the master
  if(genKnapCut + genFlowCut + genSKnapCut > 0 && maxCuts>0) {
    cPool = new cutPool(maxCuts);
    if( NULL == cPool ){
      st = MEMORY_ERROR;
      return ABORT;
    }
  }
  
  // Setup the branching strategy
  if( branStrategy == 0 )
    {
      pseudocosts = new PseudoCostManager( formulation );
      if( NULL == pseudocosts ){
	st = MEMORY_ERROR;
	return ABORT;
      }
    }

  //setup searching rule 
  switch(seleStrategy){
  case 0:  //peudocost estimation
    set_task_key_function(&pc);
    current_seleStrategy = 0;
    break;
  case 1:  //depth first
    set_task_key_function(&dfp);
    current_seleStrategy = 1;
    break;
  case 2: case 4:  //2: best first, 4: hybrid strategy
    set_task_key_function(&bb);
    current_seleStrategy = 2;
    break;
  case 3:  //deepest first
    set_task_key_function(&deepest_first);
    current_seleStrategy = 3;
    break;
  default:
    cerr<<"wrong searching algorithm."<<endl;
    cerr<<"use default searching algorithm: Best bound!."<<endl;
    set_task_key_function(&bb);
    current_seleStrategy = 2;
    break;
  }
  
  sort_task_list();
  

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

  // This loads the problem into the LPSolver.
  if( ! lpsolver->LPLoad() )
    {      
      xlpRoot = new double [formulation->get_ncols()];
      djRoot = new double [formulation->get_ncols()];
      cstatRoot = new CBasisStatus [formulation->get_ncols()];
      rstatRoot = new CBasisStatus [formulation->get_nrows()];
      
      if( NULL == xlpRoot   || NULL == djRoot || 
	  NULL == cstatRoot || NULL == rstatRoot ){
	st = MEMORY_ERROR;
	return ABORT;
      }
      
      //XXX Could put barrier solve (with crossover) here.
      CLPStatus sol_status = lpsolver->LPSolvePrimal( &zlpRoot, 
						      xlpRoot,
						      NULL, NULL, 
						      djRoot );
      lpsolver->LPGetBasis( cstatRoot, rstatRoot );
      
      if ( sol_status != LP_OPTIMAL )
	{
	  cout << " Oh man!!! Unable to solve root LP.  packing up." 
	       << endl << "Sol status = " << sol_status << endl;

	  if ( sol_status == LP_INFEASIBLE ){
	    cout << "MIP-- is infeasible\n" << flush;
	    st = INFEASIBLE;
	  }
	  else if ( sol_status == LP_UNBOUNDED ){
	    cout << "MIP-- is unbounded\n" << flush;
	    st =  UNBOUNDED;
	  }
	  else{
	    cout << "An error occurred during the call to LP solver" << flush;
	    st =  LPSOLVER_ERROR;
	  }

	  status = ABORT;
	}
      else
	{
#if defined( DEBUGMW )
	  cout << "root LP has value: " << zlpRoot << endl;
#endif
	  lpPivots += lpsolver->LPGetItCount();
	}
    }
  
  message("Starting fatcop...");
  input_time = gfclck();


  mycplexcopies = 0; 
  
  return status;
}

#endif //GAMS_INTERFACE

/// Set up an array of tasks here
MWReturn FATCOP_driver::setup_initial_tasks(int *n_init , MWTask ***init_tasks) 
{
  
#ifdef DEBUGMW
  cout<<" $$$$ calling FATCOP_driver setup_initial_tasks "<<endl;
#endif

  // Put first task on the pool.  Then give task a limit of one node to evaluate.   
  MWReturn status = OK;

  *n_init = 1;
  *init_tasks = new MWTask *[*n_init];
  if( NULL == *init_tasks ){
    st = MEMORY_ERROR;
    return ABORT;
  }
  MIPNode *rootnode = new MIPNode;
  if( NULL == rootnode ){
    st = MEMORY_ERROR;
    return ABORT;
  }
  
  (*init_tasks)[0] = new FATCOP_task( rootnode, pseudocosts );
  if( NULL == (*init_tasks)[0]){
    st = MEMORY_ERROR;
    return ABORT;
  } 

  return status;

}

/// What to do when a task finishes:
MWReturn FATCOP_driver::act_on_completed_task( MWTask * t )
{

#ifdef DEBUGMW
  cout<<" $$$$ calling FATCOP_driver act_on_completed_task "<<endl;
#endif
  
  MWReturn status = OK;
  
  // dynamic casting:
  FATCOP_task* tf = dynamic_cast<FATCOP_task *> ( t );
  assert( tf );
  
  nTasksReported++;
  
  //found better solution?
  if( ( tf->improvedSolution ) && tf->solutionValue < bestUpperBound )
    {
      
      MWprintf( 10, "Task %d returned with improved solution: %.6lf\n",
		tf->number, tf->solutionValue );
      
      bestUpperBound = tf->solutionValue;

      if( !feasibleSolution ) feasibleSolution = new double[formulation->get_ncols()];
      memcpy( feasibleSolution, tf->feasibleSolution,  formulation->get_ncols() * sizeof( double ) );
      
      //XXX Make sure key is by upper bound before doing this!        
      // set_task_key_function
      
      double diff = getStopTol();
      delete_tasks_worse_than(bestUpperBound-diff);
      
      /* 
       * Root reduced cost fixing.
       * You can fix variables and update the formulation.
       */

      int nFixedVars = 0;
      
      for( int i = 0; i < formulation->get_ncols(); i++ ) {
	
	double* lb = formulation->acc_bdl();
	double* ub = formulation->acc_bdu();
	
	if( formulation->get_colmajor()->get_ctype()[i] != COL_TYPE_CONTINUOUS ){
	  if( cstatRoot[i] != BASIC ){
	    
	    //if bounds has already been fixed
	    if( lb[i] > ub[i] - tol ) {
	      continue;
	    }
	    
	    //if nonbasic at lower bound and ...
	    if( xlpRoot[i] < lb[i] + tol &&
		bestUpperBound - zlpRoot - djRoot[i] < tol ){
	      MWprintf( 80, "upper: %f, zlp: %f, dj: %f \n", 
			bestUpperBound, zlpRoot, djRoot[i]);
	      ub[i] = lb[i];
	      MWprintf( 80, "var %d was fixed at lower bound %f \n", i, lb[i]);
	      nFixedVars++;
	    }
	    
	    //if nonbasic at upper bound and ...
	    if( xlpRoot[i] > ub[i] - tol &&
		bestUpperBound - zlpRoot + djRoot[i] < tol ){
	      MWprintf( 80, "upper: %f, zlp: %f, dj: %f \n", 
			bestUpperBound, zlpRoot, djRoot[i]);
	      lb[i] = ub[i];
	      MWprintf( 80, "var %d was fixed at upper bound %f \n", i, ub[i]);
	      nFixedVars++;
	    }
	  }
	}
  
      }
      
      if(  nFixedVars > 0 ){
	MWprintf( 30, "global reduced cost fixing fixed %d vars\n", 
		  nFixedVars  );
	//Qun: question - how can we broadcast the change of formulation to
	//workers that already was started?

	// Jeff -- maybe a good idea.  You would put it in "worker task data"
      }
      
    }
  
  // If the stack depth is <= 0 you processed the whole subtree
  
  if( tf->stackDepth > 0 )
    {
      // Create nodes from the stack.
      int numNewNodes = 0;
      MIPNode **newNodes;
      createNodesFromStack( newNodes, numNewNodes, tf->startingNode, 
			    tf->stackDepth, tf->branchStack, 
			    tf->backtracking );
      
      
      MWprintf( 60, "Task %d reported with %d new tasks\n", tf->number, numNewNodes );
      
#if defined( DEBUG_DFS ) 
      cout << "Task: " << tf->number << " reported with " << numNewNodes 
	   << " new tasks: " << endl;
      for (int i = 0; i < numNewNodes; i++ )
	cout << (*newNodes[i] ) << endl;
#endif
                  
      for( int i = 0; i < numNewNodes; i++ )
	{
	  // Create a task for each of the nodes.
	  if( newNodes[i]->bestLowerBound + getStopTol() > bestUpperBound ){
	    //cout << " this node is fathomed " << endl;
	  }
	  else{
	    addTask( new FATCOP_task( newNodes[i], pseudocosts ) );
	  }
	}
      
      if( numNewNodes > 0 )
	delete [] newNodes;
      
    }
  
#if defined( DEBUGMW )
  print_task_keys();
#endif
  
    
  // Put the cuts in the cutpool
  //XXX These could be handled like pseudocosts -- by putting the 
  // address of the cutpool in the task.  Is there an advantage to doing it this way...
  cPool->insert( tf->newCuts, tf->numNewCuts );
  
  // Merge the new pseudocosts.
  // (This is done in unpack).
  // pseudocosts->mergeNew( tf->newPseudoCosts, tf->numNewPseudoCosts );
  
  
  // we can decide at this point whether switch node selection rule
  //if number of tasks is bigger than TASK_HI_THRESHOLD, send the deepest nodes
  //to workers
  if( seleStrategy == 4 ){ //hybrid strategy
    if( current_seleStrategy == 2 &&  //best bound currently
	get_number_tasks() > TASK_HI_THRESHOLD ){
      set_task_key_function( &deepest_first );
      sort_task_list();
      current_seleStrategy = 3;
      MWprintf( 20, "Driver Control. Switched from best bound to deepest first \n" );
    }
    
    //but we do not want to do deepest first for ever, so the program will
    //switch back after the number of tasks drops to some point
    if( current_seleStrategy == 3 && //deepest node currently
	get_number_tasks() < TASK_LOW_THRESHOLD ){
      set_task_key_function( &bb );
      sort_task_list();
      current_seleStrategy = 2;
      MWprintf( 20, "Driver Control. Switched from deepest first to best bound \n" );
    }
  }
    
  nNodes += tf->numNodesCompleted;
  lpPivots += tf->numLPPivots;
  solutionTime += tf->CPUTime;
  
  
  if(nNodes - lastlognNodes >= logFrequency){
    double bestLowerBound = getBestLowerBoundVal();
    int nActiveNodes = get_number_tasks();
    lastlognNodes = nNodes;
    cout << "STATUS: "
	 <<setiosflags(ios::left)<<setw(14)<<nNodes      
	 <<setiosflags(ios::left)<<setw(14)<<nActiveNodes
	 <<setiosflags(ios::left)<<setw(14)<<setprecision(6)<<bestLowerBound
	 <<setiosflags(ios::left)<<setw(14)<<setprecision(6)<<bestUpperBound
	 <<endl;
  }
  
  //check stop condition, should also check time limit?
  if(nNodes> maxNodes)
    {
      MWprintf( 10, "node limit reached!\n");
      printresults();
      status = QUIT;
    }
  if( maxTime > 0 )
    {
      double ctime = difftime( time(NULL), start_time );
      if( ctime + cumulativeTime > maxTime )
	{
	  MWprintf( 10, "Timelimit reached!\n" );
	  printresults();
	  status = QUIT;
	}
    }
  

  // Now update all of your strategy information.

    
  //XXX Also here update all of the cut and heuristic strategy information.

  nCutTrial += tf->nCutTrial;
  nCutSuccess += tf->nCutSuccess;
  cutTime += tf->cutTime;

  if( tf->nCutSuccess > 0 )
    {
      lastCutSuccess = nTasksReported;      
    }

  // Turn off cut generation if it's not working.

  const int TASK_CUT_OFF = 75;
  const int TASK_CUT_ON = 25;
  const double TASK_CUT_OFF_PCT = 0.025;
  
  if( genKnapCut + genFlowCut + genSKnapCut > 0 )
    {
      if( nTasksReported - lastCutSuccess > TASK_CUT_OFF || 
	  ( nTasksReported > TASK_CUT_ON && 
	    (double) nCutSuccess/nCutTrial < TASK_CUT_OFF_PCT ) )
	{
	  MWprintf( 10, " Driver Control.  Turning Off cut generation\n" );
	  genKnapCut = 0;
	  genFlowCut = 0;
	  genSKnapCut = 0;
	}
    }

  // Check on the status of the heuristic
  divingHeuristicTrial += tf->divingHeuristicTrial;
  divingHeuristicSuccess += tf->divingHeuristicSuccess;
  divingHeuristicTime += tf->divingHeuristicTime;

  // If the diving heuristic time is more than XXX percent
  //  of the total CPU time, you had better turn it off.

  const double DIVING_LO_PCT = 0.1;
  const double DIVING_HI_PCT = 0.2;
  if( originalDivingHeuristic > 0 )
    {
      if( divingHeuristic > 0 ) 
	{
	  if( divingHeuristicTime > DIVING_HI_PCT * solutionTime )
	    {
	      MWprintf( 10, " Driver Control.  Turning off diving heuristic: (%.1lf > %.1lf)\n", divingHeuristicTime,  DIVING_HI_PCT * solutionTime );
	      divingHeuristic = 0;
	    }
	  else
	    {
	      MWprintf( 70, " Driver Control.  Leaving heuristic on: (%.1lf < %.1lf)\n",
			divingHeuristicTime,  DIVING_HI_PCT * solutionTime );	
	    }
	}
      else
	{
	  if( divingHeuristicTime < DIVING_LO_PCT * solutionTime )
	    {
	      MWprintf( 10, " Driver Control.  Turning on diving heuristic: (%.1lf < %.1lf)\n", divingHeuristicTime,  DIVING_LO_PCT * solutionTime );
	      divingHeuristic = originalDivingHeuristic;
	    }
	}
    }

  // The branching rule
  branTime += tf->branTime;

  /* Here is a simple strategy for determining the grain size
   * If you don't have twice as many tasks as workers, 
   *  only allow workers to do one node, else
   *  allow them to work for 100 seconds
   */

  int nw = numWorkers();
  int nt = get_number_tasks();

  // Also we don't want to do the diving heuristic if there 
  // aren't enough nodes to go around. 
  if( nt < 2 * nw )
    {
      maxNodeInSlave = 1;
      if( ( originalDivingHeuristic > 0 ) && ( divingHeuristic > 0 ) )
	{
	  MWprintf( 10, "Turning off diving heuristic until we get enough nodes\n" );
	  divingHeuristic = 0;
	}
    }
  else
    {
      maxNodeInSlave = userMaxNodeInSlave;
    }

  lpsolveTime += tf->lpsolveTime;

  return status;
}


/// Put things in the send buffer here that go to a worker
MWReturn FATCOP_driver::pack_worker_init_data()
{

  MWReturn status = OK;

  // Tell the worker what LP solver to use.
  //  Right now we assume they all have CPLEX.  
  //  In the long run, probably unpack_worker_initinfo()
  //  will be redefined to unpack this information.
  

  int ncl = LPNumCPLEXLicensesInUse( cplexPointerFile );
  
  int nleft = numCplexLicenses - numCplexLeaveOpen - ncl;
  if( nleft < 0 ) nleft = 0;
  MWprintf( 20, "%d CPLEX licenses in use, %d left to assign\n", 
	    ncl, nleft );
  MWprintf( 20, "my program is using %d copies, we have a limit %d\n",
	    mycplexcopies, maxcplexcopies);
  
  int lpsolvertouse = altlpsolver; 
  
  if( nleft > 0  && mycplexcopies < maxcplexcopies)
    {
      lpsolvertouse = CPLEX;
      mycplexcopies++;
    }
  
  RMC->pack( &lpsolvertouse, 1, 1 );
  if( lpsolvertouse == CPLEX )
    {
      //XXX CPLEX license directory is  worker-specific, it shouln't
      //    be passed from the master.
      RMC->pack( cplexLicenseDir );
    }
  
  // Pack the formulation...
  formulation->MWPack( RMC );

  // Pack root LP basis (so it's easy to solve the root LP).
  int ncols = formulation->get_ncols();
  int nrows = formulation->get_nrows();
  RMC->pack( &ncols, 1, 1 );
  RMC->pack( &nrows, 1, 1 );
  RMC->pack( cstatRoot, ncols, 1 );
  RMC->pack( rstatRoot, nrows, 1 );
  
  // Pass it a cutoff value.
  RMC->pack( &bestUpperBound, 1, 1 );
  
  // Pass all the (original) strategy information that the Worker needs to know about.
  RMC->pack(&branStrategy,1,1);  
  RMC->pack(&genKnapCut,1,1);
  RMC->pack(&genFlowCut,1,1);  
  RMC->pack(&genSKnapCut,1,1);  
  RMC->pack(&maxCuts,1,1);
  
  RMC->pack(&divingHeuristic,1,1);  //1: do  0: no

  RMC->pack(&maxTimeInSlave,1,1);  
  RMC->pack(&maxNodeInSlave,1,1);  
  RMC->pack(&nodepreprocessing,1,1);  
  
  // pack all the cutting planes in the current pool.
  if( genKnapCut + genFlowCut + genSKnapCut >= 1 )
    {
      CCut** cuts;
      int nCuts;
      cPool->getCuts(cuts,nCuts);
      
      RMC->pack( &nCuts, 1, 1 ); 
      for( int i = 0; i < nCuts; i++ )
	cuts[i]->MWPack( RMC );
    }

  // Here we could/should pass pseudocosts when we figure out what to do
  //  with them

  if( pseudocosts && branStrategy == 0 )
    pseudocosts->MWPackWorking( RMC );
  
  return status;
  
}

/// tell the world the results, don't you? :-)
void FATCOP_driver::printresults()
{

  MWprintf( 10, "*** Best solution found is: %.6lf\n", bestUpperBound );
  MWprintf( 10, "*** Nodes: %d  Pivots: %d  Time:  %.2lf\n",
	    nNodes, lpPivots, solutionTime );

  MWprintf( 10, "*****  Cut Stats.  Success: %d  Trials: %d  Time: %.2lf\n",
	    nCutSuccess, nCutTrial, cutTime );
  MWprintf( 10, "*****  Heuristic Stats.  Success: %d  Trials: %d  Time: %.2lf\n",
	    divingHeuristicSuccess, divingHeuristicTrial, divingHeuristicTime );
  MWprintf( 10, "*****  LP Solving Stats.  Time: %.2lf\n", lpsolveTime );
  MWprintf( 10, "*****  Branching Stats.  Time: %.2lf\n", branTime );

  MWprintf( 10, "\n\n" );
  if( feasibleSolution )
    {
      for( int i = 0; i < formulation->get_nvars(); i++ )
	if( feasibleSolution[i] > 1.0e-6 || feasibleSolution[i] < -1.0e6 )
	  MWprintf( 10, "x[%d] = %.4lf\n", i, feasibleSolution[i] );
    }

  // ###qun not sure it is right: check condition program terminates
  // without resource limit
  if ( 0 == get_number_tasks() ){
    if ( bestUpperBound*10 < DBL_MAX ){ //feasible solution found
      st = SOLVED;
    }
    else{
      st  = INT_INFEASIBLE;
    }
  }
  else{
    if ( bestUpperBound*10 < DBL_MAX ){ //feasible solution found
      st = INT_SOLUTION;
    }
    else{
      st  = NO_INT_SOLUTION;
    }
  }
  
  
  // ###qun
#if defined( GAMS_INTERFACE )
  //8. get results
  calc_time = gfclck() - input_time;
  mipWork.getResults(&mipobj,x,mypi,slack,dj,colstat,rowstat);
  
  //9. write solutions
  write_solution();
  
  //10. just need to free solution
  free_memory() ;
  
  //11. tell condor job is done
  createFinishFile();
#endif
  
  return;
}

/** Write out the state of the master to an fp. */
void FATCOP_driver::write_master_state( FILE *fp )
{

  // Write everything out.
  fprintf( fp, "%d\n", nTasksReported );
  fprintf( fp, "%d\n", seleStrategy );
  fprintf( fp, "%d\n", TASK_HI_THRESHOLD );
  fprintf( fp, "%d\n", TASK_LOW_THRESHOLD );
  fprintf( fp, "%d\n", current_seleStrategy );
  fprintf( fp, "%d\n", branStrategy );
  fprintf( fp, "%f\n", branTime );
  fprintf( fp, "%f\n", lpsolveTime );
  fprintf( fp, "%d\n", presolLev );
  fprintf( fp, "%d\n", genKnapCut );
  fprintf( fp, "%d\n", genFlowCut );
  fprintf( fp, "%d\n", genSKnapCut );
  fprintf( fp, "%d\n", nCutTrial );
  fprintf( fp, "%d\n", nCutSuccess );
  fprintf( fp, "%f\n", cutTime );
  fprintf( fp, "%d\n", lastCutSuccess );
  fprintf( fp, "%d\n", maxCuts );
  fprintf( fp, "%d\n", nodepreprocessing );
  fprintf( fp, "%d\n", preprocTrial );
  fprintf( fp, "%d\n", preprocSuccess );
  fprintf( fp, "%f\n", preprocTime );
  fprintf( fp, "%f\n", absoluteGap );
  fprintf( fp, "%f\n", relativeGap );
  fprintf( fp, "%d\n", iniPseuCost );
  fprintf( fp, "%d\n", pseudoItlim );
  fprintf( fp, "%d\n", pseudoVarlim );
  fprintf( fp, "%d\n", divingHeuristic );
  fprintf( fp, "%d\n", originalDivingHeuristic );
  fprintf( fp, "%d\n", divingHeuristicTrial );
  fprintf( fp, "%d\n", divingHeuristicSuccess );
  fprintf( fp, "%f\n", divingHeuristicTime );
  fprintf( fp, "%d\n", preHeuristics );  
  fprintf( fp, "%d\n", checkpointType );
  fprintf( fp, "%d\n", checkpointFrequency );
  fprintf( fp, "%d\n", logFrequency );
  fprintf( fp, "%d\n", maxNodes );
  fprintf( fp, "%f\n", maxTime );
  // Write out the cumulative time so far
  cumulativeTime = difftime( time(NULL), start_time );
  fprintf( fp, "%f\n", cumulativeTime );
  fprintf( fp, "%f\n", maxPoolMem );
  fprintf( fp, "%d\n", maxWorkers );
  fprintf( fp, "%f\n", maxTimeInSlave );
  fprintf( fp, "%f\n", userMaxTimeInSlave );
  fprintf( fp, "%d\n", maxNodeInSlave );
  fprintf( fp, "%d\n", userMaxNodeInSlave );
  
  //t_num_arches?

  fprintf( fp, "%d\n", altlpsolver );
  fprintf( fp, "%d\n", numCplexLicenses );
  fprintf( fp, "%d\n", numCplexLeaveOpen );

  // mycplexcopies -- should be reset to 0?

  fprintf( fp, "%d\n", maxcplexcopies );
  fprintf( fp, "%s\n", cplexPointerFile );
  fprintf( fp, "%s\n", cplexLicenseDir );
  
  formulation->write( fp );
  if( cPool )
    {
      fprintf( fp, "%d\n", 1 );
      cPool->write( fp );
    }
  else
    {
      fprintf( fp, "%d\n", 0 );
    }

  if( pseudocosts )
    {
      fprintf( fp, "%d\n", 1 );
      pseudocosts->write( fp );
    }
  else
    {
      fprintf( fp, "%d\n", 0 );
    }

  fprintf( fp, "%f\n", bestUpperBound );  
  fprintf( fp, "%d\n", nNodes );
  fprintf( fp, "%d\n", lastlognNodes );
  fprintf( fp, "%d\n", lpPivots );
  fprintf( fp, "%f\n", solutionTime );
  fprintf( fp, "%f\n", zlpRoot );
  for( int i = 0; i < formulation->get_ncols(); i++ )
    fprintf( fp, "%f %f %d\n", xlpRoot[i], djRoot[i], cstatRoot[i] );
  for( int i = 0; i < formulation->get_nrows(); i++ )
    fprintf( fp, "%d\n", rstatRoot[i] );
  
  //Status st?

  if( feasibleSolution )
    {
      fprintf( fp, "%d\n", 1 );
      for( int i = 0; i < formulation->get_ncols(); i++ )
	fprintf( fp, "%f\n", feasibleSolution[i] );
    }
  else
    {
      fprintf( fp, "%d\n", 0 );
    }

  fprintf( fp, "%d\n", revobj );
  
  return;
}


/** Read the state from an fp.  This is the reverse of 
    write_master_state(). */
void FATCOP_driver::read_master_state( FILE *fp )
{

  // Read everything in.
  fscanf( fp, "%d", &nTasksReported );
  fscanf( fp, "%d", &seleStrategy );
  fscanf( fp, "%d", &TASK_HI_THRESHOLD );
  fscanf( fp, "%d", &TASK_LOW_THRESHOLD );
  fscanf( fp, "%d", &current_seleStrategy );
  fscanf( fp, "%d", &branStrategy );
  fscanf( fp, "%lf", &branTime );
  fscanf( fp, "%lf", &lpsolveTime );
  fscanf( fp, "%d", &presolLev );
  fscanf( fp, "%d", &genKnapCut );
  fscanf( fp, "%d", &genFlowCut );
  fscanf( fp, "%d", &genSKnapCut );
  fscanf( fp, "%d", &nCutTrial );
  fscanf( fp, "%d", &nCutSuccess );
  fscanf( fp, "%lf", &cutTime );
  fscanf( fp, "%d", &lastCutSuccess );
  fscanf( fp, "%d", &maxCuts );
  fscanf( fp, "%d", &nodepreprocessing );
  fscanf( fp, "%d", &preprocTrial );
  fscanf( fp, "%d", &preprocSuccess );
  fscanf( fp, "%lf", &preprocTime );
  fscanf( fp, "%lf", &absoluteGap );
  fscanf( fp, "%lf", &relativeGap );
  fscanf( fp, "%d", &iniPseuCost );
  fscanf( fp, "%d", &pseudoItlim );
  fscanf( fp, "%d", &pseudoVarlim );
  fscanf( fp, "%d", &divingHeuristic );
  fscanf( fp, "%d", &originalDivingHeuristic );
  fscanf( fp, "%d", &divingHeuristicTrial );
  fscanf( fp, "%d", &divingHeuristicSuccess );
  fscanf( fp, "%lf", &divingHeuristicTime );
  fscanf( fp, "%d", &preHeuristics );  
  fscanf( fp, "%d", &checkpointType );
  fscanf( fp, "%d", &checkpointFrequency );
  fscanf( fp, "%d", &logFrequency );
  fscanf( fp, "%d", &maxNodes );
  fscanf( fp, "%lf", &maxTime );

  fscanf( fp, "%lf", &cumulativeTime );
  fscanf( fp, "%lf", &maxPoolMem );
  fscanf( fp, "%d", &maxWorkers );
  fscanf( fp, "%lf", &maxTimeInSlave );
  fscanf( fp, "%lf", &userMaxTimeInSlave );
  fscanf( fp, "%d", &maxNodeInSlave );
  fscanf( fp, "%d", &userMaxNodeInSlave );
  
  //t_num_arches?

  fscanf( fp, "%d", &altlpsolver );
  fscanf( fp, "%d", &numCplexLicenses );
  fscanf( fp, "%d", &numCplexLeaveOpen );

  // mycplexcopies -- should be reset to 0?

  fscanf( fp, "%d", &maxcplexcopies );
  fscanf( fp, "%s", cplexPointerFile );
  fscanf( fp, "%s", cplexLicenseDir );

  formulation = new CProblemSpec;  
  formulation->read( fp );

  MWprintf( 50, "Read formulation from checkpoint file\n" );

  int tflag = 0;
  fscanf( fp, "%d", &tflag );
  if( tflag == 1 )
    {
      cPool = new cutPool(maxCuts);
      cPool->read( fp );
    }

  fscanf( fp, "%d", &tflag );
  if( tflag == 1 )
    {
      pseudocosts = new PseudoCostManager( formulation );
      pseudocosts->read( fp );
    }

  fscanf( fp, "%lf", &bestUpperBound );  
  fscanf( fp, "%d", &nNodes );
  fscanf( fp, "%d", &lastlognNodes );
  fscanf( fp, "%d", &lpPivots );
  fscanf( fp, "%lf", &solutionTime );
  fscanf( fp, "%lf", &zlpRoot );

  // Allocate the memory you need.

  xlpRoot = new double [formulation->get_ncols()];
  djRoot = new double [formulation->get_ncols()];
  cstatRoot = new CBasisStatus [formulation->get_ncols()];
  rstatRoot = new CBasisStatus [formulation->get_nrows()];

  

  for( int i = 0; i < formulation->get_ncols(); i++ )
    fscanf( fp, "%lf %lf %d", &(xlpRoot[i]), &(djRoot[i]), &(cstatRoot[i]) );
  for( int i = 0; i < formulation->get_nrows(); i++ )
    fscanf( fp, "%d", &(rstatRoot[i]) );
  
  //Status st?

  int fs = 0;
  fscanf( fp, "%d", &fs );
  if( fs == 1 )
    {
      feasibleSolution = new double[formulation->get_ncols()];
      for( int i = 0; i < formulation->get_ncols(); i++ )
	fscanf( fp, "%lf", &(feasibleSolution[i]) );
    }

  fscanf( fp, "%d", &revobj );
  
  return;
}


MWTask* FATCOP_driver::gimme_a_task()
{
  return new FATCOP_task;
}

void FATCOP_driver::pack_driver_task_data( void )
{
  RMC->pack( &bestUpperBound, 1, 1 );

  // What grain size would we like
  RMC->pack( &maxTimeInSlave, 1, 1 );
  RMC->pack( &maxNodeInSlave, 1, 1 );
  
  // Pack all strategy information
  RMC->pack( &genKnapCut, 1, 1 );
  RMC->pack( &genFlowCut, 1, 1 );
  RMC->pack( &genSKnapCut, 1, 1 );
  RMC->pack( &nodepreprocessing, 1, 1 );
  RMC->pack( &divingHeuristic, 1, 1 );

  // Pack the branching strategy information -- will you change this 
  //  dynamically?

  RMC->pack( &iniPseuCost, 1, 1 );
  RMC->pack( &pseudoItlim, 1, 1 );
  RMC->pack( &pseudoVarlim, 1, 1 );
    
}


void FATCOP_driver::createNodesFromStack( MIPNode ** &newNodes,
					  int &numNewNodes,
					  const MIPNode *startNode,
					  int stackDepth,
					  const MIPBranch *branchStack,
					  int backtracking )
{

  assert( backtracking >= 0 && backtracking <= 1 );
  // Figure out how many nodes you will have...
  numNewNodes = backtracking ? 0 : 1;  
  for( int i = 0; i < stackDepth; i++ )
    {
      if( branchStack[i].brotheropen == 1 )
	numNewNodes++;
    }

  if( numNewNodes == 0 )
    return;

  newNodes = new MIPNode * [numNewNodes];
  assert ( NULL != newNodes );
  
  int tmp = 0;
  for( int i = 0; i < stackDepth; i++ )
    {
      if( branchStack[i].brotheropen == 1 )
	{
	  // Create the node.
	  newNodes[tmp] = new MIPNode;
	  assert( NULL != newNodes[tmp] );

	  int nb = startNode->numBounds + i + 1;
	  newNodes[tmp]->numBounds = nb;
	  newNodes[tmp]->boundIx = new int [nb];
	  newNodes[tmp]->whichBound = new char [nb];
	  newNodes[tmp]->boundVal = new double [nb];

	  newNodes[tmp]->depth = nb;
	  newNodes[tmp]->bestLowerBound = branchStack[i].openLowerBound;
	  newNodes[tmp]->bestEstimate = branchStack[i].openEst;

      
	  // Copy "root" of tree to the current node
	  for( int j = 0; j < startNode->numBounds; j++ )
	    {
	      newNodes[tmp]->boundIx[j] = startNode->boundIx[j];
	      newNodes[tmp]->whichBound[j] = startNode->whichBound[j];
	      newNodes[tmp]->boundVal[j] = startNode->boundVal[j];
	    }

	  assert( NULL != newNodes[tmp]->boundIx    &&
		  NULL != newNodes[tmp]->whichBound &&
		  NULL != newNodes[tmp]->boundVal );
	  
	  int tmp2 = startNode->numBounds;
	  for( int j = 0; j < i; j++ )
	    {
	      if( branchStack[j].whichBound == 'L' )
		{
		  newNodes[tmp]->whichBound[tmp2] = 'L';
		  newNodes[tmp]->boundVal[tmp2] = (double) ( (int) branchStack[j].val + 1.0 );
		}
	      else if( branchStack[j].whichBound == 'U' )
		{
		  newNodes[tmp]->whichBound[tmp2] = 'U';
		  newNodes[tmp]->boundVal[tmp2] = (double) ( (int) branchStack[j].val );
		}
	      newNodes[tmp]->boundIx[tmp2++] = branchStack[j].idx;
	    }

	  assert( tmp2 == nb - 1 );

	  // Set bound for i (to the opposite)

	  if( branchStack[i].whichBound == 'L' )
	    {
	      newNodes[tmp]->whichBound[nb-1] = 'U';
	      newNodes[tmp]->boundVal[nb-1] = (double) ( (int) branchStack[i].val );
	    }
	  else if( branchStack[i].whichBound == 'U' )
	    {
	      newNodes[tmp]->whichBound[nb-1] = 'L';
	      newNodes[tmp]->boundVal[nb-1] = (double) ( (int) branchStack[i].val + 1.0 );
	    }
	  else
	    assert( 0 );
	  
	  newNodes[tmp]->boundIx[nb-1] = branchStack[i].idx;

	  tmp++;
	}
    }

  if( backtracking == 0 )
    assert( tmp == numNewNodes - 1 );
  else
    assert( tmp == numNewNodes );

  // Add the bottom node
  if( backtracking == 0 )
    {
      newNodes[numNewNodes-1] = new MIPNode;
      assert( NULL != newNodes[numNewNodes-1] );

      int nb = startNode->numBounds + stackDepth;
      newNodes[numNewNodes-1]->numBounds = nb;
      newNodes[numNewNodes-1]->boundIx = new int [nb];
      newNodes[numNewNodes-1]->whichBound = new char [nb];
      newNodes[numNewNodes-1]->boundVal = new double [nb];

      assert( NULL != newNodes[tmp]->boundIx    &&
		  NULL != newNodes[tmp]->whichBound &&
		  NULL != newNodes[tmp]->boundVal );

      newNodes[numNewNodes-1]->depth = nb;
      newNodes[numNewNodes-1]->bestLowerBound = branchStack[stackDepth-1].openLowerBound;
      newNodes[numNewNodes-1]->bestEstimate = branchStack[stackDepth-1].openEst;


      // Copy "root" of tree to the current node
      for( int j = 0; j < startNode->numBounds; j++ )
	{
	  newNodes[numNewNodes-1]->boundIx[j] = startNode->boundIx[j];
	  newNodes[numNewNodes-1]->whichBound[j] = startNode->whichBound[j];
	  newNodes[numNewNodes-1]->boundVal[j] = startNode->boundVal[j];
	}
	  
      int tmp2 = startNode->numBounds;
      for( int j = 0; j < stackDepth; j++ )
	{
	  if( branchStack[j].whichBound == 'L' )
	    {
	      newNodes[numNewNodes-1]->whichBound[tmp2] = 'L';
	      newNodes[numNewNodes-1]->boundVal[tmp2] = (double) 
		( (int) branchStack[j].val + 1.0 );
	    }
	  else if( branchStack[j].whichBound == 'U' )
	    {
	      newNodes[numNewNodes-1]->whichBound[tmp2] = 'U';
	      newNodes[numNewNodes-1]->boundVal[tmp2] = (double) ( (int) branchStack[j].val );
	    }
	  newNodes[numNewNodes-1]->boundIx[tmp2++] = branchStack[j].idx;
	}
    }
  
}


void FATCOP_driver::readOption( char* fileName = "fatcop.opt" )
{
  ifstream optfile(fileName);
  if(optfile){ //option file exist
    char* s = new char[256];
    
    //read option file line by line
    while(optfile.get(s,255,'\n')){
      optfile.ignore(1000,'\n'); 
      
      setOption(s);
    }

    delete[] s;
  }
  
  else{
    cout<<"option file does not exsit, use default options"<<endl;
  }
  
  optfile.close();
  

  return;
  
}



void FATCOP_driver::setOption(char*  s)
{

  //cout<<s<<endl;
  
  cString y(s), words[2];
  int level;
  double val;
    
  y.split(words,2);
  
  if(words[0].getLen() != 0) { //nonempty line
    
    //this is a comment
    if((words[0])[0]=='#') return;
    if((words[0])[0]==' ') return;
    
    if( words[0] == "workername" ) {

      RMC->set_worker_attributes( t_num_arches, words[1].getString(), NULL );
      
      MWprintf( 10, "Setting worker executable %d to %s\n",
		t_num_arches, words[1].getString() );
      
      t_num_arches++;
      
      return;
      
    }
    
    if(words[0] == "altlpsolver"){ 
 
      level = atoi((words[1]).getString());
      if( level < 1 || level > 3 ){
	MWprintf( 20, "invalid alternative lp solver option.\n");
	altlpsolver = SOPLEX;
      }
      else{
	altlpsolver = level;
      }
      
      return;
      
    }

    if(words[0] == "numcplexlicenses"){ 
      
      level = atoi((words[1]).getString());
      numCplexLicenses = level;
      return;
      
    }
    
    if(words[0] == "numcplexleaveopen"){ 
      
      level = atoi((words[1]).getString());
      numCplexLeaveOpen = level;
      return;

    }

    if(words[0] == "maxcplexcopies"){ 
      
      level = atoi((words[1]).getString());
      if( level >= 0 ){
	maxcplexcopies = level;
      }
      else{
	maxcplexcopies = 100;
      }
      return;
      
    }
   

    if( words[0] == "cplexpointerfile" ) {
      strcpy( cplexPointerFile, words[1].getString() );

      ifstream licfile;
      licfile.open( cplexPointerFile );
      
      licfile.getline( cplexLicenseDir, 255 );
      cout << " CPLEX license directory location: " << cplexLicenseDir << endl;
      licfile.close();

      return;
    }

    if(words[0] == "presolve"){ 
      
      level = atoi((words[1]).getString());
      
      if(level>=0 && level<3)
	presolLev = level;
      else
	cout<<"wrong presolve level, solver won't accept the option."<<endl;  
      
      return;
	  
    }

    if(words[0] == "nodepreprocessing"){ 
      
      level = atoi((words[1]).getString());
      
      if ( level < 0 ){
	cerr << "wrong nodepreprocessing option."<<endl;
      }
      else if ( level > 0 ){
	nodepreprocessing = 1;
      }
      else{
	nodepreprocessing = 0;
      }
      
      return;
    }
		
    if(words[0] == "nodsel"){
      level = atoi((words[1]).getString());
      
      if(level<=4&&level>=0)
	seleStrategy = level;
      else
	cout<<"wrong node selection strategy, solver won't accept the option."
	    <<endl; 
      
      return;
	    
    }
	
	
	
    if(words[0] == "varsel"){
      level = atoi((words[1]).getString());
      
      if(level<4&&level>=0)
	branStrategy = level;
      else
	cout<<"wrong variable selection strategy, solver won't accept the option."<<endl; 
      
      return;  
      
    }

    if(words[0] == "pseudo_itlim"){
      level = atoi((words[1]).getString());
      
      if( level < 0 )
	pseudoItlim = INT_MAX;
      else
	pseudoItlim = level;
      
      return;  
      
    }

    if(words[0] == "pseudo_varlim"){
      level = atoi((words[1]).getString());
      
      if( level < 0 )
	{ 
	  pseudoVarlim = INT_MAX;
	}
      else
	{
	  pseudoVarlim = level;
	}
      
      return;  
      
    }
   
    if(words[0] == "genknapcut"){
      level = atoi((words[1]).getString());
      
      if(level<2&&level>=0)
	genKnapCut = level;
      else
	cout<<"invalid option for generating cut, solver won't accept the option."<<endl; 
      
      return;  
      
    }

    if(words[0] == "genflowcut"){
      level = atoi((words[1]).getString());
      
      if(level<2&&level>=0)
	genFlowCut = level;
      else
	cout<<"invalid option for generating cut, solver won't accept the option."<<endl; 
      
      return;  
      
    }

    if(words[0] == "gensknapcut"){
      level = atoi((words[1]).getString());
      
      if(level<2&&level>=0)
	genSKnapCut = level;
      else
	cout<<"invalid option for generating cut, solver won't accept the option."<<endl; 
      
      return;  
      
    }

    if(words[0] == "maxCuts"){
      level = atoi((words[1]).getString());
      
      if(level>0)
	maxCuts = level;
      else
	cout<<"invalid option for maxCuts, solver won't accept the option."<<endl; 
      
      return;  
      
    }
    
    
    if(words[0] == "nodelim"){

      level = atoi((words[1]).getString());
	
      if(level>0)
	maxNodes=level;
      else
	{
	  cout<<"invalid node limit, solver won't accept the option."<<endl; 

	}
	  return;      
    }
    
    
    if(words[0] == "timelim"){

	val = atof((words[1]).getString());
	
	if(val>=0)
	  maxTime=val;
	else
	  cout<<"invalid time limit, solver won't accept the option."<<endl; 
      
	return;  
    }
    
    if(words[0] == "setrel"){

	val = atof((words[1]).getString());
	
	if(val>=0)
	  relativeGap = val;
	else
	  cout<<"relative gap should be positive, solver won't accept the option."<<endl; 
      
      return;
      
    }
    
    
    if(words[0] == "setabs"){

	val = atof((words[1]).getString());
	
	if(val>=0)
	  absoluteGap = val;
	else
	  cout<<"absolute gap should be positive, solver won't accept the option."<<endl; 

      
      
      return;
      
      
    }
	
    
    if(words[0] == "upperBound"){
      val = atof((words[1]).getString());
      
      bestUpperBound = val;
      
      return; 
	  
    }
	    
    if(words[0] == "way_of_ini_pseucost"){ 
      
      level = atoi((words[1]).getString());
      if(level>=0&&level<=2){
	iniPseuCost = level;
      }
      else{
	cout<<"invalid way_of_ini_pseucost, solver won't accept the option."<<endl;
      }
      
      return;
    }

    if(words[0] == "checkpointType"){ 
      
      level = atoi((words[1]).getString());
      if(level>=0&&level<=2){
	checkpointType = level;
      }
      else{
	cout<<"invalid checkpointType, solver won't accept the option."<<endl;
      }
      
      return;
    }

    if(words[0] == "checkpointFrequency"){ 
      
      level = atoi((words[1]).getString());
      if(level>=0 ){
	checkpointFrequency = level;
      }
      else{
	cout<<"invalid checkpointFrequency, solver won't accept the option."<<endl;
      }
      
      return;
    }

            
    if(words[0] == "logFrequency"){ 
      
      level = atoi((words[1]).getString());
      
      logFrequency = level;
     
      return;
    }
    
    
    if(words[0] == "preHeuristics"){ 
      //  cout<<(words[1]).getString()<<endl;
      level = atoi((words[1]).getString());
      if(level>1 || level<0){
	cout<<"invalid option for performing preHeuristics, fatcop won't accept the option."<<endl; 
      }
      else{
	preHeuristics = level;
      }
      
      return;
    }

    if(words[0] == "divingHeuristic"){ 

      level = atoi((words[1]).getString());
      if(level<0 || level > 1 ){
	cout<<"invalid option for performing divingHeuristic, fatcop won't accept the option."<<endl; 
      }
      else{
	divingHeuristic = level;
	originalDivingHeuristic = level;
      }
      
      return;
    }
      
    if(words[0] == "MaxWorkers"){ 
      //  cout<<(words[1]).getString()<<endl;
      level = atoi((words[1]).getString());
      if(level<1){
	cout<<"invalid option for initial # of host requests, fatcop won't accept the option."<<endl; 
      }
      else{
	maxWorkers = level;
      }
      
      return;
    }
    
    if(words[0] == "MaxTimeInSlave"){
      val = atof((words[1]).getString());
      maxTimeInSlave  = val;
      userMaxTimeInSlave = val;
      return;
    }
    

    if(words[0] == "MaxNodeInSlave"){
      level = atoi((words[1]).getString());
      maxNodeInSlave  = level;
      userMaxNodeInSlave = level;
      return;
    }
        
    if(words[0] == "MaxPoolMem"){ 
      
      val = atof((words[1]).getString());
      
      maxPoolMem = val;
      
      return;
    }
    
    
    cerr<<"wrong option: "<<(words[0]).getString()<<endl;
    
  }
  
  return;

}

void FATCOP_driver::load(char* fileName)
{ //load problem from a MPS file
  int m, n, nz;
  int objsen;
  double *obj;
  double *rhsx;
  char *senx;
  
  int *matbeg;
  int *matcnt;
  int *matind;
  double *matval;
  
  double *bdl;
  double *bdu;
  CColType* ctype;

  char *coltype;

  int stat = lpsolver->LPReadMPS( fileName ,&n, &m, &nz, 
				  &objsen, &obj, &rhsx, &senx, &matbeg, 
				  &matcnt, &matind, &matval, &bdl, &bdu, 
				  &coltype);

  if( stat != 0 )
    {
      cout << "LPsolver returned error reading MPS file!" << endl;
      formulation = NULL;
      return;
    }

  ctype = new CColType[n];
  assert( ctype != NULL );

  for(int i=0; i<n; i++)
    {
      if(coltype[i] == 'C')
	ctype[i] = COL_TYPE_CONTINUOUS;
      else if(coltype[i] == 'I')
	ctype[i] = COL_TYPE_INTEGER;    
      else
	ctype[i] = COL_TYPE_BINARY;
    }

  // Don't delete the LPSolver read memory -- it is used by the
  //  formulation.
  formulation = new CProblemSpec(objsen, n, m, nz, 
				 matbeg, matind, matcnt, matval, 
				 ctype, obj, bdl, bdu,
				 rhsx, senx);
  assert( formulation != NULL );
  
  
  return;
}

// ###qun
//load from matrix
void FATCOP_driver::load(int objsense, int n, int m, int nz, 
			 int*& matbeg, int*& matind, 
			 int*& matcnt, double*& matval, 
			 CColType*& ctype, double*& obj, 
			 double*& lb, double*& ub, 
			 double*& rhsx, char*& senx )
{
  int i;
  
  //make sure do minimization
  if(objsense != 1){ //max
    for(i=0; i<n; i++){
      obj[i] = -obj[i];
    }
    objsense = 1;
    revobj = true;
  }
  
  
  // Don't delete the LPSolver read memory -- it is used by the
  //  formulation.
  formulation = new CProblemSpec(objsense, n, m, nz, 
				 matbeg, matind, matcnt, matval, 
				 ctype, obj, lb, ub,
				 rhsx, senx);
  
  assert ( formulation != NULL );
  return;
}

double FATCOP_driver::getStopTol()
{

  double tmp = relativeGap*bestUpperBound;
  tmp = tmp >= 0 ? tmp : -tmp;
  
  tmp = (absoluteGap > tmp) ? absoluteGap : tmp;
  return tmp;

}

double FATCOP_driver::getBestLowerBoundVal()
{
  double retval = -DBL_MAX;
  //if( seleStrategy == 2 )
   // {
      double blbtd = return_best_todo_keyval();
      double blbrun = return_best_running_keyval();
	
      blbtd = blbtd > -DBL_MAX/100.0 ? blbtd : DBL_MAX;
      blbrun = blbrun > -DBL_MAX/100.0 ? blbrun : DBL_MAX;
      retval = ( blbtd < blbrun ? blbtd : blbrun );
   // }
 // else
  //  {
   //   cout << "BLB not implemented!" << endl;
   // }

  return retval;
}
