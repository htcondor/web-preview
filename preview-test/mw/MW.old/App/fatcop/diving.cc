/*---------------------------------------------------------------------------
--  FILENAME:  diving.cc
--  AUTHOR:    Jeff Linderoth
--
--  This function implements a diving based primal heuristic
---------------------------------------------------------------------------*/
/****************************************************************************
*
****************************************************************************/

/*** INCLUDES ***/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream.h>
#include <lpsolver.h>
#include <MW.h>
#include "FATCOP_worker.hh"

/*** CONSTANTS ***/

#define MAXROUNDS 50
#define AGGRESSIVEROUNDS 5
#define MAXFIX 5
#define RANGEFACTOR 0.9
#define UNDEFINED -1
#define BASIC 1
#define SMALLINF (1e14)

#define EPSILON       (1e-6)
#define ABS( x )      ( ( x ) < 0 ? -( x ) : ( x ) )
#define SMALL( x )    ( ABS( x ) < EPSILON )
#define FLOOR( x )    floor( (double) ( x ) )
#define FRAC( x )      ( (double) ( x ) - FLOOR ( x + EPSILON) )

/*** TYPES ***/

/*** PROTOTYPES ***/

static void FixAtUpper( int, const double [], const int [], const double [], 
			double [] );

static bool SelectFracVars( int, int, double [], double [], double [], 
			    const int [], const double [], int *, int [],
			    char [], double [] );

static void RedCostFixing( int, double, double, const int [], double [], 
			   double [], const double [], const double [],
			   const int[], const int [] );

/*** FUNCTIONS ***/

/****************************************************************************
*
****************************************************************************/
bool FATCOP_worker::diving_heuristic( FATCOP_task *tf )
{

  bool feas_sol = false;

  int nvars = lpsolver->LPGetActiveCols();

  if( nvars != formulation->get_nvars() )
    {
      MWprintf( 10, "Columns removed -- unable to perform primal heuristic!\n" );
      return false;
    }

  int nrows = lpsolver->LPGetActiveRows();

  double *cbdl = new double [nvars];
  double *cbdu = new double [nvars];
  double *lbval = new double [nvars];
  double *ubval = new double [nvars];

  double *obdl = new double [nvars];
  double *obdu = new double [nvars];

  double *new_xlp = new double [nvars];
  
  double *tdj = new double [nvars];
  int *cstat = new int [nvars];
  int *rstat = new int [nrows];

  double zopt = bestUpperBound;

#if defined( DEBUG_PRIMAL )
  MWprintf( 10, "Entering Heuristic.  zlp = %.4lf\n", zlp );
#endif  

  double tzlp = zlp;

  int lpcolcnt = nvars;

  const int *ovclass = formulation->get_colmajor()->get_ctype();
  const double *objx = formulation->get_objx();

  lpsolver->LPGetBdl( obdl, 0, nvars - 1 );
  lpsolver->LPGetBdu( obdu, 0, nvars - 1 );

  bool backtracking;  
  int sol_status;

  int ixcnt;
  int ixv[ MAXFIX ];
  char ixlu[ MAXFIX ];
  double ixbd[ MAXFIX ];

/* 
   Copy current bounds
*/

  int i;
  for(i = 0; i < lpcolcnt; i++)
    {
      cbdl[i] = obdl[i];
      cbdu[i] = obdu[i];
      new_xlp[i] = xlp[i];
    }

   int roundsleft = MAXROUNDS;

   int *orig_cstat = new int [nvars];
   int *orig_rstat = new int [nrows];

   lpsolver->LPGetBasis( orig_cstat, orig_rstat );
   for( i = 0; i < nvars; i++ )
     cstat[i] = orig_cstat[i];
   for( i = 0; i < nrows; i++ )
     rstat[i] = orig_rstat[i];

   for(;;)
     {
#if defined( DEBUG_PRIMAL )
       MWprintf( 60, "Primal heuristics: Rounds left = %d\n", roundsleft );
#endif

      if( roundsleft-- == 0)
	goto DONE;

      // Check to see if (by doing the heuristic, you have taken too long!)
      clock_t ft = clock();
      if( (double) (ft-startTick)/CLOCKS_PER_SEC > maxTaskTime )
	{
	  MWprintf( 20, "Diving heuristic is taking too long.  Aborting\n" );
	  goto DONE;
	}
      
/* 
   Fix all those binary variables at a value of 1
*/
      FixAtUpper(lpcolcnt, new_xlp, ovclass, cbdu, cbdl);
      
/* 
   Select some fractional variables to fix -- 
   or determine if there none.

   XXX If you want "different" diving heuristics -- rewrite this function!
*/

      if (SelectFracVars(roundsleft, lpcolcnt, new_xlp, cbdl, cbdu, ovclass, 
			 objx, &ixcnt, ixv, ixlu, ixbd) == false)
	{
/* 
   Found Primal Feasible Solution	  
*/

	  if( tzlp < bestUpperBound - EPSILON )
	    {
	      if( formulation->feasibleSolution( new_xlp ) )
		{
		  MWprintf( 10, "Heuristics on worker %s found a feasible solution of value: %.4lf\n",
			    mach_name, tzlp );
		  feas_sol = true;
		  bestUpperBound = tzlp;
		  tf->numVars = nvars;
		  if( NULL == tf->feasibleSolution )
		    tf->feasibleSolution = new double [nvars];

		  for( int i = 0; i < nvars; i++ )
		    tf->feasibleSolution[i] = new_xlp[i];

		  goto DONE;
		}
	    }
	}
      else
	{
	  backtracking = FALSE;
	  
	FIXVAR:
	  
/*
   Tentatively Set new bounds
*/

	  for(i = 0; i < lpcolcnt; i++)
	    {
	      lbval[i] = cbdl[i];
	      ubval[i] = cbdu[i];
	    }

#if defined( DEBUG_PRIMAL )
	  MWprintf( 60, "Primal Heuristic: fixing %d variables\n", ixcnt );
#endif
	  for(i = 0; i < ixcnt; i++)
	    {
	      if (ixlu[i] == 'L')
		lbval[ixv[i]] = ixbd[i];
	      else if(ixlu[i] == 'U')
		ubval[ixv[i]] = ixbd[i];
	      else
		{
		  MWprintf( 10, "primal_heuristics: L/U not being set\n" );
		  goto DONE;
		}
	    }

/* 
   Question -- What good does this do? -- It's in the MINTO code.
   Jeff is taking it out for the time being.
   
   I think it gets rid of annoying "bounds contraidctory" messages!
*/

	  for (i = 0; i < lpcolcnt; i++)
	    {
	      if (lbval[i] > ubval[i] + EPSILON)
		{		  
		  break;
		}
	    }
	  
	  if (i!=lpcolcnt)
	    {
#if defined( DEBUG_PRIMAL )
	      MWprintf( 60, "Primal Heuristic: Contradictory bounds.  Backtracking\n" );
#endif
	      goto BACKTRACK;
	    }


/*
   Solve the LP with the variables fixed
*/
	  lpsolver->LPSetAllBdl( lbval, lpcolcnt );
	  lpsolver->LPSetAllBdu( ubval, lpcolcnt );

	  if( backtracking )
	    lpsolver->LPLoadBasis( cstat, rstat );	  

	  sol_status = lpsolver->LPSolveDual( &tzlp, new_xlp, NULL, NULL, tdj );
#if defined( DEBUG_PRIMAL )
	  MWprintf( 60, "Primal Heuristic: zlp = %.4lf\n", tzlp );
#endif

	  if (sol_status != LP_OPTIMAL || tzlp > zopt - EPSILON)
	    {
#if defined( DEBUG_PRIMAL )
	      MWprintf( 60, "Primal Heuristic.  sol_status = %d. Backtracking!\n",
			sol_status );
#endif	      
	      goto BACKTRACK;
	    }

	  // You always load the last basis if you backtrack!!!
	  lpsolver->LPGetBasis( cstat, rstat );

/* 
   If you find have an optimal solution, try to do some rudimentary
   reduced cost fixing to see if you can fix some more variables in
   the problem
*/
   
	  if (zopt < SMALLINF )
	    {
	      RedCostFixing(lpcolcnt, tzlp, zopt, ovclass,
			  cbdl, cbdu, new_xlp, tdj, cstat, rstat);
	    }

/* 
   The LP was a success, so keep the bounds you set
*/

	  for(i=0 ; i < lpcolcnt ; i++)
	    {
	      cbdl[i] = lbval[i];
	      cbdu[i] = ubval[i];
	    }
	}
      
    }

 BACKTRACK:
  if (backtracking)
    {
      goto DONE;
    }
  else
    {

      backtracking = TRUE;

/* 
   Backtrack once and fix the first variable to 0.
   XXX -- Why 'O', shouldn't it be to the "original" lower bound?
*/

      ixlu[0] = 'U';
      ixbd[0] = (double)0;
      goto FIXVAR;
    }
  
 DONE:

  // Make sure to restore the LP bounds.

  lpsolver->LPSetAllBdl( obdl, lpcolcnt );
  lpsolver->LPSetAllBdu( obdu, lpcolcnt );

  lpsolver->LPLoadBasis( orig_cstat, orig_rstat );
  // Also make sure you resolve the LP, so that LP solver has the right
  //  solution.  

  // Bug fix, JTL 12/13/99 
  //   If using SOPLEX and the simplex upper bound has been set, you
  //     must "resolve" it using primal simplex, or else the 
  //     SOPLEX LPSOLVERInterface will stop computation
  //  Jeff fixed this logic in the LPSolverInterface, so it
  //  should work (better) with dual now.

  //sol_status = lpsolver->LPSolvePrimal( &tzlp, NULL, NULL, NULL, NULL );
  sol_status = lpsolver->LPSolveDual( &tzlp, NULL, NULL, NULL, NULL );

  if( sol_status != LP_OPTIMAL )
    {
      // Yikes!  SOPLEX sometimes gives errors -- just continue on.
      // (Maybe OPTIMAL_BUT_UNSCALED, for example)

      MWprintf( 10, " Primal heuristic.  Danger Will Robinson!!!!  Can't reload initial LP!\n  Status = %d\n", sol_status );
    }
  

#if defined( DEBUG_PRIMAL )
  MWprintf( 10, "Leaving Heuristic.  zlp = %.4lf\n", tzlp );
#endif  

  if( orig_cstat ) delete [] orig_cstat;
  if( orig_rstat ) delete [] orig_rstat;

  if( cbdl ) delete [] cbdl;
  if( cbdu ) delete [] cbdu;
  if( lbval ) delete [] lbval;
  if( ubval ) delete [] ubval;
  if( obdl ) delete [] obdl;
  if( obdu ) delete [] obdu;
  if( new_xlp ) delete [] new_xlp;
  if( tdj ) delete [] tdj;
  if( cstat ) delete [] cstat;
  if( rstat ) delete [] rstat;
  
  return feas_sol;

}

/****************************************************************************
*
*  This function fixes all variables currently at their upper
*   bounds to that value forever
*
****************************************************************************/
static void FixAtUpper
(
int n,
const double xlp[], 
const int ovclass[], 
const double cbdu[], 
double cbdl[]
)
{
  for(int i=0;i<n;i++)
    if ((ovclass[i] != COL_TYPE_CONTINUOUS) && (xlp[i] > cbdu[i] - EPSILON))
      {
	cbdl[i] = cbdu[i];
      }
}

/****************************************************************************
*
* This function selects some fractional variables to fix and returns them
*  in the arrays ixv (index), ixlu (whether fixed to upper or lower), and
*  ixbd (the actual value at which the variable is fixed)
*
****************************************************************************/
static bool SelectFracVars
(
int roundsleft,
int n,
double xlp[],
double lb[],
double ub[],
const int ovclass[],
const double objx[],
int *ixcnt,
int ixv[],
char ixlu[],
double ixbd[]
)     
{
  
  int freecnt = 0;
  int fraccnt = 0;
  double thresh = 0;
  int ix = UNDEFINED;
  double frac;
  char lu = 'L';
  double bd = 0.0;

/* 
   Determine Largest fractional binary value = 'thresh'
*/

  int j;

  for(j = 0; j < n; j++)
    {

      if (ovclass[j] == COL_TYPE_BINARY)
	{
	  if (lb[j] < ub[j] - EPSILON)
	    {
	      freecnt++;
	      double frac = FRAC( xlp[j] );
	      if (frac > EPSILON && frac < 1-EPSILON)
		{
		  fraccnt++;
		  if (frac > thresh)
		    thresh = frac;
		}
	    }
	}
	      
    }

  if (thresh < EPSILON)
    {

/*
   No fractional binary variables exists
*/

      ix = UNDEFINED;
      double maxdiff = (double) -1;
      for(j = 0; j < n; j++)
	{
	  if(ovclass[j] == COL_TYPE_INTEGER)
	    {
	      double frac = FRAC(xlp[j]);
	      if (frac > EPSILON && frac < 1 - EPSILON)
		{
		  double diff = ABS (frac - 0.5);
		  if (diff > maxdiff + EPSILON)
		    {
		      maxdiff = diff;
		      ix = j;
		      lu = (frac < 0.5) ? 'U' : 'L';
		      bd = (frac < 0.5) ? FLOOR (xlp[j] + EPSILON) :
				  CEIL (xlp[j] - EPSILON);
		    }
		}
	    }
	}
      
      if (ix == UNDEFINED)
/* 
   We have found a feasible solution
*/
	return false;

      else
	{
/* 
   Fix the one integer variable
*/
	  *ixcnt = 1;
	  ixv[0] = ix;
	  ixlu[0] = lu;
	  ixbd[0] = bd;
	  return true;
	}
    }
  
/* 
   Now take care of fractional binary variables -- Maybe fix more than one
   and take into account the objective function coefficient.
*/

  if (roundsleft > AGGRESSIVEROUNDS)
    *ixcnt = 1;
  else
    {
      if( roundsleft > 0 )
	*ixcnt = (int) CEIL( freecnt / (10*roundsleft));
      else
	*ixcnt = freecnt;
	
      *ixcnt = MIN(*ixcnt, fraccnt);
      *ixcnt = MIN(*ixcnt, MAXFIX);
    }


/*
   What the heck does this do? -- Just increments i then does nothing.
   Decrements thresh too.
*/

  int i;
  for (i = 0; i < *ixcnt; ) {
        thresh *= RANGEFACTOR;
        for (i = 0, j = 0; j < n; j++) {
            if (ovclass[j] == COL_TYPE_BINARY) {
              frac = FRAC(xlp[j]);
              if (frac > EPSILON && frac < 1 - EPSILON && frac > thresh) {
                  i++;
              }
            }
        }
      }

  for (i = 0; i < *ixcnt; i++)
    {
      double cmax = -INFBOUND;
      for(j = 0; j < n; j++)
	{
	  if (ovclass[j] == COL_TYPE_BINARY)
	    {
	      frac = FRAC(xlp[j]);
	      if (frac > EPSILON &&
		  frac < 1 - EPSILON &&
		  frac > thresh &&
		  objx[j] > cmax)
		{
		  cmax = objx[j];
		  ix = j;
		}
	    }
	}
      ixv[i] = ix;
      ixlu[i] = 'L';
      ixbd[i] = 1.0;
    }
  return true;
}

/****************************************************************************
*
*  This function performs rudimentary reduced cost fixing.
*
****************************************************************************/
static void RedCostFixing
(
int n,
double zlp,
double zopt, 
const int ovclass[], 
double lb[], 
double ub[], 
const double xlp[],
const double dj[],
const int cstat[],
const int rstat[]
)
{

  for(int j = 0; j < n; j++)
    {
      if (cstat[j] != BASIC && ovclass[j] != COL_TYPE_CONTINUOUS)
	{
	  if (lb[j] > ub[j] - EPSILON)
	    continue;
	  if (xlp[j] < lb[j] + EPSILON && zlp + dj[j] - zopt < EPSILON)
	    {
	      ub[j] = lb[j];
	    }
	  if (xlp[j] > ub[j] - EPSILON && zlp - dj[j] - zopt < EPSILON)
	    {
	      lb[j] = ub[j];
	    }
	}
    }
}

/** End of file ***/
