#include  <stdio.h>

#include "PRINO.hh"
#include "darray.hh"

// This is needed to pack the formulation.
#include <MWRMComm.h>


void CProblemSpec::colmajor_to_rowmajor( int ncols,
					 const int colbeg[], 
					 const int colind[],
					 const double colval[],
					 int nrows, int rowbeg[], int rowind[],
					 double rowval[],
					 int rowcnt[], int nvals )
{
  int i, j;
  
  /* Count the number of nonzero elements per row */
  for( i = 0; i < nrows; i++ ) rowbeg[ i ] = 0;
  for( i = 0; i < nvals; i++ ) rowbeg[ colind[ i ] ]++;
  
  /* Convert the row counts to the starting positions */
  for( i = nrows; i > 0; i-- ) rowbeg[ i ] = rowbeg[ i - 1 ];
  for( rowbeg[ 0 ] = 0, i = 2; i <= nrows; i++ ) rowbeg[ i ] += rowbeg[ i-1 ];
  
  /* Insert the nonzero elements */
  for( i = 0; i < ncols; i++ )
    {
      for( j = colbeg[ i ]; j < colbeg[ i+1 ]; j++ )
	{
	  int row, rowfirst;
	  row = colind[ j ];
	  rowfirst = rowbeg[ row ];
	  rowind[ rowfirst ] = i;
	  rowval[ rowfirst ] = colval[ j ];
	  rowbeg[ row ]++;
        }
    }
  
  /* Shift back the starting positions */
  for( i = nrows - 1; i > 0; i-- ) rowbeg[ i ] = rowbeg[ i - 1 ];
  rowbeg[ 0 ] = 0;
  
  /* Find the row counts */
  for( i = 0; i < nrows; i++ )
    rowcnt[ i ] = rowbeg[ i + 1 ] - rowbeg[  i ];
}


void colmajor_to_rowmajor( int ncols,
			   const int colbeg[], const int colind[],
			   const double colval[],
			   int nrows, int rowbeg[], int rowind[],
			   double rowval[],
			   int rowcnt[], int nvals )
{
  int i, j;
  
  /* Count the number of nonzero elements per row */
  for( i = 0; i < nrows; i++ ) rowbeg[ i ] = 0;
  for( i = 0; i < nvals; i++ ) rowbeg[ colind[ i ] ]++;
  
  /* Convert the row counts to the starting positions */
  for( i = nrows; i > 0; i-- ) rowbeg[ i ] = rowbeg[ i - 1 ];
  for( rowbeg[ 0 ] = 0, i = 2; i <= nrows; i++ ) rowbeg[ i ] += rowbeg[ i-1 ];
  
  /* Insert the nonzero elements */
  for( i = 0; i < ncols; i++ )
    {
      for( j = colbeg[ i ]; j < colbeg[ i+1 ]; j++ )
	{
	  int row, rowfirst;
	  row = colind[ j ];
	  rowfirst = rowbeg[ row ];
	  rowind[ rowfirst ] = i;
	  rowval[ rowfirst ] = colval[ j ];
	  rowbeg[ row ]++;
        }
    }
  
  /* Shift back the starting positions */
  for( i = nrows - 1; i > 0; i-- ) rowbeg[ i ] = rowbeg[ i - 1 ];
  rowbeg[ 0 ] = 0;
  
  /* Find the row counts */
  for( i = 0; i < nrows; i++ )
    rowcnt[ i ] = rowbeg[ i + 1 ] - rowbeg[  i ];
}

void rowmajor_to_colmajor( int nrows,
    const int rowbeg[],
    const int rowind[], const double rowval[],
    int ncols, int colbeg[], int colind[],
    double colval[], int colcnt[],
    int nvals )
{
    colmajor_to_rowmajor( nrows, rowbeg, rowind, rowval,
			  ncols, colbeg, colind, colval,
			  colcnt, nvals );
}



//copy constructor
CProblemSpec::CProblemSpec( const CProblemSpec& form )
{
  objsen = form.objsen;
  
  int m = form.get_nrows();
  int n = form.get_ncols();
  int nz = form.get_nnzero();
  
  int* colind  = new int [nz];
  int* colbeg  = new int [n+1];
  int* colcnt  = new int [n];
  double* colval  = new double [nz];
  CColType *ctype = new CColType[n];
  double* obj = new double[n];
  double* lb = new double[n];
  double* ub = new double[n];
    
  int* rowind  = new int[nz];
  int* rowbeg  = new int [m+1];
  int* rowcnt  = new int [m];
  double* rowval  = new double [nz];

  CRowType *rtype = new CRowType[m];
  double* rhsx = new double[m];
  char* senx = new char[m];
  
  memcpy( colind, form.colmajor.get_matind(), nz*sizeof(int) );
  memcpy( colbeg, form.colmajor.get_matbeg(), (n+1)*sizeof(int) );
  memcpy( colcnt, form.colmajor.get_matcnt(), n*sizeof(int) );
  memcpy( colval, form.colmajor.get_matval(), nz*sizeof(double) );
  memcpy( ctype,  form.colmajor.get_ctype(),  n*sizeof(CColType) );
  memcpy( obj,    form.get_objx(),      n*sizeof(double) );
  memcpy( lb,     form.get_bdl(),       n*sizeof(double) );
  memcpy( ub,     form.get_bdu(),       n*sizeof(double) );
  
  memcpy( rowind, form.rowmajor.get_matind(), nz*sizeof(int) );
  memcpy( rowbeg, form.rowmajor.get_matbeg(), (m+1)*sizeof(int) );
  memcpy( rowcnt, form.rowmajor.get_matcnt(), m*sizeof(int) );
  memcpy( rowval, form.rowmajor.get_matval(), nz*sizeof(double) );
  memcpy( rtype,  form.rowmajor.get_rtype(),  m*sizeof(CRowType) );
  memcpy( rhsx,   form.get_rhsx(),      m*sizeof(double) );
  memcpy( senx,   form.get_senx(),      m*sizeof(char) );
  
  colmajor.init( n, m, nz, 0, 
		 colbeg, colcnt, colind, colval, 
		 ctype, obj, lb, ub );
  
  rowmajor.init( n, m, nz, 0, 
		 rowbeg, rowcnt, rowind, rowval, 
		 rtype, rhsx, senx );
  
  
  //setup the variable upper bounds
  vlb = new CVLB[form.get_ncols()];
  vub = new CVUB[form.get_ncols()];
  
  //can try to copy directly
  /*setupVarBounds(vlb,vub,form.get_ncols(),form.get_nrows(),
    form.rowmajor.get_matbeg(), form.rowmajor.get_matind(),
    form.rowmajor.get_matval(), form.rowmajor.get_matcnt(), 
    form.rowmajor.get_rtype(), form.colmajor.get_ctype());
  */
  for( int i=0; i <  form.get_ncols(); i++){
    vlb[i].set_var( (form.get_vlb())[i].get_var() );
    vlb[i].set_val( (form.get_vlb())[i].get_val() );
    vub[i].set_var( (form.get_vub())[i].get_var() );
    vub[i].set_val( (form.get_vub())[i].get_val() );
  }
  
  
  //initialize intConsIndex (for Chen's locking code)
  nInts = form.nInts;
  intConsIndex = new int [nInts];
  memcpy( intConsIndex, form.intConsIndex, nInts*sizeof(int) );

}




CProblemSpec::CProblemSpec( int objsense, int n, int m, int nz, int matbeg[],
			    int matind[], int matcnt[], double matval[], 
			    CColType ctype[], double obj[], double lb[],
			    double ub[], double rhsx[], char senx[] )
{
  int i;
  objsen = objsense;

  colmajor.init( n, m, nz, 0, matbeg, matcnt, matind, matval, ctype, obj, lb, ub );
  
  //convert column matrix to row matrix
  /*
  int* rowbeg = new int[m+1];
  int* rowcnt = new int[m];
  int* rowind = new int[nz];
  double* rowval = new double[nz];
  */

  int* rowind  = new int [nz];
  int* rowbeg  = new int [m+1];
  int* rowcnt  = new int [m];
  double* rowval  = new double [nz];

  colmajor_to_rowmajor(n,matbeg,matind,matval, m, rowbeg, 
		       rowind, rowval, rowcnt, nz );
  
  CRowType *rtype = new CRowType[m];
  
  
  rowmajor.init( n, m, nz, 0, rowbeg, rowcnt, rowind, rowval,
		 rtype, rhsx, senx );

  int nbc = 0;
  int ncc = 0;
  int nrr = 0;
  preprocess( &nbc, &ncc, &nrr, 1 );

  MWprintf( 10, "Preprocessing changed %d bounds, %d coefficients, and removed %d rows\n", nbc, ncc, nrr );
 
  // ### qun: classify here  
  for (i=0; i<get_nrows(); i++){
    rtype[i] = classify_one_row(rowmajor.get_matcnt()[i], 
				rowmajor.get_matind()+rowmajor.get_matbeg()[i],
				rowmajor.get_matval()+rowmajor.get_matbeg()[i], 
				rowmajor.get_senx()[i],
				rowmajor.get_rhsx()[i],
				colmajor.get_ctype());
  }
    
  //setup the variable upper bounds
  vlb = new CVLB[n];
  vub = new CVUB[n];
  
  setupVarBounds(vlb,vub,get_ncols(),get_nrows(),
		 rowmajor.get_matbeg(), rowmajor.get_matind(),
		 rowmajor.get_matval(), rowmajor.get_matcnt(), 
		 rowmajor.get_rtype(), colmajor.get_ctype());


  //initialize intConsIndex (for Chen's locking code)
  
  dArray<int> intIdx;
  
  for(i=0; i<n; i++){
    if(ctype[i] != COL_TYPE_CONTINUOUS ){
      intIdx.append(i);
    }
  }
  nInts = intIdx.size();
  intConsIndex = new int [nInts];
  for(i=0; i<nInts; i++){
    intConsIndex[i] = intIdx[i];
  }  

}


//assume all memory have be assigned, only need a couple
//of memory copies
//assume matrix size is the same as well
void CProblemSpec::copy( const CProblemSpec& form )
{
  int m = form.get_nrows();
  int n = form.get_ncols();
  int nz = form.get_nnzero();

  int *colind = colmajor.acc_matind();
  int *colbeg = colmajor.acc_matbeg();
  int *colcnt = colmajor.acc_matcnt();
  double *colval = colmajor.acc_matval();
  double* lb = acc_bdl();
  double* ub = acc_bdu();
  
  int *rowind = rowmajor.acc_matind();
  int *rowbeg = rowmajor.acc_matbeg();
  int *rowcnt = rowmajor.acc_matcnt();
  double *rowval = rowmajor.acc_matval();
  double * rhsx = rowmajor.acc_rhsx();
  
  memcpy( colind, form.colmajor.get_matind(), nz*sizeof(int) );
  memcpy( colbeg, form.colmajor.get_matbeg(), (n+1)*sizeof(int) );
  memcpy( colcnt, form.colmajor.get_matcnt(), n*sizeof(int) );
  memcpy( colval, form.colmajor.get_matval(), nz*sizeof(double) );
  //memcpy( ctype,  form.colmajor.get_ctype(),  n*sizeof(CColType) );
  //memcpy( obj,    form.get_objx(),      n*sizeof(double) );
  memcpy( lb,     form.get_bdl(),       n*sizeof(double) );
  memcpy( ub,     form.get_bdu(),       n*sizeof(double) );
  
  memcpy( rowind, form.rowmajor.get_matind(), nz*sizeof(int) );
  memcpy( rowbeg, form.rowmajor.get_matbeg(), (m+1)*sizeof(int) );
  memcpy( rowcnt, form.rowmajor.get_matcnt(), m*sizeof(int) );
  memcpy( rowval, form.rowmajor.get_matval(), nz*sizeof(double) );
  //memcpy( rtype,  form.rowmajor.get_rtype(),  m*sizeof(CRowType) );
  memcpy( rhsx,   form.get_rhsx(),      m*sizeof(double) );
  //memcpy( senx,   form.get_senx(),      m*sizeof(char) );
  
  return;
}



CProblemSpec::~CProblemSpec()
{
  if(vlb) delete [] vlb;
  if(vub) delete [] vub;
}



/****************************************************************************/
void CColMajorMatrix::init( int nc, int nr, int nv, int alloc_mem,
            int *mb, int *mc, int *mi, double *mv,
            CColType *ct, double *ob, double *l, double *u )
{
            if( alloc_mem )
            {
                if( !mb ) { mb = NEWV( int, nc+1 );
                    for( int i = 0; i < nc+1; i++ ) mb[i] = 0;
                }
                if( !mc ) { mc = NEWV( int, nc );
                    for( int i = 0; i < nc; i++ ) mc[i] = 0;
                }
                if( !mi ) { mi = NEWV( int, nv );
                    for( int i = 0; i < nv; i++ ) mi[i] = 0;
                }
                if( !mv ) { mv = NEWV( double, nv );
                    for( int i = 0; i < nv; i++ ) mv[i] = 0;
                }
                if( !ct ) { ct = NEWV( CColType, nc );
                    for( int i = 0; i < nc; i++ ) ct[i] = COL_TYPE_INVALID;
                }
                if( !ob ) { ob = NEWV( double, nc );
                    for( int i = 0; i < nc; i++ ) ob[i] = 0;
                }
                if( !l ) { l  = NEWV( double, nc );
                    for( int i = 0; i < nc; i++ ) l[i] = 0;
                }
                if( !u ) { u  = NEWV( double, nc );
                    for( int i = 0; i < nc; i++ ) u[i] = 0;
                }
            }
            init_matrix( nc, nr, nv, mb, mc, mi, mv );
            ctype = ct; objx = ob; bdl = l; bdu = u;
}

/****************************************************************************/
void CRowMajorMatrix::init( int nc, int nr, int nv, int alloc_mem,
            int *mb, int *mc, int *mi, double *mv,
            CRowType *rt, double *rh, char *sn )
{
            if( alloc_mem )
            {
                if( !mb ) { mb = NEWV( int, nr+1 );
                    for( int i = 0; i < nr+1; i++ ) mb[i] = 0;
                }
                if( !mc ) { mc = NEWV( int, nr );
                    for( int i = 0; i < nr; i++ ) mc[i] = 0;
                }
                if( !mi ) { mi = NEWV( int, nv );
                    for( int i = 0; i < nv; i++ ) mi[i] = 0;
                }
                if( !mv ) { mv = NEWV( double, nv );
                    for( int i = 0; i < nv; i++ ) mv[i] = 0;
                }
                if( !rt ) { rt = NEWV( CRowType, nr );
                    for( int i = 0; i < nr; i++ ) rt[i] = ROW_TYPE_INVALID;
                }
                if( !rh ) { rh = NEWV( double, nr );
                    for( int i = 0; i < nr; i++ ) rh[i] = 0;
                }
                if( !sn ) { sn = NEWV( char, nr );
                    for( int i = 0; i < nr; i++ ) sn[i] = 'L';
                }
            }
            init_matrix( nc, nr, nv, mb, mc, mi, mv );
            rtype = rt; rhsx = rh; senx = sn;
}



void CProblemSpec::Flip (int nzcnt, int *rind, double *rval, char *sense, double *rhs)
{
    register int j;

    *rhs = -(*rhs);
    *sense = (*sense == 'L') ? 'G' : 'L';
    for (j = 0; j < nzcnt; j++) {
        rval[j] = -rval[j];
    }
}

CRowType CProblemSpec::classify_one_row(int nzcnt, const int rind[], 
					const double rval[], 
					char sense, double rhs, 
					const CColType ctype[] )
{
    register int bpcnt,   /* binary positive count */
                 bncnt,   /* binary negative count */
                 ipcnt,   /* integer positive count */
                 incnt,   /* integer negative count */
                 cpcnt,   /* continuous positive count */
                 cncnt,   /* continuous negative count */
                 bcnt,    /* binary count */
                 icnt,    /* integer count */
                 ccnt,    /* continuous count */
                 pcnt,    /* positive count */
                 ncnt,    /* negative count */
                 i, j;
    double sum;
    CRowType rowclass = ROW_TYPE_UNCLASSIFIED;



    unsigned flip = FALSE;

    if (nzcnt == 0) 
      {
	return (ROW_TYPE_UNCLASSIFIED);
      }

    /*
     * Put >= constraint in <= form
     */

    if (sense == 'G') 
      {
        Flip (nzcnt, rind, rval, &sense, &rhs);
        flip = TRUE;
      }

/*
   We replace the set sign function by the lines of code that will in rsign.
    SetSign (nzcnt, rind, rval, sense, rhs);
*/
    
/*
   INITIAL COUNTER VALUES FOR THE CURRENT ROW
*/
                 
    bpcnt = bncnt = ipcnt = incnt = cpcnt = cncnt = 0;
    
/*
   COUNT THE ENTRIES IN THE CURRENT ROW
*/
                 
    for (j = 0; j < nzcnt; j++) 
      {
        if (rval[j] < -EPSILON) {
	  switch (ctype[rind[j]]) {
	  case COL_TYPE_BINARY:
	    bncnt++;
	    break;
	  case COL_TYPE_INTEGER:
	    incnt++;
	    break;
	  case COL_TYPE_CONTINUOUS:
	    cncnt++;
	    break;
	  }
        }
        else {
	  switch (ctype[rind[j]]) {
	  case COL_TYPE_BINARY:
	    bpcnt++;
	    break;
	  case COL_TYPE_INTEGER:
	    ipcnt++;
	    break;
	  case COL_TYPE_CONTINUOUS:
	    cpcnt++;
	    break;
	  }
        }
      }
    
    bcnt = bncnt + bpcnt;
    icnt = incnt + ipcnt;
    ccnt = cncnt + cpcnt;
    
    pcnt = cpcnt + ipcnt + bpcnt;
    ncnt = cncnt + incnt + bncnt;

/*
   CLASSIFY THE CONSTRAINT
*/
     
/*
   All variables are binary
*/

    if ((rowclass == ROW_TYPE_UNCLASSIFIED) && (bcnt == nzcnt)) {

/*
   Find out whether the inequality has nonzero coefficients
   that are all equal to either -1 or 1.
*/
         
      for (i = -1, j = 0; j < nzcnt; j++) {
	if (!(rval[j] < 1 + EPSILON && rval[j] > 1 - EPSILON) && 
	    !(rval[j] < -1 + EPSILON && rval[j] > -1 - EPSILON)) {
	  if (i < 0) {
	    i = j;
	  }
	  else {
	    break;
	  }
	}
      }

      if (j == nzcnt) {
	if (i < 0) {
	  if ((rhs < -bncnt + 1 + EPSILON) && (rhs > -bncnt + 1 - EPSILON)) {
	    if (sense == 'L') {
	      rowclass = ROW_TYPE_BINSUM1UB;
	    }
	    else {
	      rowclass = ROW_TYPE_BINSUM1EQ;
	    }
	  }
	  else if ((rhs < bpcnt - 1 + EPSILON) && (rhs > bpcnt - 1 - EPSILON)) {
	    if (sense == 'L') {
	      rowclass = ROW_TYPE_BINSUM1LB;
	    }
	    else {
	      rowclass = ROW_TYPE_BINSUM1EQ;
	    }
	  }
	  else {
	    if (sense == 'L') {
	      rowclass = ROW_TYPE_BINSUM1UBK;
	    }
	    else {
	      rowclass = ROW_TYPE_BINSUM1EQK;
	    }
	  }
	}
	else {
	  if ((rval[i] < -EPSILON) && (rhs < -bncnt + EPSILON) && (rhs > -bncnt - EPSILON)) {
	    if (sense == 'L') { 
	      rowclass = ROW_TYPE_BINSUM1VARUB;
	    }
	    else {
	      rowclass = ROW_TYPE_BINSUM1VAREQ;
	    }
	  }
	  else {
	    if (sense == 'L') {
	      rowclass = ROW_TYPE_ALLBINUB;
	    }
	    else {
	      rowclass = ROW_TYPE_ALLBINEQ;
	    }
	  }
	}
      }
      else {
	for (sum = (double) 0, j = 0; j < nzcnt; j++) {
	  if (rval[j] < -EPSILON) {
	    sum  += rval[j];
	  }
	}
	for (j = 0; j < nzcnt; j++) {
	  if (rval[j] < -EPSILON) {
	    if ((rhs < sum - rval[j] + EPSILON) &&
		(rhs > sum - rval[j] - EPSILON)) {
	      if (sense == 'L') {
		rowclass = ROW_TYPE_BINSUMVARUB;
	      }
	      else {
		rowclass = ROW_TYPE_BINSUMVAREQ;
	      }
	      break;
	    }
	  }
	}
	if (j == nzcnt) {
	  if (sense == 'L') {
	    rowclass = ROW_TYPE_ALLBINUB;
	  }
	  else {
	    rowclass = ROW_TYPE_ALLBINEQ;
	  }
	}
      }
    }
    
/*
   None of the variables is binary
*/

    if ((rowclass == ROW_TYPE_UNCLASSIFIED) && (bcnt == 0)) {
      if (sense == 'L') {
	rowclass = ROW_TYPE_NOBINUB;
      }
      else {
	rowclass = ROW_TYPE_NOBINEQ;
      }
    }

/*
   Binary as well as integer or continuous variables
*/
     
    if (rowclass == ROW_TYPE_UNCLASSIFIED) {
      if ((rhs < -EPSILON) || (rhs > EPSILON) || (bcnt != 1)) {
        if (sense == 'L') {
          rowclass = ROW_TYPE_MIXUB;
        }
        else {
          rowclass = ROW_TYPE_MIXEQ;
        }
      }
      else {
        if (nzcnt == 2) {
          if (sense == 'L') {
            if ((ncnt == 1) && (bncnt == 1)) {
              rowclass = ROW_TYPE_VARUB;
            }
            if ((pcnt == 1) && (bpcnt == 1)) {
              rowclass = ROW_TYPE_VARLB;
            }
          }
          else {
            rowclass = ROW_TYPE_VAREQ;
          }
        }
        else {
          if ((ncnt == 1) && (bncnt == 1)) {
            if (sense == 'L') {
              rowclass = ROW_TYPE_SUMVARUB;
            }
            else {
              rowclass = ROW_TYPE_SUMVAREQ;
            }
          }
        }
      }
    }
    
    if (rowclass == ROW_TYPE_UNCLASSIFIED) {
      if (sense == 'L') {
	rowclass = ROW_TYPE_MIXUB;
      }
      else {
	rowclass = ROW_TYPE_MIXUB;
      }
    }
    
    if (flip) {
      Flip (nzcnt, rind, rval, &sense, &rhs);
    }
    
    return (rowclass);
}





/****************************************************************************
 *
****************************************************************************/

void CProblemSpec::setupVarBounds(CVLB* vlb, CVUB* vub, int n, int m, 
		 const int row_matbeg[], 
		 const int row_matind[], const double row_matval[],
		 const int row_matcnt[], const CRowType rtype[],
		 const CColType ctype[] )
{
  int i, j;
  
  int rbeg;
  const int *rind;
  const double *rval;
  int xi, yi;
  double xv, yv;
  
  for ( j = 0; j < n; j++ ){
    vub[j].set_var( UNDEFINED );
    vlb[j].set_var( UNDEFINED );
  }
  
  for ( i = 0; i < m; i++ ){
    
    rbeg = row_matbeg[i];
    rind = row_matind + rbeg;
    rval = row_matval + rbeg;

    if ( rtype[i] == ROW_TYPE_VARUB )
      {
	if (ctype[rind[0]] == COL_TYPE_BINARY)
	  {
	    xi = rind[0];
	    xv = rval[0];
	    yi = rind[1];
	    yv = rval[1];
	  }
	else
	  {
	    yi = rind[0];
	    yv = rval[0];
	    xi = rind[1];
	    xv = rval[1];
	  }

	vub[yi].set_var ( xi );
	vub[yi].set_val ( -xv/yv );   
      }

    else if ( rtype[i] == ROW_TYPE_VAREQ )
      {
	
	if (ctype[rind[0]] == COL_TYPE_BINARY)
	  {
	    xi = rind[0];
	    xv = rval[0];
	    yi = rind[1];
	    yv = rval[1];
	  }
	else
	  {
	    yi = rind[0];
	    yv = rval[0];
	    xi = rind[1];
	    xv = rval[1];
	  }
	vlb[yi].set_var( xi );
	vub[yi].set_var( xi );
	vlb[yi].set_val( -xv/yv );     
	vub[yi].set_val( -xv/yv );              
      }
    else if ( rtype[i] == ROW_TYPE_VARLB )
      {
	if (ctype[rind[0]] == COL_TYPE_BINARY)
	  {
	    xi = rind[0];
	    xv = rval[0];
	    yi = rind[1];
	    yv = rval[1];
	  }
	else
	  {
	    yi = rind[0];
	    yv = rval[0];
	    xi = rind[1];
	    xv = rval[1];
	  }
	vlb[yi].set_var( xi );
	vlb[yi].set_val (-xv/yv );
	
      }
  }

  return;
}


void CProblemSpec::MWPack( MWRMComm *RMC )
{  
  RMC->pack(&objsen,1,1);
  colmajor.MWPack( RMC );
  rowmajor.MWPack( RMC );
  RMC->pack( &nInts, 1, 1 );
  RMC->pack( intConsIndex, nInts, 1 );
  return;
}

void CProblemSpec::MWUnpack( MWRMComm *RMC )
{  
  RMC->unpack(&objsen,1,1);

  colmajor.MWUnpack( RMC );
  rowmajor.MWUnpack( RMC );

  RMC->unpack(&nInts, 1, 1 );
  intConsIndex =  new int [nInts];
  RMC->unpack( intConsIndex, nInts, 1 );

  // Convert from column major to rowmajor.
  //  The memory for the row major matrix is already allocated.

  rowmajor.convert( get_colmajor() );

  // Set up the Variable upper bounds for flow cover generation...
  int n = get_ncols();
  int m = get_nrows();
  vlb = new CVLB[n];
  vub = new CVUB[n];
  
  setupVarBounds(vlb,vub,n,m,rowmajor.get_matbeg(),
		 rowmajor.get_matind(),
		 rowmajor.get_matval(),
		 rowmajor.get_matcnt(),
		 rowmajor.get_rtype(),
		 colmajor.get_ctype() );
    
  return;
}

void CColMajorMatrix::MWPack( MWRMComm *RMC )
{
  //pack matrix
  RMC->pack(&ncols,1,1);
  RMC->pack(&nrows,1,1);
  RMC->pack(&matvalsz,1,1);
  RMC->pack(matind,matvalsz,1);
  RMC->pack(matbeg,ncols+1,1);
  RMC->pack(matcnt,ncols,1);
  RMC->pack(matval,matvalsz,1);
  
  //pack bounds
  RMC->pack(bdl,ncols,1);
  RMC->pack(bdu,ncols,1);
  
  //pack obj coefficients
  RMC->pack(objx,ncols,1);

  //pack col type
  RMC->pack(ctype,ncols,1);
  
  return;
}

void CColMajorMatrix::MWUnpack( MWRMComm *RMC )
{
  //pack matrix
  RMC->unpack(&ncols,1,1);
  RMC->unpack(&nrows,1,1);
  RMC->unpack(&matvalsz,1,1);

  matind = new int[matvalsz];
  matbeg = new int [ncols+1];
  matcnt = new int [ncols];
  matval = new double [matvalsz];

  RMC->unpack(matind,matvalsz,1);
  RMC->unpack(matbeg,ncols+1,1);
  RMC->unpack(matcnt,ncols,1);
  RMC->unpack(matval,matvalsz,1);

  bdl = new double [ncols];
  bdu = new double [ncols];
  objx = new double [ncols];
  ctype = new CColType [ncols];

  RMC->unpack(bdl,ncols,1);
  RMC->unpack(bdu,ncols,1);
  RMC->unpack(objx,ncols,1);
  RMC->unpack(ctype,ncols,1);
  
  return;
}



void CRowMajorMatrix::MWPack( MWRMComm *RMC )
{

  RMC->pack( &nrows, 1, 1 );
  RMC->pack(rhsx,nrows,1);  
  RMC->pack(senx,nrows,1);
  RMC->pack( rtype, nrows, 1 );
  RMC->pack( &ncols, 1, 1 );
  RMC->pack( &matvalsz, 1, 1 );
 
  return;
}

void CRowMajorMatrix::MWUnpack( MWRMComm *RMC )
{

  RMC->unpack( &nrows, 1, 1 );

  rhsx = new double [nrows];
  senx = new char [nrows];
  rtype = new CRowType [nrows];

  //pack  right hand side
  RMC->unpack(rhsx,nrows,1);
  RMC->unpack(senx,nrows,1);
  RMC->unpack( rtype, nrows, 1 );


  // Allocate memory so that we can set up row major form
  RMC->unpack( &ncols, 1, 1 );
  RMC->unpack( &matvalsz, 1, 1 );
  matind  = new int [matvalsz];
  matbeg  = new int [nrows+1];
  matcnt  = new int [nrows];
  matval  = new double [matvalsz];
  
  return;
}

void CProblemSpec::write( FILE *fp )
{
  fprintf( fp, "%d\n", objsen );
  colmajor.write( fp );
  rowmajor.write( fp );
  fprintf( fp, "%d\n", nInts );
  for( int i = 0; i < nInts; i++ )
    fprintf( fp, "%d\n", intConsIndex[i] );
}

void CProblemSpec::read( FILE *fp )
{
  fscanf( fp, "%d", &objsen );
  colmajor.read( fp );
  rowmajor.read( fp );
  fscanf( fp, "%d", &nInts );
  intConsIndex = new int [nInts];
  for( int i = 0; i < nInts; i++ )
    fscanf( fp, "%d", &(intConsIndex[i]) );

  // Need to set up the variable upper bounds...
  //??? SHould I just write these out...

  int n = get_ncols();
  int m = get_nrows();
  vlb = new CVLB[n];
  vub = new CVUB[n];
  
  setupVarBounds(vlb,vub,n,m,rowmajor.get_matbeg(),
		 rowmajor.get_matind(),
		 rowmajor.get_matval(),
		 rowmajor.get_matcnt(),
		 rowmajor.get_rtype(),
		 colmajor.get_ctype() );

  return;
}  

void CColMajorMatrix::write( FILE *fp )
{
  fprintf( fp, "%d\n", ncols );
  fprintf( fp, "%d\n", nrows );
  fprintf( fp, "%d\n", matvalsz );

  for( int i = 0; i < matvalsz; i++ )
    fprintf( fp, "%d %lf\n", matind[i], matval[i] );

  for( int i = 0; i < ncols + 1; i++ )
    fprintf( fp, "%d\n", matbeg[i] );

  for( int i = 0; i < ncols; i++ )
    fprintf( fp, "%d %lf %lf %lf %d\n", matcnt[i], bdl[i], bdu[i], objx[i], 
	     ctype[i] );

  return;
}

void CColMajorMatrix::read( FILE *fp )
{
  fscanf( fp, "%d", &ncols );
  fscanf( fp, "%d", &nrows );
  fscanf( fp, "%d", &matvalsz );

  matind = new int [matvalsz];
  matbeg = new int [ncols + 1];
  matcnt = new int [ncols];
  matval = new double [matvalsz];
  
  for( int i = 0; i < matvalsz; i++ )
    fscanf( fp, "%d %lf", &(matind[i]), &(matval[i]) );

  for( int i = 0; i < ncols + 1; i++ )
    fscanf( fp, "%d", &(matbeg[i]) );

  bdl = new double [ncols];
  bdu = new double [ncols];
  objx = new double [ncols];
  ctype = new CColType [ncols];

  for( int i = 0; i < ncols; i++ )
    fscanf( fp, "%d %lf %lf %lf %d\n", &(matcnt[i]), &(bdl[i]), &(bdu[i]), 
	    &(objx[i]), &(ctype[i]) );

  return;
}


void CRowMajorMatrix::write( FILE *fp )
{
  fprintf( fp, "%d\n", ncols );
  fprintf( fp, "%d\n", nrows );
  fprintf( fp, "%d\n", matvalsz );

  for( int i = 0; i < matvalsz; i++ )
    fprintf( fp, "%d %lf\n", matind[i], matval[i] );

  for( int i = 0; i < nrows + 1; i++ )
    fprintf( fp, "%d\n", matbeg[i] );

  for( int i = 0; i < nrows; i++ )
    fprintf( fp, "%d", matcnt[i] );

  for( int i = 0; i < nrows; i++ )
    fprintf( fp, "%lf %c %d\n", rhsx[i], senx[i], rtype[i] );

  return;
}


void CRowMajorMatrix::read( FILE *fp )
{
  fscanf( fp, "%d", &ncols );
  fscanf( fp, "%d", &nrows );
  fscanf( fp, "%d", &matvalsz );

  matind = new int [matvalsz];
  matbeg = new int [nrows+1];
  matcnt = new int [nrows];
  matval = new double [matvalsz];
  
  for( int i = 0; i < matvalsz; i++ )
    fscanf( fp, "%d %lf", &(matind[i]), &(matval[i]) );

  for( int i = 0; i < nrows + 1; i++ )
    fscanf( fp, "%d", &(matbeg[i]) );

  for( int i = 0; i < nrows; i++ )
    fscanf( fp, "%d", &(matcnt[i]) );

  rhsx = new double [nrows];
  senx = new char [nrows];
  rtype = new CRowType [nrows];

  for( int i = 0; i < nrows; i++ )
    fscanf( fp, "%lf %c %d", &(rhsx[i]), &(senx[i]), &(rtype[i]) );


  return;
}



//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

/****************************************************************************/
void CColMajorMatrix::convert( const CRowMajorMatrix *r )
{
    if( get_ncols() < r->get_ncols() ||
	get_nrows() < r->get_nrows() ||
	get_matvalsz() < r->get_matvalsz() )
    {
	cerr << "Fatal: Cannot convert row to col major: size mismatch!"<<endl;
    }
    else
    {
        rowmajor_to_colmajor( r->get_nrows(),
	    r->get_matbeg(), r->get_matind(), r->get_matval(),
	    r->get_ncols(), matbeg, matind, matval, matcnt, r->get_matvalsz() );

        ncols = r->get_ncols();
        nrows = r->get_nrows();
	matvalsz = r->get_matvalsz();
    }
}

/****************************************************************************/
void CRowMajorMatrix::convert( const CColMajorMatrix *c )
{
    if( get_ncols() < c->get_ncols() ||
	get_nrows() < c->get_nrows() ||
	get_matvalsz() < c->get_matvalsz() )
    {
	cerr << "Fatal: Cannot convert col to row major: size mismatch!"<<endl;
    }
    else
    {
        colmajor_to_rowmajor( c->get_ncols(),
	    c->get_matbeg(), c->get_matind(), c->get_matval(),
	    c->get_nrows(), matbeg, matind, matval, matcnt, c->get_matvalsz() );

        ncols = c->get_ncols();
        nrows = c->get_nrows();
	matvalsz = c->get_matvalsz();
    }
}



/****************************************************************************/
ostream &operator<<( ostream &out, const CProblemSpec &p ) { return p >> out; }

/****************************************************************************/
ostream &CProblemSpec::operator>>( ostream &out ) const
{
    out << "#Vars = " << get_nvars()
	<< ", #Constraints = " << get_nrows() << endl;
    out << "Objective = " << (get_objsen()==-1 ? "MAXIMIZE":"MINIMIZE") << endl;
    out << *get_colmajor();
    out << *get_rowmajor();

    return out;
}

/****************************************************************************/
ostream &operator<<( ostream &out, const CColMajorMatrix &c ) {return c >> out;}

/****************************************************************************/
ostream &CColMajorMatrix::operator>>( ostream &out ) const
{
    PRINT( out, "OBJX", get_objx(), get_ncols(), 10 );
    PRINT( out, "BDL",  get_bdl(),  get_ncols(), 10 );
    PRINT( out, "BDU",  get_bdu(),  get_ncols(), 10 );

    out << "Matrix in column-major format:" << endl;
    PRINT( out, "MATBEG", get_matbeg(), get_ncols()+1, 15 );
    PRINT( out, "MATCNT", get_matcnt(), get_ncols(), 15 );
    PRINT( out, "MATIND", get_matind(), get_matvalsz(), 15 );
    PRINT( out, "MATVAL", get_matval(), get_matvalsz(), 10 );
    PRINT( out, "COLTYPES", get_ctype(), get_ncols(), 10 );
    return out;
}

/****************************************************************************/
ostream &operator<<( ostream &out, const CRowMajorMatrix &r ) {return r >> out;}

/****************************************************************************/
ostream &CRowMajorMatrix::operator>>( ostream &out ) const
{
    PRINT( out, "RHSX", get_rhsx(), get_nrows(), 10 );
    PRINT( out, "SENX", get_senx(), get_nrows(), 15 );

    out << "Matrix in row-major format:" << endl;
    PRINT( out, "MATBEG", get_matbeg(), get_nrows()+1, 15 );
    PRINT( out, "MATCNT", get_matcnt(), get_nrows(), 15 );
    PRINT( out, "MATIND", get_matind(), get_matvalsz(), 15 );
    PRINT( out, "MATVAL", get_matval(), get_matvalsz(), 10 );
    PRINT( out, "ROWTYPES", get_rtype(), get_nrows(), 10 );
    return out;
}





bool CProblemSpec::feasibleSolution( const double *xlp )
{
  for( int j = 0; j < nInts; j++ )
    {
      if( ! INTEGRAL( xlp[intConsIndex[j]] ) )
	return false;
    }

  return true;
}

//### qun
//for debuging preprocessing
void CProblemSpec::print()
{

  int i;
  
  char* colType[4]={
    "COL_TYPE_INVALID",
    "COL_TYPE_BINARY",
    "COL_TYPE_INTEGER",
    "COL_TYPE_CONTINUOUS"
  };

  char* rowType[22]={
    "ROW_TYPE_INVALID",
    "ROW_TYPE_MIXUB",
    "ROW_TYPE_MIXEQ",
    "ROW_TYPE_NOBINUB",
    "ROW_TYPE_NOBINEQ",
    "ROW_TYPE_ALLBINUB",
    "ROW_TYPE_ALLBINEQ",
    "ROW_TYPE_SUMVARUB",
    "ROW_TYPE_SUMVAREQ",
    "ROW_TYPE_VARUB",
    "ROW_TYPE_VAREQ",
    "ROW_TYPE_VARLB",
    "ROW_TYPE_BINSUMVARUB",
    "ROW_TYPE_BINSUMVAREQ",
    "ROW_TYPE_BINSUM1VARUB",
    "ROW_TYPE_BINSUM1VAREQ",
    "ROW_TYPE_BINSUM1UB",
    "ROW_TYPE_BINSUM1EQ",
    "ROW_TYPE_BINSUM1LB",
    "ROW_TYPE_BINSUM1UBK",
    "ROW_TYPE_BINSUM1EQK",
    "ROW_TYPE_UNCLASSIFIED"
  };
 
 
  int m = get_nrows();
  int n = get_ncols();
  int nz = get_nnzero();

  const int* col_matbeg = colmajor.get_matbeg();
  const int* col_matcnt = colmajor.get_matcnt();
  const int* col_matind = colmajor.get_matind();
  const double* col_matval = colmajor.get_matval();
 
  const CColType* ctype = colmajor.get_ctype();
  const double * objx = colmajor.get_objx();
  const double * bdl  = colmajor.get_bdl();
  const double * bdu  = colmajor.get_bdu();

  const int* row_matbeg = rowmajor.get_matbeg();
  const int* row_matcnt = rowmajor.get_matcnt();
  const int* row_matind = rowmajor.get_matind();
  const double* row_matval = rowmajor.get_matval();
 
  const CRowType * rtype = rowmajor.get_rtype();
  const double   * rhsx  = rowmajor.get_rhsx();
  const char *     senx  = rowmajor.get_senx();

  cout<<"########## start to print PRINO data ##########\n"<<endl;
  
  cout<<"m: "<<m<<"    n: "<<n<<"    nz: "<<nz<<"    sense: "<<objsen<<endl;
  
  cout<<"\n\ncolumnwise matrix: "<<endl;
  cout<<"matbeg: ";printIntVec(col_matbeg,n);
  cout<<"matcnt: ";printIntVec(col_matcnt,n);
  cout<<"matind: ";printIntVec(col_matind,nz);
  cout<<"matval: ";printDoubleVec(col_matval,nz);
  
  
  cout<<"ctype: \n";
  for( i=0; i<n; i++){
    cout<<i<<": "<<colType[ctype[i]-COL_TYPE_INVALID]<<endl;
  }
  
  cout<<"objx: ";printDoubleVec(objx,n);
  cout<<"bdl: "; printDoubleVec(bdl,n);
  cout<<"bdu: "; printDoubleVec(bdu,n);
  
  
  cout<<"\n\nrowwise matrix: "<<endl;
  cout<<"matbeg: ";printIntVec(row_matbeg,m);
  cout<<"matcnt: ";printIntVec(row_matcnt,m);
  cout<<"matind: ";printIntVec(row_matind,nz);
  cout<<"matval: ";printDoubleVec(row_matval,nz);
  
  
  cout<<"rtype: \n";
  for( i=0; i<m; i++){
    cout<<i<<": "<<rowType[rtype[i]-ROW_TYPE_INVALID]<<endl;
  }

  cout<<"rhsx: ";printDoubleVec(rhsx,m);
  cout<<"senx: ";printCharVec(senx,m);

  /*  
  cout<<"\n\nvariable bounds: "<<endl;
  cout<<"VLB: "<<endl;
  for( i=0; i<n; i++){
    cout<<i<<":  ";
    vlb[i].print();
  }
  cout<<"VUB: "<<endl;
  for( i=0; i<n; i++){
    cout<<i<<":  ";
    vub[i].print();
  }
  */
  cout<<"\n########## finish to print PRINO data ##########\n"<<endl<<endl;
  
  return;
}

void CProblemSpec::printLP()
{
  const double tol = 1e-6;
  

  int m = get_nrows();
  int n = get_ncols();
  //  int nz = get_nnzero();
  
  //  const int* col_matbeg = colmajor.get_matbeg();
  //  const int* col_matcnt = colmajor.get_matcnt();
  //  const int* col_matind = colmajor.get_matind();
  //  const double* col_matval = colmajor.get_matval();
  
  //  const CColType* ctype = colmajor.get_ctype();
  const double * objx = colmajor.get_objx();
  const double * bdl  = colmajor.get_bdl();
  const double * bdu  = colmajor.get_bdu();
  
  const int* row_matbeg = rowmajor.get_matbeg();
  const int* row_matcnt = rowmajor.get_matcnt();
  const int* row_matind = rowmajor.get_matind();
  const double* row_matval = rowmajor.get_matval();
 
  //  const CRowType * rtype = rowmajor.get_rtype();
  const double   * rhsx  = rowmajor.get_rhsx();
  const char *     senx  = rowmajor.get_senx();

  
  int i,j;
  
  cout<<"\n\nmin:  ";
  for (i=0; i<n; i++){
    if(0==i) cout<<objx[i]<<" x"<<i;
    else{
      if(objx[i]>tol){
	cout<<" + "<<objx[i]<<" x"<<i;
      }
      else{
	cout<<" - "<<fabs(objx[i])<<" x"<<i;
      }
    }
  }
  cout<<endl<<endl;
 
  const int *rind;
  const double *rval;
  int idx;
  double coef;

  for(i=0; i<m; i++){
    rind = row_matind + row_matbeg[i];
    rval = row_matval + row_matbeg[i];
    
    for( j=0; j< row_matcnt[i]; j++){
      idx = rind[j];
      coef = rval[j];

      if(0==j) cout<<"     "<<coef<<" x"<<idx;
      else{
	if(coef>tol){
	  cout<<" + "<<coef<<" x"<<idx;
	}
	else{
	  cout<<" - "<<fabs(coef)<<" x"<<idx;
	}
      }
    }
    
    if(senx[i]=='L'){
      cout<<" <= "<<rhsx[i]<<endl;
    }
    else if(senx[i]=='G'){
      cout<<" >= "<<rhsx[i]<<endl;
    }
    else{
      cout<<" = "<<rhsx[i]<<endl;
    }
  }
  
  cout<<endl<<endl;
 
  for(i=0; i<n; i++){
    cout<<"    "<<bdl[i]<<" <=x"<<i<<" <="<<bdu[i]<<endl;
  }
  cout<<"----------------------------------"<<endl;
  return;
}


void CProblemSpec::printIntVec(const int* a, int n)
{
  for(int i=0; i<n; i++){
    cout<<a[i]<<" ";
  }
  cout<<endl;
}

void CProblemSpec::printDoubleVec(const double* d, int n)
{
  for(int i=0; i<n; i++){
    cout<<d[i]<<" ";
  }
  cout<<endl;
}

void CProblemSpec::printCharVec(const char* s, int n)
{
  for(int i=0; i<n; i++){
    cout<<s[i]<<" ";
  }
  cout<<endl;
}

int CProblemSpec::get_nfvars()
{
  const double tol = 1e-6;
  
  const double* lb = get_bdl();
  const double* ub = get_bdu();

  int nfixed = 0;
  
  for(int i=0; i<get_nvars(); i++){
    if(fabs(ub[i]-lb[i])<tol){
      nfixed++;
    }
  }
  
  return nfixed;
}


//compact row major matrix
void CRowMajorMatrix::compact()
{
  int i;  //row index
  int j;  //col index
  int rowptr = 0, nnz = 0;
  
  int m = get_nrows();
  int beg, cnt;

  for(i = 0; i<m; i++){
    beg = get_matbeg()[i];
    cnt = get_matcnt()[i];

    if(cnt > 0 ){
      for(j=beg; j <beg + cnt; j++ ){
	matind[nnz] = matind[j];
	matval[nnz] = matval[j];
	nnz++;
      }
	  
      matbeg[rowptr] = nnz - cnt;
      matcnt[rowptr] = cnt;
      rhsx[rowptr]   = rhsx[i];
      senx[rowptr]   = senx[i];
      rowptr++;
    }
  }
  matbeg[rowptr] = nnz;
  nrows = rowptr ;
  matvalsz = nnz;
}


void CRowMajorMatrix::Flip (int nzcnt, 
			    int *rind, double *rval, 
			    char *sense, double *rhs)
{
  register int j;
  
  *rhs = -(*rhs);
  *sense = (*sense == 'L') ? 'G' : 'L';
  for (j = 0; j < nzcnt; j++) {
    rval[j] = -rval[j];
  }
}
