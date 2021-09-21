/* lshaped-driver.C

These methods specific to the LShaped application

*/

#include "MW.h"
#include "lshaped-driver.h"
#include "lshaped-worker.h"
#include "lshaped-task.h"
#include <iostream.h>
#include <fstream.h>
#include <pvm3.h>
#include <sys/resource.h>
#include <unistd.h>

#define PRINT( ofile, str, arr, n, max_n )  do { \
    (ofile) << (str) << ": ( " << (n) << " ) = ["; \
    if( !(arr) ) \
    { \
        (ofile) << "NULL"; \
    } \
    else \
    { \
        for( int k = 0; k < (n); k++ ) \
        { \
            if( k % (max_n) == 0 ) (ofile) << endl << "    "; \
            (ofile) << (arr)[k] << " "; \
        } \
    } \
    (ofile) << endl << "]" << endl; \
} while( 0 )

LShapedDriver::LShapedDriver() 
{
  mncols = tmncols = mnrows = mnnzero = UNDEFINED;
  mos = MINIMIZE;
  mobj = NULL;
  mrhs = NULL;
  mmatval = NULL;
  mbdl = NULL;
  mbdu = NULL;
  msense = NULL;
  mmatbeg = NULL;
  mmatind = NULL; 

  cutbufmatbeg = NULL;
  cutbufmatind = NULL;
  cutbufmatval = NULL;
  cutbufsense = NULL;
  cutbufrhs = NULL;
  cutbufn = cutbufmaxn = 0;
  cutbufnz = cutbufmaxnz = 0;
  
  mxlp = NULL;
  oldmxlp = NULL;
  mpi = NULL;
  slackCounter = NULL;

  maxNumCuts = 0;

  iteration = 0;
  feasibleSolutionValue = LPSOLVER_INFINITY;
  lowerBound = -LPSOLVER_INFINITY;
  
  converged = false;
  iterationStatus = new isstruct [config.MAX_ITERATIONS];

}

LShapedDriver::~LShapedDriver() 
{
  
  //!!! Most of these things were allocated by malloc, so use free

  if( mobj ) free( mobj );
  if( mrhs ) free( mrhs );
  if( mmatval ) free( mmatval );
  if( mbdl ) free( mbdl );
  if( mbdu ) free( mbdu );
  if( msense ) free( msense );
  if( mmatbeg ) free( mmatbeg );
  if( mmatind ) free( mmatind );

  if( cutbufmatbeg ) free( cutbufmatbeg );
  if( cutbufmatind ) free( cutbufmatind );
  if( cutbufmatval ) free( cutbufmatval );
  if( cutbufsense ) free( cutbufsense );
  if( cutbufrhs ) free( cutbufrhs );

  if( mxlp ) free( mxlp );  
  if( oldmxlp ) free( oldmxlp );
  if( mpi ) free( mpi );
  if( slackCounter ) free( slackCounter );
  
  itlog.close();

}

MWReturn LShapedDriver::get_userinfo( int argc, char *argv[] ) 
{

  MWReturn ustat = OK;

  if( argc != 2 )
    {      
      cout << "Usage: lshaped-driver <input_file>." << endl;
      ustat = ABORT;
      return ustat;
    }
  
  strcpy( config.input_file, argv[1] );

  FILE *fin = fopen( argv[1], "r" );
  if( ! fin )
    {
      cerr << "Cannot open input file" << config.input_file << endl;
      ustat = ABORT;
      return ustat;
    }
    
  read_input( fin );

  char logfilename[256];
  
  //XXX Probably should chop the stoch file name, and use it as the log name
  sprintf( logfilename, "%s.log", "iter" );
    
  itlog.open( logfilename, ios::app );

  if( config.sample_input )
    {
      // We don't do sampling yet.
      cerr << "We don't sample.  Sorry!" << endl;
      ustat = ABORT;
      return ustat;
    }

  // Read the data files
  spprob.readFiles(  config.core_filename, 
		     config.time_filename, 
		     config.stoch_filename );

  if( spprob.numPeriods != 2 )
    {
      cerr << "Sorry, Charlie.  This only works for two stage problems!"
	   << endl;
      ustat = ABORT;
      return ustat;
    }

  // Create the sceanrio tree
  spprob.createScenarioTree( config.sample_input );

  // Set up the stage I LP.
  spprob.getBaseLP( 0, &mncols, &mnrows, &mnnzero, &mos, 
		    &mobj, &mrhs, &msense, &mmatbeg, &mmatind, &mmatval,
		    &mbdl, &mbdu );


  // Write out some stats.
  double dec, der;  
  spprob.DeterministicEquivalentSize( &der, &dec );

  itlog << "*** Stage 1 LP has " << mnrows << " rows and " 
	<< mncols << " columns." 
	<< endl << "*** Deterministic Equivalent has " << der << " rows and "
	<< dec << " columns." << endl;
  
  // LPsolver is stupid and requires a "matcnt" -- I should change this.
  int *mmatcnt = (int *) malloc( mncols * sizeof( int ) );
  for( int i = 0; i < mncols; i++ )
    mmatcnt[i] = mmatbeg[i+1] - mmatbeg[i];

  // You must have at most maxNumTheta clumps (ever)
  //XXX You should think of a better way to do this
  maxNumTheta = 1024;
  tmncols = mncols + maxNumTheta;

  /// This is "spurious" if you are using SOPLEX
  const int max_cutnz = 1000000;

  masterlpsolver.CLPSolverInterface::init( mncols, mnrows, mnnzero, mos, mobj, 
					   mrhs, msense, mmatbeg, 
					   mmatcnt, mmatind, mmatval, 
					   mbdl, mbdu, NULL, 
					   mncols + maxNumTheta,
					   mnrows + config.maxCuts, 
					   mnnzero + max_cutnz ); 

  masterlpsolver.LPLoad();

  free( mmatcnt );

  // Create "theta" columns.  They are empty to begin with  
  double *theta_obj = new double[maxNumTheta];
  double *theta_lb = new double[maxNumTheta];
  double *theta_ub = new double[maxNumTheta];
    
  for( int i = 0; i < maxNumTheta; i++ )
    {
      theta_obj[i] = 1.0;
      theta_lb[i] = 0.0; /* You will change this once you get 
			    some optimality cuts
			 */
      theta_ub[i] = LPSOLVER_INFINITY;
    }
  
  masterlpsolver.LPAddEmptyColumns( maxNumTheta, theta_obj, 
				    theta_lb, theta_ub );

  mxlp = (double *) malloc( tmncols * sizeof( double ) );  
  oldmxlp = (double *) malloc( tmncols * sizeof( double ) );  
  mpi = (double *) malloc( (mnrows + config.maxCuts) * sizeof( double ) );
  slackCounter = (int *) malloc( (mnrows + config.maxCuts) * sizeof( int ) );

  // Allocate memory for your cut buffer

  cutbufmaxn = maxNumTheta;
  cutbufmaxnz = maxNumTheta * tmncols + maxNumTheta;  

  cutbufmatbeg = (int *) malloc( (cutbufmaxn + 1) * sizeof( int ) );
  cutbufmatind = (int *) malloc( (cutbufmaxnz) * sizeof( int ) );
  cutbufmatval = (double *) malloc( (cutbufmaxnz) * sizeof( double ) );
  cutbufsense = (char *) malloc( (cutbufmaxn) * sizeof( char ) );
  cutbufrhs = (double *) malloc( (cutbufmaxn) * sizeof( double ) );

  if( ( cutbufmatbeg == NULL ) || ( cutbufmatind == NULL ) || 
      ( cutbufmatval == NULL ) || ( cutbufsense == NULL ) ||
      ( cutbufrhs == NULL ) )
    {
      cout << "Error!  Can't allocate memory for cut buffer" 
	   << endl << "Exiting!" << endl;
      ustat = ABORT;
      return ustat;
    }

  cutbufn = 0;
  cutbufnz = 0;
  cutbufmatbeg[0] = 0;

#if 0
  // I don't think any of this is very accurate!
  // I'm going to use clocks_per_sec for this one...
  struct rusage resourceUsage;
  getrusage( RUSAGE_SELF, &resourceUsage );

  int begTime = resourceUsage.ru_utime.tv_usec + 
    resourceUsage.ru_stime.tv_usec;
#endif

  clock_t begTime = clock();
  
  masterlpsolver.LPSolvePrimal( &mzlp, mxlp, NULL, NULL, NULL );

  clock_t endTime = clock();

#if 0
  getrusage( RUSAGE_SELF, &resourceUsage );
  int endTime = resourceUsage.ru_utime.tv_usec + 
    resourceUsage.ru_stime.tv_usec;
#endif

  double mlpsolvetime = ( (double) ( endTime - begTime ) ) / CLOCKS_PER_SEC;
  if( mlpsolvetime < 0.01 )
    mlpsolvetime = 0.01;

  cout << "master lp took " << mlpsolvetime << " sec to solve" << endl;

  /*
   * Here begins the logic for determining the chunk size
   */

  int t_chunksize = 1;
  int ns = spprob.numScenarios( 1 );

  if( config.chunk_size_type == TARGET_TIME )
    {
      const double warm_start_speedup_factor = 2.0;

      int nrt;
      spprob.getTechnologyMatrix( 1, NULL, &nrt, NULL, NULL, NULL, 
				  NULL );

      t_chunksize = (int) ( (warm_start_speedup_factor * 
			     config.target_chunk_size_time * mnrows ) 
			    / ( mlpsolvetime * nrt ) );      
    }
  else if( config.chunk_size_type == PERCENTAGE_OF_SCENARIOS )
    {
      t_chunksize = (int) ( config.percent_scenarios_per_chunk * ns );
    }
  else if( config.chunk_size_type == EXPLICIT_NUMBER_OF_SCENARIOS )
    {
      t_chunksize = config.explicit_chunk_size;
    }
  else 
    {
      cerr << "Warning -- no valid chunk_size_type specified!"  
	   << " Using full multicut method" << endl;
    }

  t_chunksize = t_chunksize > ns ? ns : t_chunksize;
  t_chunksize = t_chunksize < 1 ? 1 : t_chunksize;

  setNumScenariosPerTask( t_chunksize );

#if defined( DEBUG_APP )
  cout << "Solved base: zlp = " << mzlp << endl;
#endif

  itlog << "The SP has " << ns << " scenarios.  Set initial chunksize to " 
	<< chunkSize << endl;

  /* Set the checkpointing as the user prescribes */
  if( config.checkpoint_type == TIME )
    {
      set_checkpoint_time( config.time_checkpoint_frequency );
    }
  else if( config.checkpoint_type == ITERATIONS )
    {
      set_checkpoint_frequency( tasksPerIt * config.it_checkpoint_frequency );
    }

  if( config.debug_level != -1 )
    {
      set_MWprintf_level( 75 );
    }

  /*
   * Zero out slack counter
   */

  for( int i = 0; i < mnrows; i++ )
    {
      // You can never remove the REAL rows!
      slackCounter[i] = -1; 
    }

  for( int i = mnrows; i < mnrows + config.maxCuts; i++ )
    {
      slackCounter[i] = 0;
    }


  // Solve the "expected value" problem to get a good mxlp
  // (This routine fills in mxlp)
  CLPStatus crashstat = Crash();
  if( crashstat != LP_OPTIMAL )
    {
      cerr << "Problems solving expected value problem.  This "
	   << "does not bode well..." << endl;
    }

  double fsc = 0.0;
  for( int i = 0; i < mncols; i++ )
    fsc += masterlpsolver.LPGetObjCoef( i ) * mxlp[i];

  // Fill in the iteration information 
  iterationStatus[iteration].startNew( tasksPerIt, fsc );
    
  delete [] theta_obj;
  delete [] theta_lb;
  delete [] theta_ub;

  // Set the number of hosts we are going to ask for in MWDriver
  if( config.target_num_hosts == -2 )
    {
      int nh = (int) ( 1.0/config.start_new_it_percent * tasksPerIt );
      set_target_num_workers( nh );      
    }
  else
    {
      set_target_num_workers( config.target_num_hosts );
    }


  // We don't have a key, but Jeff used this for testing
#if defined( TEST_TASKLIST_METHODS )
  // Set the task Key function
  set_task_key_function( &taskKey1 );
#endif
  
  return ustat;

}

MWReturn LShapedDriver::setup_initial_tasks(int *n_init , MWTask ***init_tasks) 
{
  MWReturn ustat = OK;

  int nspt = numScenariosPerTask();

  *n_init = tasksPerIt;
  *init_tasks = new MWTask *[tasksPerIt];

  int start_ix = 0; int end_ix = 0;
  for( int i = 0; i < *n_init; i++ )
    {
      if( i != *n_init - 1 )
	{
	  end_ix = start_ix + nspt - 1;
	}
      else
	{
	  end_ix = spprob.numScenarios( 1 ) - 1;
	}

      (*init_tasks)[i] = new LShapedTask( 0, start_ix, end_ix, mncols, mxlp,
					  -LPSOLVER_INFINITY, i );

      start_ix += nspt;
    }

  iterationFirstTaskIx = 0;
  iterationLastTaskIx = tasksPerIt - 1;

  return ustat;
}


MWReturn LShapedDriver::act_on_completed_task( MWTask *t ) 
{

  MWReturn ustat = OK;

#if defined( DUMB_COMPILER )
  LShapedTask *lst = (LShapedTask *) t;
#else
  LShapedTask *lst = dynamic_cast<LShapedTask *> ( t );
  assert( lst );
#endif

#if 0
  // This is a bit of defensive programming.
  // If you wish to test MW's task "rescheduling" 
  // (with which I think there is still a bug), then you should
  // comment this out.  
  // (It worked last time).  It will not work without "synchronous"
  //

  if( lst->number < iterationFirstTaskIx || lst->number > iterationLastTaskIx ) 
    {
      cout << " Error!  A task is reporting that shouldn't be." << endl
	   << "The results of this task " << lst->number 
	   << " will be ignored" << endl;
      return;
    }
#endif


  // Update statistics
  lshapedStats.Update( lst );

#if 1
  cout << "A task from iteration " << lst->iteration << endl;
#endif

  /// Update the status of the iteration table.
  bool completedIteration = iterationStatus[lst->iteration].update( lst );

  if( completedIteration )
    {

#if 1
      cout << "Iteration " << lst->iteration << " is completed" << endl;
#endif

      double fsc = iterationStatus[lst->iteration].fscost;
      double ssc = iterationStatus[lst->iteration].sscost;

#if 1
      cout << "fs cost was: " << fsc << " ss cost was: " << ssc
	   << endl;
#endif

      if( iterationStatus[lst->iteration].feasible )
	{
	  if( fsc + ssc < feasibleSolutionValue )
	    {
	      feasibleSolutionValue = fsc + ssc;
#if 1
	      itlog << "Found improved solution of value " 
		    << feasibleSolutionValue << endl;
#endif
	    }
	}

      double denom = feasibleSolutionValue > 0 ? feasibleSolutionValue 
		      : -feasibleSolutionValue;
      denom = denom > 1 ? denom : 1;

      double solgap = (feasibleSolutionValue - lowerBound) / denom;
      solgap *= 100;
      solgap = solgap > 0 ? solgap : -solgap;
  
      itlog << "  ****** Master Iteration " << iteration << " ******" << endl
	    << "   zlb: " << lowerBound
	    << "   zub: " << feasibleSolutionValue << endl;
      itlog << "   Gap: " << solgap << endl;
      itlog << "   Total Error: " <<  iterationStatus[lst->iteration].error 
	    << endl;
            	  
      int cur_nrows, cur_nz;
	  
      cur_nrows = masterlpsolver.LPGetActiveRows();
#if defined( SOPLEXv1 ) //XXX Fix this for the new LPsolver.
      cur_nz = masterlpsolver.nofNZEs();
#endif
	  
      itlog << "   current nrows: " << cur_nrows
	    << "   current nz: " << cur_nz << endl;
#if defined( DEBUG_APP )
      PRINT( itlog, "x", mxlp, tmncols, 10 );
#endif  

      itlog << "  *******************************" << endl 
	    << endl;

      if( solgap < config.convergeTol )
	{
	  itlog << endl << "CONVERGED within specified tolerance: ("
		<< config.convergeTol << ")" << endl;
	  converged = true;
	  ustat = QUIT;
	}      

      if( iterationStatus[lst->iteration].numCuts == 0 )
	{
	  itlog << "No cuts reported on iteration " 
		<< iteration << endl;
	  converged = true;
	  ustat = QUIT;
	}
    }

  if( ! converged )
    {

#if 0
      itlog << " Cuts from Task " << lst->number << ": " 
	    << lst->ncuts << endl;

      itlog << " This task contributed: " << lst->sscost
	    << "  to the second stage cost." << endl;
#endif

      /// Add cuts to the LP if necessary
      if( lst->ncuts > 0 )
	{
	  processCuts( lst );
	}

      // Are we going to start a new iteration?
      if( startNewIteration( iteration, lst->iteration ) )
	{

	  iteration++;

#if 1
	  itlog << " Starting iteration " << iteration << endl;
	  itlog << iterationStatus[iteration-1].numCuts 
		<< " cuts were received from the last iteration" 
		<< endl;
	  MWprintf( 30, "Starting new iteration %d\n", iteration );
#endif

	  if( iteration >= config.MAX_ITERATIONS )
	    {
	      cout << " You have reached the maximum number of iterations"
		   << endl;
	      ustat = QUIT;
	      goto EXIT;
	    }

#if 0
	  cout << "About to add cuts... " << endl;
	  for( int i = 0; i < cutbufn; i++ )
	    {
	      cout << "Cut: " << i << endl;
	      for( int j = 0;  j < cutbufmatbeg[i+1] - cutbufmatbeg[i]; j++ )
		{
		  cout << "a[" << cutbufmatind[cutbufmatbeg[i]+j] << "] = " 
		       <<  cutbufmatval[cutbufmatbeg[i]+j] << endl;
		}
	      cout << "  > " << cutbufrhs[i] << endl << endl;
	    }
#endif

	  // Load all your cuts in the current cut buffer to the LP solver.
	  masterlpsolver.LPAddRows( cutbufn, cutbufnz, cutbufrhs, cutbufsense, 
				    cutbufmatbeg, cutbufmatind, cutbufmatval );	  

	  
	  // Zero out the cutbuffer
	  cutbufn = 0;
	  cutbufnz = 0;
	  cutbufmatbeg[0] = 0;

	  // Solve the LP.

#if defined( DEBUG_APP2 )
	  masterlpsolver.LPWriteMPS( "master-lp" );
#endif	  

	  //XXX Do something about reading the cuts from disk
	  // (if you decide to do this)...


	  // For measuring the step length
	  memcpy( oldmxlp, mxlp, mncols * sizeof( double ) );

	  int mlpstat = masterlpsolver.LPSolvePrimal( &mzlp, mxlp, 
						      mpi, NULL, NULL );

	  if( lowerBound < mzlp ) // This should (almost) always be true
	    lowerBound = mzlp;

	  double stepl = 0.0;
	  double fsc = 0.0;

	  // You only wish to measure the step length in x (not theta)
	  for( int i = 0; i < mncols; i++ )
	    {
	      fsc += masterlpsolver.LPGetObjCoef( i ) * mxlp[i];
	      double tt = mxlp[i] - oldmxlp[i];
	      stepl += tt < 0 ? -tt : tt;
	    }

	  itlog << "   Last step norm: " << stepl << endl;

	  // Fill in the new iteration information
	  iterationStatus[iteration].startNew( tasksPerIt, fsc );

	  // Here delete the inactive rows
	  mlpstat = DeleteSomeRows();

	  if( mlpstat != LP_OPTIMAL )
	    {
	      cout << "Master LP not optimal.  What the!?!?!" << endl;
	      masterlpsolver.LPWriteMPS( "numlp1" );

	      mlpstat = ResolveNumericalIssues( );

	    }	    	   
	  
	  // Do another iteration -- only if you were able to
	  // resolve the numerical issues!

	  if( mlpstat != LP_OPTIMAL )
	    {
	      cout << "Master LP *still* not optimal." 
		   << endl << "I am quitting" << endl
		   << " Check numlp1 and numlp2 to debug" 
		   << endl;
	      masterlpsolver.LPWriteMPS( "numlp2" );
	      
	    }
	  else  // Create the new tasks
	    {
	      MWTask **new_tasks = new MWTask *[tasksPerIt];
		  
	      int start_ix = 0;
	      int end_ix = 0;
	      for( int i = 0; i < tasksPerIt; i++ )
		{
		  if( i != tasksPerIt - 1 )
		    {
		      end_ix = start_ix + numScenariosPerTask() - 1;
		    }
		  else
		    {
		      end_ix = spprob.numScenarios( 1 ) - 1;
		    }
		  
		  double theta_bdl = 0.0;
		  masterlpsolver.LPGetBdl( &theta_bdl, mncols + i, mncols + i );
		  if( theta_bdl < -1.0 )
		    {
		      new_tasks[i] = new LShapedTask( iteration, 
						      start_ix, end_ix,
						      mncols, mxlp,
						      mxlp[mncols+i], i);
		    }
		  else
		    {
		      new_tasks[i] = new LShapedTask( iteration,
						      start_ix, end_ix, 
						      mncols, mxlp,
						      -LPSOLVER_INFINITY, i);
		    }
		      
		  start_ix += numScenariosPerTask();
		}
	      
	      iterationFirstTaskIx = iteration * tasksPerIt;
	      iterationLastTaskIx = (iteration+1) * tasksPerIt - 1;
	      
	      addTasks( tasksPerIt, new_tasks );	  
	      delete [] new_tasks;
		  

	    }
	}
    }

 EXIT:
  return ustat;
}


MWReturn LShapedDriver::pack_worker_init_data( void ) 
{  
  MWReturn ustat = OK;

  spprob.packInPVMBuffer();

  return ustat;
}

void LShapedDriver::printresults() 
{
  itlog << endl << "**************************************************" << endl;
  itlog << "Completed! " << endl;
  int oldp = itlog.precision( 16 );  
  itlog << "Optimal Solution: " << mzlp << endl << endl;
  itlog.precision( oldp );

  itlog << "Master Iterations: " << iteration << endl;

#if defined( PRINT_SOL )
  for( int i = 0; i < mncols; i++ )
    itlog << "xlp[" << i << "] = " << mxlp[i];
#endif

  itlog << "Maximum #Cuts: " << maxNumCuts << endl;

  itlog << lshapedStats << endl;

  itlog << endl << "**************************************************" << endl;

}

void LShapedDriver::write_master_state( FILE *fp )
{

  // Write the input file so that you can call getuserinfo()
  // to re-setup most of the information.
  fprintf( fp, "%s\n", config.input_file );

  fprintf( fp, "%d %d %d %d %f %f\n", tasksPerIt, chunkSize,
	   maxNumTheta, iteration, feasibleSolutionValue,
	   lowerBound );
  
  // Write the cuts.
  int nrows = masterlpsolver.LPGetActiveRows();

  // (tmncols is with the thetas!)
  double *cutval = new double [tmncols];
  int *cutind = new int [tmncols];
  
  fprintf(fp, "%d\n", nrows - mnrows );
  for( int i = mnrows; i < nrows; i++ )
    {
      int nz;
      int rmatbeg[2];

      int stat = masterlpsolver.LPGetRows( &nz, rmatbeg, cutind, cutval, tmncols,
					   i, i );
      fprintf( fp, "%d\n", nz );
      for( int j = 0; j < nz; j++ )
	{
	  fprintf( fp, "%d %.12e\n", cutind[j], cutval[j] );
	}      

      char s;
      stat = masterlpsolver.LPGetSense( &s, i, i );
      fprintf( fp, "%c\n", s );
      
      double r;
      stat = masterlpsolver.LPGetRHS( &r, i, i );
      fprintf( fp, "%.12e\n", r );
      
    }

  delete [] cutval;
  delete [] cutind;

  fprintf( fp, "%d\n", converged ? 1 : 0 );
  write_iterationStatus( fp );

  // Write the "stats" class
  lshapedStats.write( fp );

       
}

void LShapedDriver::read_master_state( FILE *fp )
{
  char ifile[256];
  char **av = new char * [2];
  av[1] = ifile;

  fscanf( fp, "%s", ifile );

  MWprintf(10, " Starting checkpoint with input file: %s\n", ifile );

  // This sets up the initial base LP, the scenario tree,
  // solves the LP, and figures out the chunk size.
  get_userinfo( 2, av );
  delete [] av;
  itlog << endl << " *** Restarting from checkpoint *** " << endl << endl;

  // This will reset the chunk size to what it was before
  // (NOT what the get_userinfo() just did.  This is good
  // since otherwise the chunk sizes might be different

  fscanf( fp, "%d %d %d %d %lf %lf", &tasksPerIt, &chunkSize,
	  &maxNumTheta, &iteration, &feasibleSolutionValue,
	  &lowerBound );

  printf(" Resetting chunk size to previous value of %d\n", chunkSize );
  
  // Read in all the cuts in the LP.

  int ncuts;
  fscanf( fp, "%d", &ncuts );

  // Now read all the cuts you added
  // Don't add the rows one at a time -- it takes up too much memory
  // You may as well assume all the cuts to be dense

  int maxnzcuts = ncuts * tmncols + ncuts;

  double *cutval = new double [maxnzcuts];
  int *cutind = new int [maxnzcuts];
  int *cutbeg = new int [ncuts+1];
  char *cutsense = new char [ncuts];
  double *cutrhs = new double [ncuts];

  if( cutval == 0 || cutind == 0 || cutbeg == 0 ) 
    {
      cout << "Unable to allocate memory to start from checkpoint\n";
      exit( 1 );
    }

  int nz = 0;
  for( int i = 0; i < ncuts; i++ )
    {
      cutbeg[i] = nz;

      int tnz;
      int tind;
      double tval;
      fscanf( fp, "%d", &tnz );

      for( int j = 0; j < tnz; j++ )
	{
	  fscanf( fp, "%d %le", &tind, &tval );
	  cutind[nz] = tind;
	  cutval[nz++] = tval;
	}

      double r;
      char s[1];
      fscanf( fp, "%s", s );
      fscanf( fp, "%le", &r );

      cutsense[i] = s[0];
      cutrhs[i] = r;
      
    }

  cutbeg[ncuts] = nz;
  masterlpsolver.LPAddRows( ncuts, nz, cutrhs, cutsense, cutbeg, 
			    cutind, cutval );

  // Resolve the LP.
  masterlpsolver.LPSolveDual( &mzlp, mxlp, NULL, NULL, NULL );
  printf("After all cuts reloaded, master LP has value: %f\n", mzlp );


  delete [] cutbeg;
  delete [] cutval;
  delete [] cutind;
  delete [] cutsense;
  delete [] cutrhs;

  int t_converged;
  fscanf( fp, "%d", &t_converged );
  converged = t_converged ? true : false;

  // Read the iteration history
  read_iterationStatus( fp );

  // Read the slackcounter array
  // Let's just forget about it...
  // fscanf( "%d", &maxNumCuts );
  
  lshapedStats.read( fp );
  
}

MWTask * LShapedDriver::gimme_a_task()
{
  return new LShapedTask;
}

// Beginning of non-virtual functions

int LShapedDriver::numScenariosPerTask()
{
  return( chunkSize );
}

void LShapedDriver::processCuts( LShapedTask *lst )
{
  for( int i = 0; i < lst->ncuts; i++ )
    {

      if( lst->cutType[i] == FEASIBILITY_CUT )
	{
	  for( int j = 0; j < lst->cutbeg[i+1] - lst->cutbeg[i]; j++ )
	    {
	      cutbufmatind[cutbufnz] = lst->cutind[lst->cutbeg[i]+j];
	      cutbufmatval[cutbufnz++] = lst->cutval[lst->cutbeg[i]+j];
	    }
	  cutbufsense[cutbufn] = 'G';
	  cutbufrhs[cutbufn] = lst->cutrhs[i];
	  cutbufn++;
	  cutbufmatbeg[cutbufn] = cutbufnz;
	}
      else if( lst->cutType[i] == OPTIMALITY_CUT )
	{
	  
	  // Reset the bound on theta if necessary
	  double tbdl = 0.0;
	  masterlpsolver.LPGetBdl( &tbdl,  mncols + lst->thetaIx,  
				   mncols + lst->thetaIx );
	  if( tbdl > -1.0 )
	    {
	      masterlpsolver.LPSetOneBdl( -LPSOLVER_INFINITY, mncols + lst->thetaIx );
	    }
	  
	  for( int j = 0; j < lst->cutbeg[i+1] - lst->cutbeg[i]; j++ )
	    {
	      cutbufmatind[cutbufnz] = lst->cutind[lst->cutbeg[i]+j];
	      cutbufmatval[cutbufnz++] = lst->cutval[lst->cutbeg[i]+j];
	    }
	  
	  // Add the theta coefficient
	  cutbufmatind[cutbufnz] = mncols + lst->thetaIx;
	  cutbufmatval[cutbufnz++] = 1.0;
	  
	  cutbufsense[cutbufn] = 'G';
	  cutbufrhs[cutbufn] = lst->cutrhs[i];
	  cutbufn++;
	  cutbufmatbeg[cutbufn] = cutbufnz;
	}
      else
	{
	  assert( 0 );
	}
      
      // A bit of defensive programming
      if( ( cutbufn > cutbufmaxn ) || ( cutbufnz > cutbufmaxnz ) )
	{
	  cout << "OH NO!  You over-ran your cut buffers.  I am quitting" 
	       << endl;
	  exit( 666 );
	}

#if 0
      masterlpsolver.LPAddRows( 1, cut_nnz, &cutrhs, &tmp_sense, cutbeg,
				cutind, cutval );
#endif

    }
}

bool LShapedDriver::startNewIteration( int currentIt, int itJustReported )
{

  // If alpha = 75 percent of the workers have found cuts
  // (for the latest iteration)
  // start a new iteration.  This assumes only one cut 
  // maximum per worker!

  int nCutsStartNew = (int) ( config.start_new_it_percent * 
			      iterationStatus[currentIt].numChunksPerIt );
  nCutsStartNew = nCutsStartNew < 1 ? 1 : nCutsStartNew;

  if( iterationStatus[currentIt].numCuts >= nCutsStartNew )
    return true;
  else
    return false;

#if 0
  // This one is for a synchronous implementation
  return ( iterationStatus[itJustReported].numChunksCompleted >= 
	   iterationStatus[itJustReported].numChunksPerIt );
#endif
}

void LShapedDriver::setNumScenariosPerTask( int n )
{
  int ns = spprob.numScenarios( 1 );
  chunkSize = n;
  tasksPerIt = ns / chunkSize + ( ns % chunkSize ? 1 : 0 );

  if( tasksPerIt >= maxNumTheta )
    {
      tasksPerIt = maxNumTheta;
      chunkSize = ns / tasksPerIt;
      cout << "Warning!  Setting chunksize to " << chunkSize
	   << " to accomidate max column size of "
	   << maxNumTheta << endl;
    }

}

//XXX This will now only work for SOPLEX!
#if defined( SOPLEXv1 )
CLPStatus LShapedDriver::DeleteSomeRows()
{
  // epsilon for counter whether or not row is slack
  const double ROW_REMOVE_TOL = (1e-8);

  // First adjust the counters

  int m = masterlpsolver.LPGetActiveRows();
  assert( m >= mnrows );

  if( m > mnrows + config.maxCuts )
    {
      cout << "** You have added more than maxCuts cuts to the formulation"
	   << endl << "    Cut deletion will no longer work properly!" 
	   << endl;
    }

  if( m - mnrows > maxNumCuts )
    {
      maxNumCuts = m - mnrows;
    }

  for( int i = mnrows; i < m; i++ )
    {
      if( ( mpi[i] > ROW_REMOVE_TOL) || ( mpi[i] < -ROW_REMOVE_TOL ) )
	{
	  slackCounter[i] = 0;
	}
      else
	{
	  (slackCounter[i])++;
	}
    }

  // Now remove the rows!

  int *delstat = new int [m];
  int *perm = new int [m];

  for( int i = 0; i < m; i++ )
    delstat[i] = 0;

  int delcnt = 0;
  for( int i = mnrows; i < m; i++ )
    {
      if( slackCounter[i] > config.cutDelBnd )
	{
#if defined( DEBUG_CUTREMOVAL )
	  cout << " Deleting row: " << i << endl;
#endif
	  delstat[i] = 1;
	  delcnt++;
	}
    }

  cout << "Deleting " << delcnt << " rows that have become slack" << endl;

  // WriteCutsToDisk( delstat );

  CLPStatus stat = LP_OPTIMAL;
  if( delcnt > 0 )
    {
      
#if defined( DEBUG_CUTREMOVAL )
      cout << "Before, slackCounter is: " << endl;
      for( int i = 0; i < m; i++ )
	cout << "sc[" << i << "] = " << slackCounter[i] << endl;
#endif
      
      masterlpsolver.LPDeleteRows( delstat, perm );      
      PushSlackCounter( m, perm );

#if defined( DEBUG_CUTREMOVAL )
      cout << endl << "After, slackCounter is: " << endl;
      for( int i = 0; i < m; i++ )
	cout << "sc[" << i << "] = " << slackCounter[i] << endl;
#endif
      
      stat = masterlpsolver.LPSolveDual( &mzlp, mxlp, mpi, NULL, NULL );
    }
	      
  delete [] delstat;  
  delete [] perm;
  return stat;
  
}
#endif

CLPStatus LShapedDriver::ResolveNumericalIssues()
{

  const double REMOVE_TOL = 1.0;

  int m = masterlpsolver.LPGetActiveRows();
  assert( m >= mnrows );

  double *slack = new double[m];

  int stat = masterlpsolver.LPSolvePrimal( &mzlp, mxlp, NULL, slack, NULL );
  if( stat == LP_OPTIMAL )
    {
      cout << "Yahoo!  The LP is optimal now" << endl;
    }
  else
    {

      char *sense = new char [m];
      int *delstat = new int [m];
      for( int i = 0; i < m; i++ )
	delstat[i] = 0;

      // This is a temporary fix -- delete all cuts with
      // slack is bigger than REMOVE_TOL

      masterlpsolver.LPGetSense( sense, 0, m-1 );
      
      int delcnt = 0;
      for( int i = mnrows; i < m; i++ )
	{
	  if( ( sense[i] == 'L' ) && ( slack[i] > REMOVE_TOL ) )
	    {
	      delstat[i] = 1;
	      delcnt++;
	    }
	  else if( ( sense[i] == 'G' ) && ( slack[i] < -REMOVE_TOL ) )
	    {
	      delstat[i] = 1;
	      delcnt++;
	    }
	  else
	    {
	      delstat[i] = 0;
	    }
	}

      cout << "Deleting " << delcnt << " rows in order to make LP easier";

      masterlpsolver.LPDeleteRows( delstat );
      
      stat = masterlpsolver.LPSolvePrimal( &mzlp, mxlp, mpi, NULL, NULL );
	      
      delete [] sense;
      delete [] delstat;

    }

  delete [] slack;
  return stat;  
}

#if defined( SOPLEXv1 )
void LShapedDriver::PushSlackCounter( int oldm, int perm[] )
{
  for( int i = 0; i < oldm; i++ )
    {
#if defined( DEBUG_CUTREMOVAL )
      cout << " perm[" << i << "] = " << perm[i] << endl;
      if( perm[i] >= 0 )
	{
	  slackCounter[perm[i]] = slackCounter[i];
	}
#endif
    }
}
#elif defined( CPLEX )
// The correctness of this function is based on the fact that
// LP solvers will renumber the rows by "pushing them up".
// CPLEX is this way, SOPLEX is NOT!!!!  I need to fix it.

void LShapedDriver::PushSlackCounter( void )
{
  // You haven't deleted the rows yet, so this is OK
  int m = masterlpsolver.LPGetActiveRows();
  int count = m;

  for( int i = mnrows; i < count; i++ )
    {
      if( slackCounter[i] > config.cutDelBnd ) 
	{
	  for( int j = i + 1; j < count; j++ )
	    slackCounter[j-1] = slackCounter[j];
	  slackCounter[count--] = 0;
	}
    }  
}
#endif

CLPStatus LShapedDriver::Crash()
{
#if defined( SOPLEXv1 )
  CSOPLEXInterface crashlpsolver;
#else
#error "No LP Solver for Crash!"
#endif

  int ncols, nrows, nnzero, os;
  double *obj = 0;
  double *rhs = 0;
  char *sense = 0;
  int *matbeg = 0;
  int *matind = 0;
  double *matval = 0;
  double *bdl = 0;
  double *bdu = 0;
  
  spprob.getExpectedValueProblem( &ncols, &nrows,
				  &nnzero, &os, &obj, 
				  &rhs, &sense, &matbeg, &matind, &matval,
				  &bdl, &bdu );

  int *matcnt = (int *) malloc( ncols * sizeof( int ) );
  for( int i = 0; i < ncols; i++ )
    matcnt[i] = matbeg[i+1] - matbeg[i];
  
  // Load it
  crashlpsolver.CLPSolverInterface::init( ncols, nrows, nnzero, os, obj, 
					  rhs, sense, matbeg, 
					  matcnt, matind, matval, 
					  bdl, bdu, NULL );


#if defined( DEBUG_CRASH )
  PRINT( cout, "obj", obj, ncols, 10 );
  PRINT( cout, "rhs", rhs, nrows, 10 );
  PRINT( cout, "sense", sense, nrows, 10 );
  PRINT( cout, "matbeg", matbeg, (ncols+1), 10 );
  PRINT( cout, "matcnt", matcnt, ncols, 10 );
  PRINT( cout, "matind", matind, nnzero, 10 );
  PRINT( cout, "matval", matval, nnzero, 10 );
  PRINT( cout, "bdl", bdl, ncols, 10 );
  PRINT( cout, "bdu", bdu, ncols, 10 );
#endif

  crashlpsolver.LPLoad();

  free( matcnt );

  // Solve it (fill in mxlp)

  double crashobj;
  double *cxlp = (double *) malloc( ncols * sizeof( double ) );

  CLPStatus stat = crashlpsolver.LPSolvePrimal( &crashobj, cxlp, NULL, NULL, NULL );

  cout << " Expected value problem has objective: " << crashobj << endl;

  // copy cxlp into mxlp

  assert( mncols <= ncols );
  for( int i = 0; i < mncols; i++ )
    {
      mxlp[i] = cxlp[i];
    }

  crashlpsolver.LPWriteMPS( "eval.mps" );

  // Free memory

  if( cxlp ) free( cxlp );
  if( obj ) free( obj );
  if( rhs ) free( rhs );
  if( sense ) free( sense );
  if( matbeg ) free( matbeg );
  if( matind ) free( matind );
  if( matval ) free( matval );
  if( bdl ) free( bdl );
  if( bdu ) free( bdu );

  // Return solve status

  return stat;  

}


/*
void LShapedDriver::WriteCutsToDisk( int delstat[] )
{
  int m = masterlpsolver.LPGetActiveRows();


}
*/


LShapedDriver::isstruct::isstruct()
{
  fscost = sscost = 0.0;
  feasible = true;
  numChunksCompleted = 0;
  numChunksPerIt = 0;
  numCuts = 0;
}

LShapedDriver::isstruct::~isstruct()
{
}

void LShapedDriver::isstruct::startNew( int nc, double fsc )
{
  fscost = fsc;
  numChunksPerIt = nc;
  numChunksCompleted = 0;
  sscost = 0;
  error = 0;
  numCuts = 0; 
  feasible = true;  
}


bool LShapedDriver::isstruct::update( LShapedTask *lst )
{

  numChunksCompleted++;
  numCuts += lst->ncuts;

  for( int i = 0; i < lst->ncuts; i++ )
    {
      if( lst->cutType[i] == FEASIBILITY_CUT )
	{
	  feasible = false;
	  break;
	}
    }
  
  error += lst->error;
  sscost += lst->sscost;

#if 1
  cout << "Task reported.  ncuts = " << lst->ncuts << " error = " << lst->error
       << " fscost = " << fscost << " sscost = " << lst->sscost << endl;
#endif
    
  

  return( numChunksCompleted >= numChunksPerIt );

}

/****************************************************************************
*
****************************************************************************/
void LShapedDriver::write_iterationStatus( FILE *fp )
{
  for( int i = 0; i < iteration; i++ )
    {
      fprintf( fp, "%d %d %f %f %f %d %d\n", 
	       iterationStatus[i].numChunksCompleted,
	       iterationStatus[i].numChunksPerIt,
	       iterationStatus[i].fscost,
	       iterationStatus[i].sscost, 
	       iterationStatus[i].error,
	       iterationStatus[i].feasible ? 1 : 0,
	       iterationStatus[i].numCuts );
    }
}

/****************************************************************************
*
****************************************************************************/
void LShapedDriver::read_iterationStatus( FILE *fp )
{
  for( int i = 0; i < iteration; i++ )
    {
      int t_feas;
      fscanf( fp, "%d %d %lf %lf %lf %d %d\n", 
	      &(iterationStatus[i].numChunksCompleted),
	      &(iterationStatus[i].numChunksPerIt),
	      &(iterationStatus[i].fscost),
	      &(iterationStatus[i].sscost), 
	      &(iterationStatus[i].error),
	      &t_feas,
	      &(iterationStatus[i].numCuts) );
      iterationStatus[i].feasible = t_feas ? true : false;
    }

}

/****************************************************************************
*
****************************************************************************/
bool LShapedDriver::read_input( FILE *fp )
{
  int t_num_arches = 0;

  bool success = true;
  char buf[2000];
  while( fgets( buf, sizeof( buf ), fp ) )
    {
      int len = strlen( buf );
      if( buf[len-1] == '\n' || buf[len-1] == '\r' ) buf[len-1] = 0;
      len = strlen( buf );
      if( !len || buf[0] == '#' ) continue;

      char *s;
      char t_exe_name[256];
      if( strstr( buf, "NUM_ARCHES" ) && (s = strstr( buf, "=" )) )
	{
	  sscanf(strchr(s, '=')+1, "%d", &config.num_arches);
	}      
      else if( strstr( buf, "EXE_NAME" ) && ( s = strstr( buf, "=" ) ) )
	{
	  sscanf( strchr( s, '=' )+1, "%s", t_exe_name );
	  set_worker_executable( t_exe_name, t_num_arches++ );
	}
      else if( strstr( buf, "CORE_FILE" ) && ( s = strstr( buf, "=" ) ) )
	{
	  sscanf( strchr( s, '=' )+1, "%s", config.core_filename );
	}
      else if( strstr( buf, "TIME_FILE" ) && ( s = strstr( buf, "=" ) ) )
	{
	  sscanf( strchr( s, '=' )+1, "%s", config.time_filename );
	}
      else if( strstr( buf, "STOCH_FILE" ) && ( s = strstr( buf, "=" ) ) )
	{
	  sscanf( strchr( s, '=' )+1, "%s", config.stoch_filename );
	}
      else if( strstr(buf, "SYNCH_COEF") && (s = strstr( buf, "=" )) )
	{
	  sscanf( strchr( s, '=' )+1, "%lf", &config.start_new_it_percent);
	}
      else if( strstr( buf, "CHUNK_SIZE_TYPE" ) && strstr( buf, "=" ) )
	{
	  if( strstr( buf, "TARGET_TIME" ) )
	    config.chunk_size_type = TARGET_TIME;
	  if( strstr( buf, "PERCENTAGE_OF_SCENARIOS" ) )
	    config.chunk_size_type = PERCENTAGE_OF_SCENARIOS;
	  if( strstr( buf, "EXPLICIT_NUMBER_OF_SCENARIOS" ) )
	    config.chunk_size_type = EXPLICIT_NUMBER_OF_SCENARIOS;
	}
      else if( strstr(buf, "TARGET_CHUNK_SIZE_TIME") && (s = strstr( buf, "=" )) )
	{
	  sscanf( strchr( s, '=' )+1, "%lf", &config.target_chunk_size_time );
	}      
      else if( strstr(buf, "PERCENTAGE_CHUNK_SIZE") && (s = strstr( buf, "=" )) )
	{
	  sscanf( strchr( s, '=' )+1, "%lf", &config.percent_scenarios_per_chunk );
	}      
      else if( strstr( buf, "CHUNK_SIZE" ) && (s = strstr( buf, "=" )) )
	{
	  sscanf(strchr(s, '=')+1, "%d", &config.explicit_chunk_size);
	}      
      else if( strstr(buf, "CONVERGE_TOL") && (s = strstr( buf, "=" )) )
	{
	  sscanf( strchr( s, '=' )+1, "%lf", &config.convergeTol );
	}      
      else if( strstr( buf, "DELETE_USELESS_CUTS_AFTER" ) && (s = strstr( buf, "=" )) )
	{
	  sscanf(strchr(s, '=')+1, "%d", &config.cutDelBnd);
	}
      else if( strstr( buf, "MAXCUTS" ) && (s = strstr( buf, "=" )) )
	{
	  sscanf(strchr(s, '=')+1, "%d", &config.maxCuts);
	}
      else if( strstr( buf, "CHECKPOINT_TYPE" ) && strstr( buf, "=" ) )
	{
	  if( strstr( buf, "TIME" ) )
	    config.checkpoint_type = TIME;
	  if( strstr( buf, "ITERATIONS" ) )
	    config.checkpoint_type = ITERATIONS;
	}
      else if( strstr( buf, "CHECKPOINT_IT_FREQ" ) && (s = strstr( buf, "=" )) )
	{
	  sscanf(strchr(s, '=')+1, "%d", &config.it_checkpoint_frequency );
	}
      else if( strstr(buf, "CHECKPOINT_TIME_FREQ") && (s = strstr( buf, "=" )) )
	{
	  sscanf( strchr( s, '=' )+1, "%lf", &config.time_checkpoint_frequency );
	}      
      else if( strstr( buf, "DEBUG_LEVEL" ) && (s = strstr( buf, "=" )) )
	{
	  sscanf(strchr(s, '=')+1, "%d", &config.debug_level );
	}
      else if( strstr( buf, "TARGET_NUM_HOSTS" ) && (s = strstr( buf, "=" )) )
	{
	  sscanf(strchr(s, '=')+1, "%d", &config.target_num_hosts );
	}
      else
	{
	  cerr << "Unknown config option \"" << buf << "\"" << endl;
	}
    }

  if( t_num_arches != config.num_arches )
    {
      cerr << "Number of executables does not match number "
	   << "of declared architectures in config file"
	   << endl;
      success = false;
    }

  if( t_num_arches > 16 )
    {
      cerr << "LIAR!!!!  You don't have more than 16 architectures" 
	   << endl;
      success = false;
    }

  return success;
}

/*
 * Constructor for config
 */
LShapedDriver::Config::Config()
{
  
  start_new_it_percent = 1.0;
  chunk_size_type = TARGET_TIME;
  target_chunk_size_time = 10.0;
  percent_scenarios_per_chunk = 0.0;
  explicit_chunk_size = 0;

  convergeTol = 1.0e-6;
  cutDelBnd = 25;
  
  strcpy( core_filename, "ssn.cor" );
  strcpy( time_filename, "ssn.tim" );
  strcpy( stoch_filename, "ssn.sto" );

  sample_input = 0;
  sample_size = -1;

  checkpoint_type = TIME;
  it_checkpoint_frequency = 0;
  time_checkpoint_frequency = 300;

  debug_level = 10;
  target_num_hosts = -1;

}

LShapedDriver::Config::~Config()
{
}
