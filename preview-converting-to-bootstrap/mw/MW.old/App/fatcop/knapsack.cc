/*---------------------------------------------------------------------------
--  FILENAME:  knapsack.cc
--  DATE:      Thu Aug 22 10:37:30 EDT 1996
--  AUTHOR:    Jeff T. Linderoth
--
--  This code is just an adaptation to FATCOP and C++ of
--  M. Savelsbergh's code for lifted knapsack cover inequalities.
--
---------------------------------------------------------------------------*/
/*****************************************************************************
*
*****************************************************************************/

/*** INCLUDES ***/

#include <stdio.h>
#include <stdlib.h>
#include "cutgen.hh"

//#define FALSE false
//#define TRUE true

#define INFBOUND 1.0e+100
#define KNAP_THRESHOLD -INFBOUND

/*** FUNCTIONS ***/

/*****************************************************************************
*
*****************************************************************************/

int CKnapsackCutGen::generate( const CProblemSpec *problem,
			       const CLPSolution *sol, int max_cuts, 
			       CCut **cuts )
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
  
  const double *xlp = sol->get_xlp();  
  const double *dj = sol->get_dj();

  int nrow = problem->get_nrows();
  int ncol = problem->get_nvars();

  int ngen = 0;       
  int i;
  for( i = 0; i < nrow ; i++ )
    {
      rtype = problem->get_rowmajor()->get_rtype()[i];
      if( ( rtype != ROW_TYPE_ALLBINUB ) &&
	  ( rtype != ROW_TYPE_ALLBINEQ ) &&
	  ( rtype != ROW_TYPE_BINSUMVARUB ) &&
	  ( rtype != ROW_TYPE_BINSUMVAREQ ) &&
	  ( rtype != ROW_TYPE_BINSUM1VARUB ) &&
	  ( rtype != ROW_TYPE_BINSUM1VAREQ ) )
	continue;


      nz = problem->get_rowmajor()->get_matcnt()[i];
      sense = problem->get_senx()[i];
      rhs = problem->get_rhsx()[i];       	
      ind_const = problem->get_rowmajor()->get_matind() +
	problem->get_rowmajor()->get_matbeg()[i] ;
      coef_const =  problem->get_rowmajor()->get_matval() +
	problem->get_rowmajor()->get_matbeg()[i];

      ind = NEWV( int, nz );
      coef = NEWV( double, nz );
      
      for( int j = 0; j < nz; j++ )
	{
	  ind[j] = ind_const[j];
	  coef[j] = coef_const[j];
	}

      if( sense == 'E' )
	{ 
#if defined( TIMER )
	  CAbsTime t1;
#endif

	  gen = knapsack_cover( problem, xlp, dj, nz, ind, coef, 'L', rhs,
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
				       : cut_sense == 'E' ? 
				       EQUAL_TO : GREATER_OR_EQUAL ) );
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
	  for( int j = 0; j < nz; j++ )
	    {
	      ind[j] = ind_const[j];
	      coef[j] = coef_const[j];
	    }

	  gen = knapsack_cover( problem, xlp, dj, nz, ind, coef, 'G', rhs,
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
	      cuts[ngen]->set_sense( ( cut_sense == 'L' ? LESS_OR_EQUAL : 
				       cut_sense == 'E' ? 
				       EQUAL_TO : GREATER_OR_EQUAL ) );
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
	  gen = knapsack_cover( problem, xlp, dj, nz, ind, coef, sense, rhs,
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
	      cuts[ngen]->set_sense( ( cut_sense == 'L' ? LESS_OR_EQUAL : 
				       cut_sense == 'E' ? 
				       EQUAL_TO : GREATER_OR_EQUAL ) );
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

      if( ind )  DELETEV( ind ); ind = NULL;
      if( coef ) DELETEV( coef ); coef = NULL;
    
    }

 EXIT:

  if( ind )  DELETEV( ind );
  if( coef ) DELETEV( coef );
 
#ifdef DEBUG_CUTS
  cout << "Generated " << ngen << " knapsack cuts" << endl;
  for( i = 0; i < ngen; i++ )
    cout << (*cuts[i]) << endl; 
#endif

  return ngen;
}

/*****************************************************************************
*
*  This function is passed a pure knapsack row and returns 
*  a lifted knapsack cover
*  
*****************************************************************************/
bool CKnapsackCutGen::knapsack_cover
(
 const CProblemSpec *p,     // The problem specification.
 const double *xlp,         // An LP solution.
 const double *dj,          // The reduced costs for the LP solution.
 int nz,                    // These five arguments specify the knapsack
 int *ind,                  // row.
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
  bool generated = FALSE;
  int ncol = p->get_nvars();

  int *pcut_ind = NULL;
  double *pcut_coef = NULL;

  double violation = (double) 0;
  double valsum = (double) 0;
  int n1 = 0, n2 = 0;
  int fraccnt = 0, zerocnt = 0;

  if( sense == 'G' )
    flip( nz, ind, coef, &rhs );
  
  complement( nz, ind, coef, &rhs );
  
  kp_nzcnt = nz;
  kp_rhs = rhs;

  int j;

  for( j = 0; j < kp_nzcnt; j++ ) 
    {
      kp_ind[j] = ind[j];
      kp_val[j] = coef[j];
      kp_obj[j] = (ind[j] < COMPVAL) ? 1.0 - xlp[ind[j]] : xlp[ind[j]-COMPVAL];
    }

    /*
     * Order the variables in a way that reflects the current
     * LP-solution:
     *
     * [0,...,zerocnt-1]:              xlp = 1         kp_obj = 0
     * [zerocnt,...,fractcnt-1]:   0 < xlp < 1     0 < kp_obj < 1
     * [fraccnt,...,nzcnt-1]:          xlp = 0         kp_obj = 1
     */
     
    if( !ordervars ( kp_obj, kp_ind, kp_val, kp_nzcnt ) )
      {
	goto EXIT;
      }


  zerocnt = 0;
  fraccnt = 0;

  for( j = 0; j < kp_nzcnt; j++ ) 
    {
      mark[j] = FALSE;
      if ( kp_obj[j] < EPSILON ) 
	zerocnt++;
      else 
	if ( kp_obj[j] < 1 - EPSILON )
	  fraccnt++;
    }

  fraccnt += zerocnt;
  
  n1 = zerocnt;
  n2 = fraccnt;
  
  for( j = 0; j < n1; j++ )
    rhs -= kp_val[j];

  valsum = (double) 0;
  for( j = n1; j < n2; j++ )
    valsum += kp_val[j];

  if( valsum < rhs + EPSILON )   // No cover exists
    {
      goto EXIT;
    }

//  Allocate memory for the cut

  pcut_ind = NEWV( int, ncol );
  pcut_coef = NEWV ( double, ncol );

  *cut_nz = 0;
  *cut_sense = 'L';
  *cut_rhs = (double) -1;

  valsum = (double) 0;
  while( valsum < rhs + EPSILON )
    {

      /*
       * DETERMINE THE NEXT VARIABLE TO BE INCORPORATED IN THE COVER
       */
      
      /*
       * Solve the linear programming relaxation of the coefficient
       * independent separation problem and round up to find the
       * initial cover
       */
 
      double ratio = INFBOUND;
      int ix;
      for( j = n1; j < n2; j++ ) 
	{
	  if( mark[j] == FALSE && kp_obj[j] < ratio ) 
	    {
	      ratio = kp_obj[j];
	      ix = j;
	    }
	}

      mark[ix] = TRUE;
      valsum += kp_val[ix];
      *cut_rhs += (double) 1;
    }


  /*
   * Turn the constructed cover into a minimal cover
   */
         
  for( j = n1; j < n2; j++ ) 
    {
      if( mark[j] == TRUE ) 
	{
	  if( valsum - kp_val[j] > rhs + EPSILON ) 
	    {
	      mark[j] = FALSE;
	      valsum -= kp_val[j];
	      *cut_rhs -= (double) 1;
	    }
	  else 
	    {
	      pcut_ind[*cut_nz] = kp_ind[j];
	      pcut_coef[(*cut_nz)++] = (double) 1;
	    }
	}
    }  

  /*
   * Check for violation
   */

  violation = (double) 0; 
  for( j = 0; j < *cut_nz; j++ ) 
    {
      if( pcut_ind[j] < COMPVAL ) 
	violation += pcut_coef[j] * xlp[pcut_ind[j]];
      else
	violation += pcut_coef[j] * (1.0 - xlp[pcut_ind[j] - COMPVAL]);        
    }

  violation -= *cut_rhs;

  
  if( violation < KNAP_THRESHOLD )
    {
      if ( pcut_ind ) delete [] pcut_ind;
      if ( pcut_coef ) delete [] pcut_coef;
      goto EXIT;
    }

  /*
   * LIFT VARIABLES
   *
   * Note: - Variables that are at their lower bound in the current
   *         LP-solution will not contribute to any violation even
   *         if they are lifted;
   *       - Variables that are at their upper bound in the current
   *         LP-solution will not constribute to any violation even
   *         if they are lifted;
   *
   *       HOWEVER, it may have some effect later on !!!
   */
  
  /*
   * Set up the dynamic programming algorithm for the knapsack
   */
  
  knapinit ();

  for( j = n1; j < n2; j++ )
    if( mark[j] == TRUE )
      if( !knapvar( kp_val[j], 1 ) )
	{
	  if ( pcut_ind ) DELETEV( pcut_ind );
	  if ( pcut_coef ) DELETEV( pcut_coef );	  

	  goto EXIT;
	}

  /*
   * Lift variables with xlp > 0 not in C
   */

  double ratio, t2;
  int t1, alpha, ix;

  
  for(;;) 
    {
    
      /*
       * DETERMINE THE NEXT VARIABLE TO BE LIFTED
       */

      /*
       * Select the variable that maximizes alpha * xlp, i.e., greedy
       */
      
      ratio = -INFBOUND; 
      int ix = UNDEFINED;
      for( j = n1; j < n2; j++ ) 
	{
	  if( mark[j] == FALSE && kp_val[j] < rhs + EPSILON ) 
	    {
	      t1 = (int) *cut_rhs - knapvalue( rhs - kp_val[j] );
	      t2 = (1.0 - kp_obj[j]) * t1;
	      if( t2 > ratio + EPSILON ) 
		{
		  ratio = t2;
		  ix = j;
		}
	    }
	}
      
      if( ix == UNDEFINED )
	{
	  break;
	}

      else 
	{
	  mark[ix] = TRUE;
	  alpha = (int) *cut_rhs - knapvalue( rhs - kp_val[ix] );            
	  if(alpha > 0) 
	    {
	      if( !knapvar( kp_val[ix], alpha ) ) 
		{
		  if ( pcut_ind ) DELETEV( pcut_ind );
		  if ( pcut_coef ) DELETEV( pcut_coef );

		  goto EXIT;
		}
	      pcut_ind[*cut_nz] = kp_ind[ix];
	      pcut_coef[(*cut_nz)++] = alpha;
	    }
	}
    }

  
  /*
   *  Check for violation
   */
  
  violation = (double) 0;
  for( j = 0; j < *cut_nz; j++ )
    if( pcut_ind[j] < COMPVAL )
      violation += pcut_coef[j] * xlp[pcut_ind[j]];
    else
      violation += pcut_coef[j] * ( 1.0 - xlp[pcut_ind[j] - COMPVAL] );
  violation -= *cut_rhs;

  if( violation < KNAP_TOLERANCE )  // We didn't find a violated cover
    {
      if ( pcut_ind) DELETEV( pcut_ind );
      if ( pcut_coef ) DELETEV( pcut_coef );	  

      goto EXIT;      
    }

  /*
   * Lift variables that have been projected out
   */


  for(;;) 
    {
      /*
       * DETERMINE THE NEXT VARIABLE TO BE LIFTED
       */

      /*
       * Select the variable with the largest reduced cost in the
       * current LP solution
       */
      
      ratio = -INFBOUND; 
      ix = UNDEFINED;
      int gamma;

      for( j = 0; j < n1; j++ )
	{
	  if( mark[j] == FALSE && dj[KP_IND(j)] > ratio + EPSILON ) 
	    {
	      ratio = dj[KP_IND(j)];
	      ix = j;
	    }
	}


      if(ix == UNDEFINED) 
	{
	  break;
	}      

      else 
	{
	  mark[ix] = TRUE;
	  rhs += kp_val[ix];
	  gamma = (int) -(*cut_rhs) + knapvalue ( rhs );
            
	  if(gamma > 0) 
	    {
	      if( !knapvar ( kp_val[ix], gamma ) ) // DP Failed. 
		{
		  if ( pcut_ind ) DELETEV( pcut_ind );
		  if ( pcut_coef ) DELETEV( pcut_coef );

		  goto EXIT;
                }
                
	      pcut_ind[*cut_nz] = kp_ind[ix];
	      pcut_coef[(*cut_nz)++] = (double) gamma;
	      *cut_rhs += (double) gamma;
            }
        }
    }


  /*
   * Check for violation again -- For numerical stability.
   */

    violation = (double) 0; 
    for( j = 0; j < *cut_nz; j++) 
      {
        if( pcut_ind[j] < COMPVAL ) 
	  violation += pcut_coef[j] * xlp[pcut_ind[j]];
        else
	  violation += pcut_coef[j] * (1.0 -xlp[pcut_ind[j] - COMPVAL]);
      }

  violation -= *cut_rhs;

  if(violation < KNAP_TOLERANCE) 
    {
      cerr << "knapsack_cover(): Numerical stability problems after gamma lifting" << endl << "Returning" << endl;
      
      if ( pcut_ind ) delete [] pcut_ind;
      if ( pcut_coef ) delete [] pcut_coef;

      goto EXIT;
    }
    
  /*
   * Lift remaining variables
   */


  for(;;) 
    {
      
      /*
       * DETERMINE THE NEXT VARIABLE TO BE LIFTED
       */

      /*
       * Select the variable with the largest reduced cost in the
       * current LP solution
       */

        ratio = -INFBOUND; 
	ix = UNDEFINED;

        for( j = n2; j < nz; j++ ) 
	  {
            if( mark[j] == FALSE && dj[KP_IND(j)] > ratio + EPSILON ) 
	      {
                ratio = dj[KP_IND(j)];
                ix = j;
	      }
	  }

        if(ix == UNDEFINED) 
	  {
            break;
	  }
        else 
	  {
            mark[ix] = TRUE;
            alpha = (int) (*cut_rhs) - knapvalue( rhs - kp_val[ix] );
            
            if(alpha > 0) 
	      {
                if( !knapvar ( kp_val[ix], alpha ) ) // DP Failed.
		  {
		    if ( pcut_ind ) DELETEV( pcut_ind );
		    if ( pcut_coef ) DELETEV( pcut_coef );
                    goto EXIT;
		  }

                pcut_ind[*cut_nz] = kp_ind[ix];
                pcut_coef[(*cut_nz)++] = (double) alpha;
	      }
	  }
      }



  /*
     Again, die to numerical stability, check the constraint 
     again for violation, and only add if it is violated.     
  */

  recomplement( *cut_nz, pcut_ind, pcut_coef, cut_rhs );


  violation = (double) 0; 
  for( j = 0; j < *cut_nz; j++) 
    violation += pcut_coef[j] * xlp[pcut_ind[j]];

  violation -= *cut_rhs;

  if( violation < KNAP_TOLERANCE ) 
    {
      cerr << "knapsack_cover(): Numerical stability problems after alpha lifting" << endl << "Returning" << endl;
      
      if ( pcut_ind ) DELETEV( pcut_ind );
      if ( pcut_coef ) DELETEV( pcut_coef );

      goto EXIT;
    }

  
  *cut_ind = pcut_ind;
  *cut_coef = pcut_coef;

  generated = TRUE;

// For the time being we will just have the priority be the violation.
  *priority = violation;

 EXIT:
  
  return generated;
}

/*****************************************************************************
*
*****************************************************************************/

bool CKnapsackCutGen::ordervars(double *a, int *b, double *c, int n)
{
  int i, j, k, t1, done;
  double t2;

  if (n >= ORDERSIZE)
    {
      cerr << "Knapsack: Cannot order variables." << endl;
      cerr << "Returning." << endl;

      return (FALSE);
    }
    
  for (k = 1; k < n+1; k++)
    {
      done = 0;
      a1[k] = a[k-1];  b1[k] = k-1;
      j = k; i = k/2;
      while (i > 0 && done == 0) 
	{
	  if (a1[i] <= a1[j])
	    {
	      done = 1;
	    }      
	  else {
	    t1 = b1[i];  b1[i] = b1[j];  b1[j] = t1;
	    t2 = a1[i];  a1[i] = a1[j];  a1[j] = t2;
	    j = i; i = i/2;
	  }
	}
    }

    for (k = 0; k < n; k++) {
       done = 0;
       a[k] = a1[1];  b0[k] = b1[1];
       i = 1;
       while (done == 0) {
          if (n < 2*i) {
	    a1[i] = INFBOUND;
             done = 1;
          }
          else if (n < 2*i+1) {
             a1[i] = a1[2*i];  b1[i] = b1[2*i];
             a1[2*i] = INFBOUND;  done = 1;
          }
          else if (a1[2*i] <= a1[2*i+1]) {
             a1[i] = a1[2*i];  b1[i] = b1[2*i];
             i = 2*i;
          }
          else {
             a1[i] = a1[2*i+1];  b1[i] = b1[2*i+1];
             i = 2*i+1;
          }
	}
     }

    for (k = 0; k < n; k++) {
       c1[k] = c[k];
       b1[k] = b[k];
    }

    for (k = 0; k < n; k++) {
       c[k] = c1[b0[k]];
       b[k] = b1[b0[k]];
     }

  return (TRUE);
}

bool CKnapsackCutGen::knapvar(double c, int a)
{
  int k;

  if ( a + 1 > KNAPSIZE ) 
    {
      cerr << "Knapsack: Coefficients get too big." << endl;
      return (FALSE);
    }
    
  if ( a > asum ) 
    {
      for ( k = 1; k <= asum; k++ )
	dp2[k] = MIN( dp1[k], c );
      
      for ( ; k <= a; k++ ) 
	dp2[k] = c;
    }    
  else 
    {
      for ( k = 1; k <= a; k++ ) 
	dp2[k] = MIN( dp1[k], c );
      
      for ( ; k <= asum; k++ )
	dp2[k] = MIN( dp1[k], dp1[k-a] + c );        
    }
  
  asum += a;
  
  if (asum + 2 > KNAPSIZE) 
    {
      cerr << "Knapsack: Coefficients get too big." << endl;
      return (FALSE);
    }
  
  for ( ; k <= asum; k++ ) 
    dp2[k] = dp1[k-a] + c;
  
  for ( k = 0; k <= asum; k++ )
    dp1[k] = dp2[k];
  
  dp1[asum+1] = INFBOUND;
  
  return (TRUE);
}
