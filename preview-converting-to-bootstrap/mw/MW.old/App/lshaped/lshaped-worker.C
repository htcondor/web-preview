/* lshaped-worker.C

*/

#include <sys/resource.h>
#include <unistd.h>
#include "lshaped-worker.h"
#include "lshaped-task.h"
#include "pvm3.h"

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


LShapedWorker::LShapedWorker() 
{
  // do this here so that workingtask has type LShapedTask, and not
  // MWTask type.  This class holds info for the task at hand.
  //XXX This is the sort of thing that can be in the "skeleton" code.

  workingTask = new LShapedTask;
  
  sobj = NULL;
  srhs = NULL;
  smatval = NULL;
  sbdl = NULL;
  sbdu = NULL;
  ssense = NULL;
  smatbeg = NULL;
  smatind = NULL;

  snew_rhs = NULL;

  OPT_TOL = 1.0e-8;
}

LShapedWorker::~LShapedWorker() 
{
  delete workingTask;
}

void LShapedWorker::unpack_init_data( void ) 
{
  spprob.unpackFromPVMBuffer();

#if defined( DEBUG_APP )
  cout << "Unpacked SPProb from PVM buffer" << endl;
#endif

  // Now set up the subproblem base LP.

  // This only works for two stage problems.
  assert( spprob.numPeriods == 2 );

  spprob.getBaseLP( 1, &sncols, &snrows, &snnzero, &sos, 
		    &sobj, &srhs, &ssense, &smatbeg, &smatind, &smatval,
		    &sbdl, &sbdu );  

  // This stores T.x
  snew_rhs = (double *) malloc( snrows * sizeof( double ) );

  // We have decided to add the artificials (both ways)
  tsncols = sncols + 2 * snrows;
  tsnrows = snrows;
  tsnnzero = snnzero + 2 * snrows;

  smatbeg = (int *) realloc( smatbeg, ( tsncols + 1 ) * sizeof( int ) );
  smatind = (int *) realloc( smatind, tsnnzero * sizeof( int ) );
  smatval = (double *) realloc( smatval, tsnnzero * sizeof( double ) );
  sbdl = (double *) realloc( sbdl, tsncols * sizeof( double ) );
  sbdu = (double *) realloc( sbdu, tsncols * sizeof( double ) );

  // Create the phase I objective
  tsobj = (double *) malloc( tsncols * sizeof( double ) );	
  for( int i = 0; i < sncols; i++ )
    tsobj[i] = 0.0;

  // Add I
  for( int i = 0; i < snrows; i++ )
    {
      tsobj[sncols+i] = 1.0;
      smatbeg[sncols+i] = snnzero + i;
      smatind[snnzero+i] = i;
      smatval[snnzero+i] = 1.0;
      sbdl[sncols+i] = 0.0;
      sbdu[sncols+i] = LPSOLVER_INFINITY;
    }
	
  // Add -I
  for( int i = 0; i < snrows; i++ )
    {
      tsobj[sncols+snrows+i] = 1.0;
      smatbeg[sncols+snrows+i] = snnzero + snrows + i;
      smatind[snnzero+snrows+i] = i;
      smatval[snnzero+snrows+i] = -1.0;
      sbdl[sncols+snrows+i] = 0.0;
      sbdu[sncols+snrows+i] = LPSOLVER_INFINITY;
      
    }
  smatbeg[sncols+2*snrows] = snnzero + 2*snrows;

#if defined( DEBUG_APP )
  cout << "  *** Base LP (w/artificals) period " << 1 << endl;
  cout << "nrows: " << tsnrows << " ncols: " << tsncols << " nnzero: " 
	  	     << tsnnzero << endl;

  PRINT( cout, "matbeg", smatbeg, tsncols + 1, 10 );
  PRINT( cout, "matind", smatind, tsnnzero, 10 );
  PRINT( cout, "matval", smatval, tsnnzero, 10 );
  PRINT( cout, "rhs", srhs, tsnrows, 10 );
  PRINT( cout, "bdl", sbdl, tsncols, 10 );
  PRINT( cout, "bdu", sbdu, tsncols, 10 );
#endif
      
  // LPsolver is stupid and requires a "matcnt" -- I should change this.
  int *smatcnt = (int *) malloc( tsncols * sizeof( int ) );
  for( int i = 0; i < tsncols; i++ )
    smatcnt[i] = smatbeg[i+1] - smatbeg[i];
  
  // (Must call init explicitly due to name conflict)
  sublpsolver.CLPSolverInterface::init( tsncols, tsnrows, tsnnzero, sos, tsobj, 
					srhs, ssense, smatbeg, 
					smatcnt, smatind, smatval,
 					sbdl, sbdu, NULL );
  sublpsolver.LPLoad();  

  if( smatcnt ) free( smatcnt );
  smatcnt = NULL;

  sxlp = (double *) malloc( tsncols * sizeof( double ) );
  spi = (double *) malloc( tsnrows * sizeof( double ) );
  sdj = (double *) malloc( tsncols * sizeof( double ) );

  sublpsolver.LPSolvePrimal( &szlp, NULL, NULL, NULL, NULL );

#if defined( DEBUG_APP )
  cout << "Base stage II LP solution: " << szlp << endl;
#endif
  
  spprob.getTechnologyMatrix( 1, &tncols, &tnrows, &tnnzero,
			      &tmatbeg, &tmatind, &tmatval );

#if defined( DEBUG_APP )
  cout << "  *** Technology Matrix ***" << endl;
	
  PRINT( cout, "matbeg", tmatbeg, tncols + 1, 10 );
  PRINT( cout, "matind", tmatind, tnnzero, 10 );
  PRINT( cout, "matval", tmatval, tnnzero, 10 );  
  cout << endl << endl;
#endif



}

void LShapedWorker::execute_task( MWTask *t ) 
{

#if defined( DUMB_COMPILER )
  LShapedTask *lst = (LShapedTask *) t;
#else
  // We have to downcast this task passed to us into an LShapedTask
  LShapedTask *lst = dynamic_cast<LShapedTask *> ( t );
  assert( lst );
#endif

  // If this isn't true, you have significant problems
  assert( tncols == lst->mncols );

#if 1
  cout << "This task solving scenarios from: " << lst->startIx 
       << " to " << lst->endIx << endl;
#endif

  struct rusage resourceUsage;
  getrusage( RUSAGE_SELF, &resourceUsage );

  int begTime = resourceUsage.ru_utime.tv_sec + 
    resourceUsage.ru_stime.tv_sec;
  
  

  // Allocate memory to hold the "temporary" cut
    
  int tmp_cutnnzero = 0;
  double tmp_cutrhs = 0.0;
  int *tmp_cutmatind = (int *) malloc( tncols * sizeof( int ) );
  double *tmp_cutmatval = (double *) malloc( tncols * sizeof( double ) );

  // (Initially dense)
  for( int i = 0; i < tncols; i++ )
    {
      tmp_cutmatind[i] = i;
      tmp_cutmatval[i] = 0.0;
    }

  bool feascut = false;

  // This will hold the cumulative second stage costs for this "x"
  // over the scenarios we solve in this task
  double sscost = 0.0;

  for( int scenario = lst->startIx; scenario <= lst->endIx; scenario++ )
    {
      double p;
      int nChanges;
      ChangeType *changes = NULL;
      int *changeEnt = NULL;
      double *changeVal = NULL;
      spprob.getScenario( 1, scenario, &p, &nChanges, &changes,
			  &changeEnt, &changeVal );      


      //  Do "incremental" changes to LP -- A little harder,
      //  but then we don't have to reload the LP all the time
      //  (You might also change TT or RHS so send that along too).
      //XXX (Actually if bounds on variables change
      //     you will need to send these along too)

      double *oldVal = (double *) malloc ( nChanges * sizeof( double ) );
      for( int i = 0 ; i < nChanges; i++ )
	{		      
	  makeLPChange( &sublpsolver, changes[i], changeEnt[i],
			changeVal[i], &(oldVal[i]), tmatval, srhs );
	}

      for( int i = 0; i < tsnrows; i++ )
	{
	  snew_rhs[i] = srhs[i];
	}

      // This does T.x
      for( int j = 0; j < tncols; j++ )
	for( int k = tmatbeg[j]; k < tmatbeg[j+1]; k++ )
	  snew_rhs[tmatind[k]] -= tmatval[k] * lst->mxlp[j];


#if defined( DEBUG_EXAMPLE2 )
      for( int i = 0; i < tsnrows; i++ )
	cout << "New RHS[" << i << "] = " << snew_rhs[i] << endl;
#endif

      // Change the RHS
      sublpsolver.LPChangeFullRHS( snew_rhs, tsnrows );

      // Now solve it.  (With SOPLEX, maybe set that basis exists)
      CLPStatus sstat = sublpsolver.LPSolveDual( &szlp, NULL, spi, 
						 NULL, sdj );

		
      if( sstat != LP_OPTIMAL )
	{
	  cerr << "The Phase I problem is infeasible -- you are hosed!" << endl;
	  exit( 666 );
	}

      if( szlp > 1.0e-6 )
	{
#if defined( DEBUG_APP )
	  cout << "Scenario " << scenario 
	       << " is infeasible.  Phase 1 zlp: " << szlp << endl;
#endif

	  feascut = true;

	  // Create feasibility cut here	  
	  for( int j = 0; j < tncols; j++ )
	    {
	      double tmp = 0.0;
	      
	      for( int k = tmatbeg[j]; k < tmatbeg[j+1]; k++ )
		tmp += spi[tmatind[k]] * tmatval[k];
	      
	      tmp_cutmatval[j] = tmp;
	    }	      

	  double tmp = 0.0;
	  for( int i = 0; i < tsnrows; i++ )
	    tmp += spi[i] * srhs[i];

	  // The RHS needs to be adjusted if there are "weird"
	  // bounds in the stage two problem.

	  for( int j = 0; j < sncols; j++ ) // We don't care about phase I here
	    {
	      //XXX Bounds might be stochastic -- you need to fix this!
	      
	      if( sbdu[j] < LPSOLVER_INFINITY/10 ) // For numerical reasons
		tmp += sbdu[j] * sdj[j];
	      
	      tmp += sbdl[j] * sdj[j];
	    }

	  tmp_cutrhs = tmp;
		    
	  // Change it back
		    
	  for( int i = 0; i < nChanges; i++ )
	    {
	      makeLPChange( &sublpsolver, changes[i], changeEnt[i],
			    oldVal[i], &(changeVal[i]), tmatval, srhs );
	    }
		    
	  free( changes ); 
	  free( changeEnt ); 
	  free( changeVal ); 

	  free( oldVal );
      
	}
      else
	{
	  // Do Phase II.
	  // Change objective and change upper bounds.

	  //XXX LPSolver should be able to change them all at once.
	  for( int i = 0; i < sncols; i++ )
	    sublpsolver.LPChangeObjCoef( sobj[i], i );
	  for( int i = sncols; i < tsncols; i++ )
	    {
	      sublpsolver.LPChangeObjCoef( 0.0, i );
	      sublpsolver.LPSetOneBdu( 0.0, i );
	    }
	  
	  CLPStatus sstat = sublpsolver.LPSolveDual( &szlp, sxlp, spi, 
						     NULL, sdj );
#if defined( DEBUG_APP2 )
	  char sfname[256];
	  sprintf( sfname, "%s.%d", "subproblem", scenario );
	  sublpsolver.LPWriteMPS( sfname );
#endif

	  // Update the second stage cost

	  sscost += p * szlp;
				
	  // Change Things back to get ready for Phase I.
	  for( int i = 0; i < sncols; i++ )
	    sublpsolver.LPChangeObjCoef( 0.0, i );
	  for( int i = sncols; i < tsncols; i++ )
	    {
	      sublpsolver.LPChangeObjCoef( 1.0, i );
	      sublpsolver.LPSetOneBdu( LPSOLVER_INFINITY, i );
	    }
	  
	  if( sstat != LP_OPTIMAL )
	    {
	      cerr << "Subproblem Phase II not optimal? -- " 
		   << sstat << endl;
	    }

	  // Create cut "incrementally" since otherwise
	  //  you (might) need to go back and change T and rhs
		    
	  for( int j = 0; j < tncols; j++ )
	    {
	      double tmp = 0.0;

	      for( int k = tmatbeg[j]; k < tmatbeg[j+1]; k++ )
		tmp += spi[tmatind[k]] * tmatval[k];

	      tmp_cutmatval[j] += p * tmp;
	    }	      

	  double tmp = 0.0;
	  for( int i = 0; i < tsnrows; i++ )
	    tmp += spi[i] * srhs[i];

	  // The RHS needs to be adjusted if there are "weird"
	  // bounds in the stage two problem.

	  for( int j = 0; j < sncols; j++ ) // We don't care about phase I here
	    {
	      //XXX Bounds might be stochastic
	      if( sbdu[j] < LPSOLVER_INFINITY/10 ) // For numerical reasons
		tmp += sbdu[j] * sdj[j];
	      
	      tmp += sbdl[j] * sdj[j];
	    }
	  
	  tmp_cutrhs += tmp * p;


#if defined( DEBUG_APP )
	  cout << " *** Scenario " << scenario << " status: "
	       << sstat << " solution: " << szlp << endl;
#if defined( DEBUG_APP )
	  PRINT( cout, "x", sxlp, sncols, 10 );
	  PRINT( cout, "pi", spi, tsnrows, 10 );
	  PRINT( cout, "dj", sdj, sncols, 10 );		    
#endif
#endif
	  
	  // Change scenario back
	  
	  for( int i = 0; i < nChanges; i++ )
	    {
	      makeLPChange( &sublpsolver, changes[i], changeEnt[i],
			    oldVal[i], &(changeVal[i]), tmatval, srhs );
	    }
		  
	  free( changes ); 
	  free( changeEnt ); 
	  free( changeVal ); 
	  
	  free( oldVal );
	  		
	}
      
    } // scenario loop


  // Remove the zeroes from the cut
  tmp_cutnnzero = tncols;
  sparsify_cut( tncols, &tmp_cutnnzero, tmp_cutmatind, tmp_cutmatval );

#if defined( DEBUG_APP ) 
  cout << "Sparsified Cut: " << endl;
  PRINT( cout, "a", tmp_cutmatval, tmp_cutnnzero, 10 );
  PRINT( cout, "ind", tmp_cutmatind, tmp_cutnnzero, 10 );		    
  cout << ">= " << tmp_cutrhs << endl;
#endif 

  if( feascut )
    {
#if defined ( DEBUG_APP )
      cout << "Feasibility Cut" << endl;
#endif
      lst->ncuts = 1;      
      lst->cutType = new int[lst->ncuts];
      lst->cutType[0] = FEASIBILITY_CUT;
      lst->error = LPSOLVER_INFINITY;
      lst->sscost = LPSOLVER_INFINITY;
    }
  else
    {
#if defined ( DEBUG_APP )
      cout << "Optimality Cut" << endl;
#endif

      // Check if you are going to add a cut

      double tmp = tmp_cutrhs;
      for( int j = 0; j < tmp_cutnnzero; j++ )
	{
	  tmp -= tmp_cutmatval[j] * lst->mxlp[tmp_cutmatind[j]];
	}
		
      double tmp2 = tmp > 1.0 ? tmp : 1.0;
      if( (lst->theta - tmp)/tmp2 > -OPT_TOL )
	{
	  cout << "No optimality cut found." << endl
	       << "Theta approx off by " << (lst->theta - tmp)
	       << endl;

	  lst->ncuts = 0;
	  lst->error = (lst->theta - tmp) < 0 ? -(lst->theta - tmp)
	    : (lst->theta - tmp);
	  lst->sscost = sscost;
	}
      else
	{
	  lst->ncuts = 1;	  
	  lst->cutType = new int [lst->ncuts];
	  lst->cutType[0] =  OPTIMALITY_CUT; 
	  lst->error = (lst->theta - tmp) < 0 ? -(lst->theta - tmp)
	    : (lst->theta - tmp);
	  lst->sscost = sscost;

#if 0
	  cout << "Optimality cut found." << endl;
	  cout << " Theta was " << lst->theta << endl;
	  cout << " w is " << tmp << endl;
	  cout << " sscost is " << lst->sscost << endl;
#endif

	  if( tmp - lst->sscost > 1.0e-6 || lst->sscost - tmp > 1.0e-6 )
	    {
	      cout << "WOAH!  Second Stage Costs Estimates different" 
		   << " than expected." << " w: " << tmp 
		   << "  sscost: " << lst->sscost << endl;
	    }
	  
	}      
      
    }

  if( lst->ncuts > 0 )
    {
      lst->cutnnzero = tmp_cutnnzero;
      lst->cutbeg = new int[lst->ncuts+1];
      lst->cutind = new int[lst->cutnnzero];
      lst->cutval = new double[lst->cutnnzero];
      lst->cutrhs = new double[lst->ncuts];

      // This is written complexly to aid if you ever want to
      //  send back > 1 cut

      for( int i = 0; i < lst->ncuts; i++ )
	{
	  lst->cutrhs[i] = tmp_cutrhs;
	  lst->cutbeg[i] = 0;
	}	

      lst->cutbeg[lst->ncuts] = lst->cutnnzero;

      for( int i = 0; i < lst->cutnnzero; i++ )
	{
	  lst->cutind[i] = tmp_cutmatind[i];
	  lst->cutval[i] = tmp_cutmatval[i];
	}
    }

  if( tmp_cutmatind ) free( tmp_cutmatind );
  if( tmp_cutmatval ) free( tmp_cutmatval );
  

  //XXX I have no idea what happens if you get suspended -- should be
  //    interesting.
  // In SOLARIS 5.6, only the time portions of rusage are implemented

  getrusage( RUSAGE_SELF, &resourceUsage );
  int endTime = resourceUsage.ru_utime.tv_sec + 
    resourceUsage.ru_stime.tv_sec;

  lst->taskTime = (double) (endTime - begTime);

  cout << "This task took " << lst->taskTime << " seconds" << endl;

}


// This function searches through the scenario and makes all the changes
int LShapedWorker::makeLPChange( CLPSolverInterface *lpsolver, ChangeType type,
				 int entry, double val, double *oldval,
				 double matval[], double rhs[])
{
  switch( type )
    {
    case TT:
      {
#if defined( DEBUG_EXAMPLE )
	cout << " Changing T matrix entry " << entry << " to " << val << endl;
#endif
	*oldval = matval[entry];
	matval[entry] = val;
	break;
      }
    case RHS:  
      {
#if defined( DEBUG_EXAMPLE )
	cout << " Changing RHS entry " << entry << " to " << val << endl;
#endif
	*oldval = rhs[entry];
	rhs[entry] = val;
	break;
      }
    case COST:
      {
	
	*oldval = lpsolver->LPGetObjCoef( entry );
	lpsolver->LPChangeObjCoef( val, entry );
	break;
      }
    default:
      {
	cout << "Unimplemented change type " << type << endl;	
      }
    }
}

const double cut_eps = 1.0e-8; // Smaller than this is 0...

void LShapedWorker::sparsify_cut( int old_nz, int *new_nz, 
				  int ind[], double val[] )
{
  int p = 0;

  for( int i = 0; i < old_nz; i++ )
    {
      if( ( val[i] > cut_eps ) || ( val[i] < -cut_eps  ) )
	{
	  val[p] = val[i];
	  ind[p] = ind[i];
	  p++;
	}
    }
  *new_nz = p;
}

void LShapedWorker::unpack_driver_task_data()
{
#if 0
  // For the real application I don't want this to do anything
  int q = 0;
  pvm_upkint( &q, 1, 1 );
  cout << "Unpacking driver task data:" << q << endl;
#endif
  
}
