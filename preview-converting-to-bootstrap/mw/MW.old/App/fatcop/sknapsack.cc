/*---------------------------------------------------------------------------
--  FILENAME:  CUTGEN/sknapsack.C
--  AUTHOR:    Jeff T. Linderoth
--
--  This code is just an adaptation to PARINO and C++ of
--  M. Savelsbergh's code for lifted surrogate knapsack
--  cover inequalities.
--
---------------------------------------------------------------------------*/
/*****************************************************************************
*
*****************************************************************************/

/*** INCLUDES ***/

#include <stdio.h>
#include <stdlib.h>
#include "cutgen.hh"

/*** CONSTANTS ***/

#ifndef UNDEFINED
#define UNDEFINED -1
#endif

#define EPSILON       (1e-6)
#define ABS( x )      ( ( x ) < 0 ? -( x ) : ( x ) )
#define SMALL( x )    ( ABS( x ) < EPSILON )
#define FLOOR( x )    floor( (double) ( x ) )
#define FRAC( x )      ( (double) ( x ) - FLOOR ( x + EPSILON) )

/*****************************************************************************
*
*****************************************************************************/

int CSKnapsackCutGen::generate( const CProblemSpec *problem,
    const CLPSolution *sol, int max_cuts, CCut **cuts )
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
  CBool gen;

  int nrow = problem->get_nrows();
  int ncol = problem->get_nvars();

  int ngen = 0;
  int j;

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
	  gen = sknapsack_cover( problem, sol, nz, ind, coef, 'L', rhs,
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
	      cuts[ngen]->set_sense( ( cut_sense == 'L' ? LESS_OR_EQUAL 
				      : cut_sense == 'E' ? EQUAL_TO : GREATER_OR_EQUAL ) );
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
	  for( j = 0; j < nz; j++ )
	    {
	      ind[j] = ind_const[j];
	      coef[j] = coef_const[j];
	    }

	  gen = sknapsack_cover( problem, sol, nz, ind, coef, 'G', rhs,
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
	      cuts[ngen]->set_sense( ( cut_sense == 'L' ? LESS_OR_EQUAL 
				       : cut_sense == 'E' ? EQUAL_TO : GREATER_OR_EQUAL ) );
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
	  gen = sknapsack_cover( problem, sol, nz, ind, coef, sense, rhs,
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
	      cuts[ngen]->set_sense( ( cut_sense == 'L' ? LESS_OR_EQUAL 
				       : cut_sense == 'E' ? EQUAL_TO : GREATER_OR_EQUAL ) );
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

      DELETEV( ind ); ind = 0;
      DELETEV( coef ); coef = 0;
      
    }

 EXIT:

  if ( ind ) DELETEV( ind );
  if ( coef ) DELETEV( coef );

#ifdef DEBUG_CUTS
  cout << "Generated " << ngen << " surrogate knapsack cover inequalities" << endl;
  for( i = 0; i < ngen; i++ )
    cout << (*cuts[i]) << endl;
#endif


  return ngen;
}

/*****************************************************************************
*
*****************************************************************************/

CBool CSKnapsackCutGen::sknapsack_cover
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

  const double *xlp = sol->get_xlp();
  
  if ( sense == 'G' )
    flip( nz, ind, coef, &rhs );

  sk_nzcnt = 0;
  sk_sense = 'L';
  sk_rhs = rhs;

  double lj, uj;

  CVLB vlb;
  CVUB vub;

  CBool fracsol = FALSE;

  int j;


  for( j = 0; j < nz; j++ )
    {
      vlb = prob->get_vlb()[ind[j]];
      lj = ( vlb.get_var() != UNDEFINED ) ? vlb.get_val() 
	: prob->get_bdl()[ind[j]];

      vub = prob->get_vub()[ind[j]];
      uj = ( vub.get_var() != UNDEFINED ) ? vub.get_val() 
	: prob->get_bdu()[ind[j]];
      
      if ( prob->get_colmajor()->get_ctype()[ind[j]] != COL_TYPE_BINARY )
	{
	  if ( coef[j] > EPSILON )
	    {
	      if ( vlb.get_var() == UNDEFINED )
		{
		  sk_rhs -= coef[j] * lj;
		}
	      else
		{
		  sk_val[sk_nzcnt] = coef[j] * lj;
		  sk_ind[sk_nzcnt++] = vlb.get_var();
		  if ( FRAC( xlp[vlb.get_var()] ) > EPSILON )
		    {
		      fracsol = TRUE;
		    }
		}
	    }
	  else
	    {
	      if ( vub.get_var() == UNDEFINED )
		{
		  sk_rhs -= coef[j] * uj;
		}
	      else
		{
		  sk_val[sk_nzcnt] = coef[j] * uj;
		  sk_ind[sk_nzcnt++] = vub.get_var();
		  if ( FRAC( xlp[vub.get_var()] ) > EPSILON )
		    {
		      fracsol = TRUE;
		    }
		}
	    }

	}
    }

  if ( ! fracsol )
    {
      return( FALSE );
    }

  /*
   * REMOVE DOUBLE OCCURENCES OF VARIABLES AND VARIABLES WITH ZERO
   * COEFFICIENTS
   */
  
  int i = 0;
  for ( j = 0; j < sk_nzcnt; j++ ) 
    {
      for ( i = 0; i < j; i++ ) 
	{
	  if ( sk_ind[i] == sk_ind[j] ) 
	    {
	      sk_val[i] += sk_val[j];
	      sk_ind[j] = -1;
            }
        }
    }

    for ( i = 0, j = 0; j < sk_nzcnt; j++ ) 
      {
        if ( sk_ind[j] == -1 || ( SMALL( sk_val[j] ) ) ) 
	  {
	    /* DO NOTHING */ ;
	  }
        else 
	  {
            sk_val[i] = sk_val[j];
            sk_ind[i] = sk_ind[j];
            i++;
	  }
      }

    sk_nzcnt = i;

    if ( sk_nzcnt == 0 ) 
      {
        return (FALSE);
      }

    return ( knapsack_cover( prob, xlp, sol->get_dj(), sk_nzcnt, sk_ind, sk_val, 
			     sk_sense, sk_rhs, cut_nz, cut_ind, cut_coef, 
			     cut_sense, cut_rhs, priority ) );

}

