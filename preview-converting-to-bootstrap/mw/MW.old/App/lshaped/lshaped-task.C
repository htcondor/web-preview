/* 
   The implementation of the LShapedTask class.
*/

#include "lshaped-task.h"
#include "MW.h"

#include <lpsolver.h>
#include <iostream.h>
#include <string.h>

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

LShapedTask::LShapedTask()
{
  iteration = startIx = endIx = mncols = UNDEFINED;
  mxlp = NULL;
  theta = -LPSOLVER_INFINITY;
  thetaIx = UNDEFINED;
  
  taskTime = -1.0;
  ncuts = 0;
  cutnnzero = 0;

  cutType = NULL;
  cutbeg = NULL;
  cutind = NULL;
  cutval = NULL;
  cutrhs = NULL;

  error = LPSOLVER_INFINITY;
  sscost = LPSOLVER_INFINITY;
}


LShapedTask::LShapedTask( int iter, int six, int eix, int nc, const double x[], 
			  double theta, int tix )
{
  iteration = iter;
  startIx = six;
  endIx = eix;

#if 0
  cout << "Making task with startIx = " << startIx << " endIx = " 
       << endIx << endl;
#endif

  assert( endIx >= startIx );
  mncols = nc;

  mxlp = new double [mncols];
  memcpy( mxlp, x, mncols * sizeof( double ) );

  this->theta = theta;
  thetaIx = tix;

  taskTime = -1.0;  
  ncuts = 0;
  cutType = NULL;
  cutbeg = NULL;
  cutind = NULL;
  cutval = NULL;
  cutrhs = NULL;

  error = LPSOLVER_INFINITY;
  sscost = LPSOLVER_INFINITY;
}

LShapedTask::~LShapedTask() 
{
  // This had better check if NULL (as advertised)

  delete [] mxlp;
  delete [] cutType;
  delete [] cutbeg;
  delete [] cutind;
  delete [] cutval;
  delete [] cutrhs;
}


void LShapedTask::pack_work( void ) 
{
  pvm_pkint( &iteration, 1, 1 );
  pvm_pkint( &startIx, 1, 1 );
  pvm_pkint( &endIx, 1, 1 );
  pvm_pkint( &mncols, 1, 1 );
  pvm_pkdouble( mxlp, mncols, 1 );
  pvm_pkdouble( &theta, 1, 1 );

  // Once you are done, you need to delete what you have packed!
  // NO!  Because you may need to "repack" it if you
  // send this task off again!  Dummy!

}


void LShapedTask::unpack_work( void ) 
{
  pvm_upkint( &iteration, 1, 1 );
  pvm_upkint( &startIx, 1, 1 );
  pvm_upkint( &endIx, 1, 1 );
  pvm_upkint( &mncols, 1, 1 );
  mxlp = new double [mncols];
  pvm_upkdouble( mxlp, mncols, 1 );
  pvm_upkdouble( &theta, 1, 1 );

}


void LShapedTask::pack_results( void ) 
{

  pvm_pkdouble( &taskTime, 1, 1 );
  pvm_pkint( &ncuts, 1, 1 );  
  pvm_pkdouble( &error, 1, 1 );
  pvm_pkdouble( &sscost, 1, 1 );

  if( ncuts > 0 )
    {
      pvm_pkint( &cutnnzero, 1, 1 );  
      pvm_pkint( cutType, ncuts, 1 );
      pvm_pkint( cutbeg, (ncuts+1), 1 );
      pvm_pkint( cutind, cutnnzero, 1 );
      pvm_pkdouble( cutval, cutnnzero, 1 );
      pvm_pkdouble( cutrhs, ncuts, 1 );
    }

  // Once you are done packing, it is important to delete the
  // memory allocated in the task!  Allocate ALL "task memory"
  // using new.

  if( mxlp ) delete [] mxlp;
  mxlp = NULL;
  if( cutType ) delete [] cutType;
  cutType = NULL;
  if( cutbeg ) delete [] cutbeg;
  cutbeg = NULL;
  if( cutind ) delete [] cutind;
  cutind = NULL;
  if( cutval ) delete [] cutval;
  cutval = NULL;
  if( cutrhs ) delete [] cutrhs;
  cutrhs = NULL;  
}


void LShapedTask::unpack_results( void ) 
{
  pvm_upkdouble( &taskTime, 1, 1 );
  pvm_upkint( &ncuts, 1, 1 );
  pvm_upkdouble( &error, 1, 1 );
  pvm_upkdouble( &sscost, 1, 1 );

  if( ncuts > 0 )
    {
  
      pvm_upkint( &cutnnzero, 1, 1 );

      cutType = new int [ncuts];
      cutbeg = new int [ncuts + 1];
      cutind = new int [cutnnzero];
      cutval = new double [cutnnzero];
      cutrhs = new double [ncuts];

      pvm_upkint( cutType, ncuts, 1 );
      pvm_upkint( cutbeg, (ncuts+1), 1 );
      pvm_upkint( cutind, cutnnzero, 1 );
      pvm_upkdouble( cutval, cutnnzero, 1 );
      pvm_upkdouble( cutrhs, ncuts, 1 );
    }

#if defined( PRINT_TASK_RESULTS )
  cout << "Unpacked results: " << endl;
  cout << "ncuts: " << ncuts << "  error: " << error 
       << "  sscost: " << sscost << endl;
  PRINT( cout, "cutType", cutType, ncuts, 10 );
  PRINT( cout, "cutbeg", cutbeg, (ncuts+1), 10 );
  PRINT( cout, "cutind", cutind, cutnnzero, 10 );
  PRINT( cout, "cutval", cutval, cutnnzero, 10 );
  PRINT( cout, "cutrhs", cutrhs, ncuts, 10 );  
#endif

}

void LShapedTask::write_ckpt_info( FILE *fp )
{
  // You need only write the "work" portion of the task to the checkpoint
  // file
  fprintf( fp, "%d\n", iteration );
  fprintf( fp, "%d %d\n", startIx, endIx );
  fprintf( fp, "%d\n", mncols );
  for( int i = 0; i < mncols; i++ )
    fprintf( fp, "%.12le\n", mxlp[i] );
  fprintf( fp, "%d %.12le\n", thetaIx, theta );  
  
}

void LShapedTask::read_ckpt_info( FILE *fp )
{
  // You need only read the "work" portion of the task to the checkpoint
  // file

  fscanf( fp, "%d", &iteration );
  fscanf( fp, "%d %d", &startIx, &endIx );
  fscanf( fp, "%d", &mncols );
  mxlp = new double[mncols];
  for( int i = 0; i < mncols; i++ )
    fscanf( fp, "%le", &(mxlp[i]) );
  fscanf( fp, "%d %le", &thetaIx, &theta );  
  
}
