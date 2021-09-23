/*---------------------------------------------------------------------------
--  FILENAME:  flowcov.cc
--  AUTHOR:    Jeff T. Linderoth
--
--  This code is just an adaptation to PARINO and C++ of
--  M. Savelsbergh's code for lifted simple generalized
--  flow cover inequalities.
--
---------------------------------------------------------------------------*/
/*****************************************************************************
*
*****************************************************************************/

/*** INCLUDES ***/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "cutgen.hh"

/*** CONSTANTS ***/

#ifndef UNDEFINED
#define UNDEFINED -1
#endif

/*** FUNCTIONS ***/

/*****************************************************************************
*
*  This function generates and returns an array of 
*  simple lifted generalized flow covers.
*
*****************************************************************************/
int CFlowCutGen::generate ( const CProblemSpec *problem,
    const CLPSolution *sol, int max_cuts, CCut **cuts  )
{
  int nz; 
  const int *ind_const;
  const double *coef_const;

  int *ind = NULL;
  double *coef = NULL;

  double rhs;
  char sense;
    
  int cut_nz, *cut_ind;
  double *cut_coef, cut_rhs, priority;
  char cut_sense;

  CRowType rtype;
  bool gen;

  int nrow = problem->get_nrows();
  int ncol = problem->get_nvars();

  int ngen = 0;
  int j = 0;
  int i;
  for( i = 0; i < nrow; i++ )
    {
      rtype = problem->get_rowmajor()->get_rtype()[i];
      if( ( rtype != ROW_TYPE_MIXUB ) &&
	  ( rtype != ROW_TYPE_MIXEQ ) &&
	  ( rtype != ROW_TYPE_NOBINUB ) &&
	  ( rtype != ROW_TYPE_NOBINEQ ) &&
	  ( rtype != ROW_TYPE_SUMVARUB ) &&
	  ( rtype != ROW_TYPE_SUMVAREQ ) )
	continue;

      nz = problem->get_rowmajor()->get_matcnt()[i];
      ind_const = problem->get_rowmajor()->get_matind() +
	problem->get_rowmajor()->get_matbeg()[i] ;
      coef_const =  problem->get_rowmajor()->get_matval() +
	problem->get_rowmajor()->get_matbeg()[i];
      sense = problem->get_senx()[i];
      rhs = problem->get_rhsx()[i];       	

      ind = NEWV( int, nz );
      coef = NEWV( double, nz );

      for( j = 0; j < nz; j++ )
	{
	  ind[j] = ind_const[j];
	  coef[j] = coef_const[j];
	}

      if( sense == 'E' )
	{ 
#if defined( TIMER )
	  CAbsTime t1;
#endif
	  gen = flow_cover( problem, sol, nz, ind, coef, 'L', rhs,
			       &cut_nz, &cut_ind, &cut_coef, &cut_sense,
			       &cut_rhs, &priority );
#if defined( TIMER )
	  CAbsTime t2; CTimePeriod dt = t2 - t1;
#endif

	  if( gen )
	    {
	      cuts[ngen] = new CCut;
	      cuts[ngen]->set_nvars( ncol );
	      cuts[ngen]->set_nnzero( cut_nz );
	      cuts[ngen]->set_ind( cut_ind );
	      cuts[ngen]->set_coeff( cut_coef );
	      cuts[ngen]->set_rhs( cut_rhs );
	      cuts[ngen]->set_sense( ( cut_sense == 'L' ? LESS_OR_EQUAL : cut_sense == 'E' ? EQUAL_TO : GREATER_OR_EQUAL ) );
	      cuts[ngen]->set_priority( priority );
#if defined( TIMER )
	      cuts[ngen]->set_generation_time( dt );
#endif
	      cuts[ngen]->compute_hash();

	      ngen++;
	      if (ngen >= max_cuts )
		goto EXIT;
	    }

#if defined( TIMER )
	  CAbsTime t3;
#endif
	  gen = flow_cover( problem, sol, nz, ind, coef, 'G', rhs,
			       &cut_nz, &cut_ind, &cut_coef, &cut_sense,
			       &cut_rhs, &priority );
#if defined( TIMER )
	  CAbsTime t4; dt = t4 - t3;
#endif

	  if( gen )
	    {
	      cuts[ngen] = new CCut;
	      cuts[ngen]->set_nvars( ncol );
	      cuts[ngen]->set_nnzero( cut_nz );
	      cuts[ngen]->set_ind( cut_ind );
	      cuts[ngen]->set_coeff( cut_coef );
	      cuts[ngen]->set_rhs( cut_rhs );
	      cuts[ngen]->set_sense( ( cut_sense == 'L' ? LESS_OR_EQUAL : cut_sense == 'E' ? EQUAL_TO : GREATER_OR_EQUAL ) );
	      cuts[ngen]->set_priority( priority );
#if defined( TIMER )
	      cuts[ngen]->set_generation_time( dt );
#endif
	      cuts[ngen]->compute_hash();

	      ngen++;
	      if (ngen >= max_cuts )
		goto EXIT;
	    }
	}
      else
	{ 
#if defined( TIMER )
	  CAbsTime t1;
#endif
	  gen = flow_cover( problem, sol, nz, ind, coef, sense, rhs,
			       &cut_nz, &cut_ind, &cut_coef, &cut_sense,
			       &cut_rhs, &priority );
#if defined( TIMER )
	  CAbsTime t2; CTimePeriod dt = t2 - t1;
#endif

	  if( gen )
	    {
	      cuts[ngen] = new CCut;
	      cuts[ngen]->set_nvars( ncol );
	      cuts[ngen]->set_nnzero( cut_nz );
	      cuts[ngen]->set_ind( cut_ind );
	      cuts[ngen]->set_coeff( cut_coef );
	      cuts[ngen]->set_rhs( cut_rhs );
	      cuts[ngen]->set_sense( ( cut_sense == 'L' ? LESS_OR_EQUAL : cut_sense == 'E' ? EQUAL_TO : GREATER_OR_EQUAL ) );
	      cuts[ngen]->set_priority( priority );
#if defined( TIMER )
	      cuts[ngen]->set_generation_time( dt );
#endif
	      cuts[ngen]->compute_hash();

	      ngen++;
	      if (ngen >= max_cuts )
		goto EXIT; 
	    }
	}

      if( ind ) DELETEV( ind ); ind = NULL;
      if( coef ) DELETEV( coef ); coef = NULL;

    }

  if( ind ) DELETEV( ind );
  if( coef ) DELETEV( coef );

 EXIT:

  const double *xlp = sol->get_xlp();

#ifdef DEBUG_CUTS
  cout << "Generated " << ngen << " Flow cover inequalities" << endl;
  for( i = 0; i < ngen; i++ )
    {
      const int *ti = cuts[i]->get_ind();
      const double *tc = cuts[i]->get_coeff();
      double lhs = 0;
      for( j = 0; j < cuts[i]->get_nnzero(); j++ )
	{
	  if( xlp[ti[j]] > EPSILON )
	    {
	      cout << "a[" << ti[j] << "] = " << tc[j] << "  "
		    << "x[" << ti[j] << "] = " << xlp[ti[j]] << endl;
	      lhs += tc[j] * xlp[ti[j]];
	    }
	}
      // chout << cuts[i] << endl; 

      cout << "   lhs = " << lhs << "  rhs = " << cuts[i]->get_rhs() << endl;
    }
#endif


  return ngen;
}
      
	  

/*****************************************************************************
*
*  This function is passed a problem, a row of this problem, and
*  an LP solution and determines if there is a violated
*  lifted simple generalized flow cover.  
*
*****************************************************************************/
bool CFlowCutGen::flow_cover
(
 const CProblemSpec *prob,
 const CLPSolution *sol,
 int nz,
 int *ind,
 double *coef,
 char sense,
 double rhs,
 int *cut_nz,               // These return the cut -- including a 
 int **cut_ind,             // priority.  (They also allocate the
 double **cut_coef,          // required memory).
 char *cut_sense,
 double *cut_rhs,
 double *priority
)
{
  bool generated = false;

  const double *xlp = sol->get_xlp();


  int *pcut_ind = NULL;
  double *pcut_coef = NULL;
  double violation = -DBL_MAX;
  double tmp1;

  double tmp, min;
  double sum = (double) 0;
  double minpls = DBL_MAX;
  double minneg = DBL_MAX;
  int splus = 0;

  double asum = (double) 0;
  double rhsknap;
  double lambda;


  CVLB vlb;
  CVUB vub;
  int i, j;
  
  if( sense == 'G' )
    flip( nz, ind, coef, &rhs );

#if defined ( DEBUG_CUTS2 )
  cout << "GENERALIZED FLOW COVER -- ORIGINAL CONSTRAINT" << endl;
  for (j = 0; j < nz; j++) {
    vlb = prob->get_vlb()[ind[j]];
    vub = prob->get_vub()[ind[j]];

    printf ("%f [%4d] -- %f", coef[j], ind[j], xlp[ind[j]]);
    fflush(stdout);
    if (vub.get_var() != UNDEFINED ) {
      printf ("  (%f [%4d] -- %f)\n",
	      vub.get_val(), vub.get_var(), xlp[vub.get_var()]); 
      fflush(stdout);
    }
    else {
      printf ("  (%f)\n", prob->get_bdu()[ind[j]]);
      fflush(stdout);
    }
  }
  printf ("SENSE: L\n");
  printf ("RHS: %f\n", rhs);
  fflush(stdout);
#endif
  
  double value;

  double lj, uj;

  for( j = 0; j < nz; j++ )
    {

      vlb = prob->get_vlb()[ind[j]];
      lj = ( vlb.get_var() != UNDEFINED ) ? vlb.get_val() 
	: prob->get_bdl()[ind[j]];

      vub = prob->get_vub()[ind[j]];
      uj = ( vub.get_var() != UNDEFINED ) ? vub.get_val() 
	: prob->get_bdu()[ind[j]];

      if ( lj < -EPSILON )
	goto EXIT;

      if( prob->get_colmajor()->get_ctype()[ind[j]] != COL_TYPE_BINARY )
	{
	  value = coef[j];
	  if( value > EPSILON )
	    {
	      sign[j] = CONTPOS;
	    }
	  else
	    {
	      sign[j] = CONTNEG;
	      value = -value;
	    }

	  up[j] = value * uj;
	  x[j] = ( vub.get_var() != UNDEFINED ) ? xlp[vub.get_var()] : (double) 1;
	  y[j] = value * xlp[ind[j]];
	}
      else
	{
	  value = coef[j];
	  if( value > EPSILON )
	    {
	      sign[j] = BINPOS;
	    }
	  else
	    {
	      sign[j] = BINNEG;
	      value = -value;
	    }

	  up[j] = value;
	  x[j] = xlp[ind[j]];
	  y[j] = value * x[j];
	}
    }




  for( j = 0; j < nz; j++ )
    if( !INTEGRAL( x[j] ) )
      break;

  if( j == nz )
    goto EXIT;


  /*
   * FIND COVER (C+,C-) IN (N+,N-)
   */

  /*
   * SET UP THE KNAPSACK PROBLEM THAT DETERMINES THE COVER
   *
   * NOTE: for j in N+ : xj = 0  ==> j not in C+
   *       for j in N- : uj = INF ==> j not in C-
   */
  
  rhsknap = rhs;



  
  for( j = 0; j < nz; j++ ) 
    {
      cand[j] = mark[j] = 0;
      ratio[j] = DBL_MAX;
        
      switch (sign[j]) {
      case CONTPOS:
      case BINPOS:
	if( y[j] > EPSILON ) 
	  {
	    ratio[j] = (1.0 - x[j]) / up[j];
	    if( y[j] > up[j] * x[j] - EPSILON ) 
	      {
		cand[j] = PRIME;
		asum += up[j];
	      }
	    else 
	      {
		cand[j] = SECONDARY;
	      }
	  }
	break;
      case CONTNEG:
      case BINNEG:
	if( up[j] > ( (1.0 - EPSILON) * DBL_MAX ) ) 
	  {
	    mark[j] = INCUT;
	  }
	else 
	  {
	    rhsknap += up[j];
	    if( y[j] < up[j] ) 
	      {
		cand[j] = PRIME;
		ratio[j] = x[j] / up[j];
		asum += up[j];
	      }
	  }
	break;
      }
    }




  double diff, d;
  int idx;
  
  while( asum < rhsknap + EPSILON ) 
    {
      diff = DBL_MAX;
      for( j = 0; j < nz; j++ ) 
	{
	  if( cand[j] == SECONDARY ) 
	    {
	      if( ( d = up[j] * x[j] - y[j] ) < diff - EPSILON )
		{
		  diff = d;
		  idx = j;
                }
            }
        }

      if( diff > ( (1.0 - EPSILON) * DBL_MAX ) ) 
	{
	  goto EXIT;
        }
      else 
	{
	  asum += up[idx];
	  cand[idx] = PRIME;
        }
    }


  
  /*
   * AT THIS POINT WE KNOW A COVER EXISTS AND SOLVE THE KNAPSACK PROBLEM
   * APPROXIMATELY
   */
     
  asum = (double) 0;
  for( j = 0; j < nz; j++ ) 
    {
      if( cand[j] == PRIME && ratio[j] < EPSILON ) 
	{
	  mark[j] = INCUT;
	  asum += up[j];
        }
    }
  
  while( asum < rhsknap + EPSILON ) 
    {
      min = DBL_MAX;
      for( j = 0; j < nz; j++ ) 
	{
	  if( cand[j] == PRIME && mark[j] == 0 && ratio[j] < min ) 
	    {
	      min = ratio[j];
	      idx = j;
            }
        }
      mark[idx] = INCUT;
      asum += up[idx];
    }
  

  /*
   * AT THIS POINT WE HAVE A COVER AND WE REDUCE IT TO A MINIMAL COVER
   */
     
  for( j = 0; j < nz; j++ ) 
    {
      if( mark[j] == INCUT && ratio[j] > EPSILON ) 
	{
	  if( asum - up[j] > rhsknap + EPSILON ) 
	    {
	      mark[j] = 0;
	      asum -= up[j];
            }
        }
    }
        
  for( j = 0; j < nz; j++ ) 
    {
      if( mark[j] == INCUT && ratio[j] < EPSILON) 
	{
	  if (asum - up[j] > rhsknap + EPSILON) 
	    {
	      mark[j] = 0;
	      asum -= up[j];
            }
        }
    }

  /*
   * COMPLEMENT THE VARIABLES IN N-
   */
     
  for( j = 0; j < nz; j++ ) 
    {
      if( sign[j] < 0 ) 
	mark[j] = 1 - mark[j];
    }

  lambda = asum - rhsknap;


  /*
   * GENERATE A VIOLATED LSGFC
   */
     
  /*
   * Initialize
   */
  
  for( j = 0; j < nz; j++ ) 
    {
      lo[j] = (double) 0;
      xcoe[j] = (double) 0;
      ycoe[j] = (double) 0;
    }
  
  *cut_rhs = rhs;

  /*
   * Project out variables in C-
   */
     
  for( j = 0; j < nz; j++) 
    {
      if( mark[j] == INCUT && sign[j] < 0 ) 
	*cut_rhs += up[j];
    }

  /*
   * (1) Compute the coefficients of the simple generalized flow cover
   * (2) Compute minpl, minneg and sum
   */

  tmp = *cut_rhs;

  for( j = 0; j < nz; j++ ) 
    {
      if( ( mark[j] == INCUT ) && sign[j] > 0 ) // C+
	{
	  ycoe[j] = 1.0;
	  if( up[j] > lambda + EPSILON ) // C++
	    {
	      xcoe[j] = lambda - up[j];
	      *cut_rhs += xcoe[j];
	      if( up[j] < minpls ) 
		{
		  minpls = up[j];
                }
            }
	  else  // C+\C++
	    {
	      xcoe[j] = 0.0;
	      sum += up[j];
            } 
        }
      
      if( ( mark[j] != INCUT ) && sign[j] < 0 ) // N-\C-
	{
	  tmp += up[j];
	  if( y[j] >= lambda * x[j] ) // L-
	    {
	      ycoe[j] = 0.0;
	      xcoe[j] = -lambda;
	      mark[j] = INLMIN;
	      if( up[j] < minneg ) 
		{
		  minneg = up[j];
                }
	    }
	  else // L--
	    {
	      ycoe[j] = -1.0;
	      xcoe[j] = 0.0;
	      mark[j] = INLMINMIN;
	      sum += up[j];
	    }
        }
    }
  

  /*
   * Sort the upper bounds (m sub j) of variables in C++ and L-.
   */
  
  int ix;
  for( ;; ) 
    {
      ix = UNDEFINED;
      value = lambda;
      for( j = 0; j < nz; j++ ) 
	{
	  if( ( mark[j] == INCUT && sign[j] > 0 ) || mark[j] == INLMIN ) 
	    {
	      if( up[j] > value ) 
		{
		  ix = j;
		  value = up[j];
                }
            }
        }
      
      if( ix == UNDEFINED ) 
	{
	  break;
	}
      
      mt[splus++] = up[ix];
      if( mark[ix] == INLMIN ) 
	{
	  mark[ix] = INLMINDONE;
        }
      else 
	{
	  mark[ix] = INCUTDONE;
        }
    }

    for( j = 0; j < nz; j++ ) 
      {
        switch( mark[j] ) {
        case INCUTDONE:
	  mark[j] = INCUT;
	  break;
        case INLMIN:
        case INLMINDONE:
        case INLMINMIN:
	  mark[j] = 0;
	  break;
        }
      }
  
    
  tmp1 = tmp;


  i = 0;

  /*
   * If minpls <= minneg or if sum >= lambda then the
   * lifting function itself is super additive, so we do not
   * need to work with a super additive valid lifting function.
   * See also Theorem 7.10
   */

  if( ( minpls > minneg ) && ( sum < lambda ) )
    {

      /*
       * Compute break point (t in thesis)
       */
         
      for( j = 0; j < splus; j++ ) 
	{
	  if ( mt[j] < minpls ) 
	    {
	      i = j;
	      break;
            }
        }

      /*
       * lo computations
       *
       * Most of the time this corresponds to MAX(mp - m sub i, lambda - m)
       */

      if( i < splus - 1 ) 
	{
	  
	  /*
	   * Preliminary work
	   */

	  if( i > 0 ) 
	    {
	      lo[i-1] = minpls - mt[i];
	      if( lo[i-1] > lambda ) 
		{
		  lo[i-1] = lambda;
                }
	      if( lambda - sum < lo[i-1] ) 
		{
		  lo[i-1] = lambda - sum;
                }
            }

            /*
             * Set rho values
             */
             
	  for( j = i; j < splus; j++ ) 
	    {
	      if( j == 0 ) 
		{
		  lo[j] = mt[j] - mt[j+1];
                }
	      else 
		{
		  lo[j] = mt[j] - mt[j+1] + lo[j-1];
		}

	      if( lo[j] > lambda ) 
		{
		  lo[j] = lambda;
                }

	      if( lambda - sum < lo[j] ) 
		{
                    lo[j] = lambda - sum;
                }
            }

	  //
	  // Compute tmp1
	  //
         
	  if( sum == 0.0 ) 
	    {
	      tmp1 = -lambda;
	      for( j = 0; j < i; j++ ) 
		{
		  tmp1 += mt[j];
		}
            }
        }
    }

  if( splus == 0 ) 
    {
      goto EXIT;
    }

  /*
   * Compute cumulative upper bound values
   */
  
  for( j = 1; j < splus; j++ ) 
    {
      mt[j] += mt[j-1];
    }

  /*
   * COMPUTE LIFTING COEFFICIENTS
   */

  for( j = 0; j < nz; j++ ) 
    {
      if( mark[j] != INCUT && sign[j] == CONTPOS ) // N+\C+
	{
#if 0
	  chdbg << "Calling liftcoe1 with the following arguments" << endl;
	  chdbg << ycoe[j] << " " << xcoe[j] << " " << up[j] << " " << x[j] << " " << y[j] << " " << splus << " " << tmp1 << " " << lambda << endl;
#endif
	  liftcoe1( &ycoe[j], &xcoe[j], up[j], x[j], y[j], 
		   splus, tmp1, lambda );
        }

      if( mark[j] != INCUT && sign[j] == BINPOS ) // N+\C+
	{
	  ycoe[j] = 0.0;
	  liftcoe2( &xcoe[j], up[j], splus, tmp, lambda );
	  xcoe[j] = -xcoe[j];
	  if( xcoe[j] < EPSILON )
	    xcoe[j] = 0.0;
        }

      if( mark[j] == INCUT && sign[j] < 0 ) // C-
	{
	  ycoe[j] = 0.0;
	  liftcoe2( &xcoe[j], up[j], splus, tmp, lambda );
	  if( -xcoe[j] < EPSILON ) 
	    {
	      xcoe[j] = 0.0;
	    }
	  else 
	    {
	      *cut_rhs += xcoe[j];
            }
        }
    }
  

  /*
   * COMPUTE VIOLATION OF LSGFC
   */

#if 0
  chdbg << "cut_rhs = " << *cut_rhs << endl;
#endif

  violation = -(*cut_rhs);
  for( j = 0; j < nz; j++ ) 
    {
#if 0
	  chdbg << "j = " << j << " ind = " << ind[j] << " sign = " << sign[j] << " coef = " << coef[j] << " x = " << x[j] << " xcoe = " << xcoe[j] << " y = " << y[j] << " ycoe = " << ycoe[j] << " up = " << up[j] << " mark = " << mark[j] << endl;
#endif
      violation += y[j] * ycoe[j] + x[j] * xcoe[j];
    }

#if 0
  chdbg << "violation = " << violation << endl;
#endif

  if ( violation > FLOW_TOLERANCE ) 
    {
      int ncol = prob->get_nvars();
      pcut_ind = NEWV( int, ncol );
      pcut_coef = NEWV( double, ncol );
      
      *cut_nz = 0;
      *cut_sense = 'L';
		
      /*
       * Transform the lifted simple generalize flow cover
       */
      
      for ( j = 0; j < nz; j++ ) 
	{
	  vub = prob->get_vub()[ind[j]];

	  if( ( sign[j] == CONTPOS ) || ( sign[j] == CONTNEG ) ) 
	    {
	      if( ABS( ycoe[j] ) > EPSILON )
		{
		  if( sign[j] == CONTPOS ) 
		    {
		      pcut_coef[*cut_nz] = coef[j] * ycoe[j];
		    }
		  else 
		    {
		      pcut_coef[*cut_nz] = -coef[j] * ycoe[j];
		    }
		  pcut_ind[(*cut_nz)++] = ind[j];
		}
	      
	      if( ABS( xcoe[j] ) > EPSILON )
		{
		  if ( vub.get_var() != UNDEFINED )
		    {
		      pcut_coef[*cut_nz] = xcoe[j];
		      pcut_ind[(*cut_nz)++] = vub.get_var();
		    }
		  else
		    {
		      *cut_rhs -= xcoe[j];
		    }
                }
            }
            
	  if( ( sign[j] == BINPOS ) || ( sign[j] == BINNEG ) )
	    {
	      if( ( ABS( ycoe[j] ) > EPSILON ) || ( ABS( xcoe[j] ) > EPSILON ) )
		{
		  if( sign[j] == BINPOS ) 
		    {
		      pcut_coef[*cut_nz] = coef[j]*ycoe[j]+xcoe[j];
#if defined( DEBUG_CUTS2 )
		      cout << "Coef[" << ind[j] << "] = " <<  pcut_coef[*cut_nz] 
			   << endl;
#endif
                    }
                    else 
		      {
			pcut_coef[*cut_nz] = -coef[j]*ycoe[j]+xcoe[j];
#if defined( DEBUG_CUTS2 )
		      cout << "Coef[" << ind[j] << "] = " <<  pcut_coef[*cut_nz] 
			   << endl;
#endif
		      }
		  pcut_ind[(*cut_nz)++] = ind[j];
                }
            }
        }

      /*
       * REMOVE DOUBLE OCCURENCES OF VARIABLES AND VARIABLES WITH ZERO
       * COEFFICIENTS
       */
      
      for( j = 0; j < *cut_nz; j++ ) 
	{
	  for( i = 0; i < j; i++ ) 
	    {
	      if( pcut_ind[i] == pcut_ind[j] )
		{
		  pcut_coef[i] += pcut_coef[j];
		  pcut_ind[j] = -1;
                }
            }
        }

      for( i = 0, j = 0; j < *cut_nz; j++ ) 
	{
	  if( ( pcut_ind[j] == -1 ) || ( ( SMALL( pcut_coef[j]) ) ) )
	    {
	      // DO NOTHING.  Removing duplicate variable.
	      ;
	    }
	  else 
	    {
	      pcut_coef[i] = pcut_coef[j];
	      pcut_ind[i] = pcut_ind[j];
	      i++;
            }
        }
      
      *cut_nz = i;

      //XXX??
      // Jeff -- In pk1, all coefficients are 0.  I think if there 
      //  are no REAL variable upper bounds (like pk1), you can get
      //  a cut all of whose coefficients are 0.  And might be reported
      //  to be violated???

      if( *cut_nz == 0 )
	{
	  DELETEV( pcut_ind );
	  DELETEV( pcut_coef );
	  goto EXIT;
	}
        
      /*
       * Defensive programming
       *
       * Check the violation again !! Due to numerical instability it
       * is possible that the generated constraint is no longer violated !!
       */

      violation = (double) 0;
      for( j = 0; j < *cut_nz; j++ ) 
	{
	  violation += pcut_coef[j] * xlp[pcut_ind[j]];
        }
      violation -= *cut_rhs;

      if ( violation > FLOW_TOLERANCE )
	{
	  // chout << "generated a flow cover with violation " << violation << endl;
	  generated = true;
	}
      else
	{
	  cerr << "flow_cover(): Numerical stability problems" << endl << "Returning" << endl;

	  DELETEV( pcut_ind );
	  DELETEV( pcut_coef );
	  goto EXIT;
	}
    }

  *cut_ind = pcut_ind;
  *cut_coef = pcut_coef;


  if ( generated )
    {
      *priority = violation;
    }
  
 EXIT:

  return ( generated );

}

void CFlowCutGen::liftcoe1( double *cyj, double *cxj, double upj, 
			   double xj, double yj, int splus, double tmp1, 
			   double lambda)
{
    int i, k;
    double tcxj = 0.0, tcyj = 0.0;   /* tcxj and tcyj are temporary lifting
                                        coefficients for xj and yj */

    /*
     * Determine interval of m sub j
     */
     
    if ( upj > mt[splus-1] - lambda + EPSILON ) 
      {
	if ( upj < tmp1 - EPSILON ) 
	  {
	    if ( yj - (mt[splus-1] - splus * lambda) * xj > 
		tcxj * xj + tcyj * yj ) 
	      {
		tcyj = 1.0;
		tcxj = -( mt[splus-1] - splus * lambda );
	      }
	  }
      }
    else 
      {
	for ( i = 0; i < splus; i++ ) 
	  {
	    if ( upj <= mt[i] ) 
	      {
		break;
	      }
	  }

	k = i;
	if ( upj > mt[k] - lambda ) 
	  {
	    if ( yj - (mt[k] - (k+1) * lambda) * xj > tcxj * xj + tcyj * yj ) 
	      {
		tcyj = 1.0;
		tcxj = -( mt[k] - (k+1) * lambda );
	      }
	  }
      }
   
    *cyj = tcyj;
    *cxj = tcxj;
  }


void CFlowCutGen::liftcoe2( double *cxj, double upj, int splus, double tmp, 
			  double lambda )
{
    int i;
 
    if ( upj > tmp ) 
      {
	*cxj = -tmp + mt[splus-1] - splus * lambda;
      }
    else 
      {
	if ( upj > mt[splus-1] - lambda ) 
	  {
	    *cxj = -upj + mt[splus-1] - splus * lambda;
	  }
	else 
	  {
	    for ( i = 0; i < splus; i++ ) 
	      {
		if ( upj < mt[i] - lo[i] ) 
		  {
		    break;
		  }
	      }
	    if (upj > mt[i] - lambda) 
	      {
		*cxj = -upj + mt[i] - (i+1) * lambda;
	      }
	    else 
	      {
		*cxj = -i * lambda;
	      }
	  }
      }
}
