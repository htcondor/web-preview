//We implemented the simple preprocessing on MIP described in
//M. W. P. Savelsbergh.
//Preprocessing and Probing for Mixed Integer Programming Problems. 
//{\em ORSA J. on Computing}, 6: 445--454, 1994.

//We process row by row and do the following for each row
//1. remove empty row
//2. remove singleton row
//3. check infeasibility
//4. check redaudancy
//5. coefficient modification
//6. bounds modification

//we repeat above process until there is no further modification



#define max(a,b) (a<b)?b:a;
#define min(a,b) (a<b)?a:b;

#include "global.hh"
#include "PRINO.hh"
#include <MW.h>

double ffloor(double a)
{
  a += tol;
  
  //double tmp = (double) (a>=0) ? (int)(a) : (int)(a) - 1;
  //cout<<"!!!floor "<< a<<" "<<floor(a)<<" "<<tmp<<endl;
  
  //return floor(a);
  return (double) (a>=0) ? (int)(a) : (int)(a) - 1;
}
double fceil(double a)
{
  a -= tol;
  //double tmp = (double) (a>0) ? (int)(a) + 1 : (int)(a);
  //cout<<"!!!ceil "<<a<<" "<<ceil(a)<<" "<<tmp<<endl;
  //return ceil(a);
  return (double) (a>0) ? (int)(a) + 1 : (int)(a);
}



void print_one_row(int nnz, int* ridx, double* rval, 
		   char rsen, double rrhs)
{
  int idx;
  double coef;
  
  for(int j=0; j< nnz; j++){
    idx = ridx[j];
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
  
  if(rsen=='L'){
    cout<<" <= "<<rrhs<<endl;
  }
  else if(rsen=='G'){
    cout<<" >= "<<rrhs<<endl;
  }
  else{
    cout<<" = "<<rrhs<<endl;
  }
  
  cout<<endl;
}


//st = 0: OK, 1: infeasible, 2: unbounded
//if preprocess root node, we hope reduce both 
//number of rows and number of cols
//but for node preprocessing, we will keep the
//matrix size unchanged

//XXX Not a nice way of doing this, but it's quick and dirty.
static int nBoundChanges = 0;
static int nCoefChanges = 0;
static int nRowsRemoved = 0;

int CProblemSpec::preprocess( int *nbc, int *ncc, int *nrr, int root )
{

#ifdef   DEBUGPS
  cout<<"#### before process the model"<<endl;
  printLP();
#endif

  int nRows = get_nrows();
  int nfixvars = get_nfvars();
#if 0
  cout<<"before preprocessing:"<<endl;
  cout<<"# of rows: "<<get_nrows()<<"  # of cols: "<<get_ncols()
      <<"  # of fixed vars "<<get_nfvars()<<endl;
#endif
  
  

  //process row by row, also might change varaible bounds
  int st = rowmajor.preprocess(colmajor.get_ctype(),
			       colmajor.acc_bdl(), 
			       colmajor.acc_bdu(),
			       root);
  
  //update column major matrix
  colmajor.convert(&rowmajor);
  
  
#ifdef   DEBUGPS
  cout<<"#### after process the model"<<endl;
  printLP();
#endif

  nRowsRemoved = nRows-get_nrows();
#if 0
  cout<<"after preprocessing:"<<endl;
  cout<<"# of rows: "<<get_nrows()<<"  # of cols: "<<get_ncols()
      <<"  # of fixed vars "<<get_nfvars()<<endl;
#endif
  

  *nbc = nBoundChanges;
  *ncc = nCoefChanges;
  *nrr = nRowsRemoved;

  MWprintf( 30, "Preprocessing removed %d rows, fixed %d vars, improved %d coefficients\n", 
	    nRowsRemoved,
	    get_nfvars()-nfixvars,
	    nCoefChanges );

  return st;
}



//process rows for multi runs until there is no changes
//st = 0: OK, 1: infeasible, 2: unbounded
int CRowMajorMatrix::preprocess(const CColType ctype[],
				double bdl[], double bdu[],
				int root)
{
  int st = 0; //value to be returned
  
  int i;
  ROWSTATE rst;
  
  bool doPsolve = true; //flag indicating whether do one more run psolve
  
#ifdef   DEBUGPS 
  int nRuns = 0;
#endif
 

  
  //bdl and bdu also might be changed
  

  ROWSTATE* rows = new ROWSTATE[get_nrows()];
  for(i = 0; i<get_nrows(); i++){
    rows[i] = UNCHANGED;
  }
  
  while(doPsolve){
    

#ifdef   DEBUGPS 
    cout<<"do run "<<++nRuns<<"&&&&"<<endl;
#endif

    doPsolve = false;
    
    
    //process each row
    for(i=0; i<get_nrows(); i++){
      
#ifdef   DEBUGPS 
      cout<<"process row: "<<i<<"####"<<endl;
#endif

      if(rows[i] == REDUNDANT){
#ifdef   DEBUGPS 
	cout<<"row "<<i<<" is redaundant"<<endl;
#endif
	continue;
      }
      
      
      rst = process_one_row(root,
			    i,
			    matcnt[i],senx[i],rhsx[i],
			    matind+get_matbeg()[i],matval+get_matbeg()[i],
			    ctype, bdl, bdu);
 
      rows[i] = rst;
      
      if(rst == INFEASIBLE){
#ifdef   DEBUGPS 
	cout<<"row "<<i<<" is infeasible."<<endl;
#endif
	st = 1; //mip infeasible
	break;
      }
      else if(rst == REDUNDANT){
#ifdef   DEBUGPS 
	cout<<"row "<<i<<" is redaundant."<<endl;
#endif
	if( root ) matcnt[i] = 0; //remove this row
      }
      else if(rst == CHANGED){
#ifdef   DEBUGPS 
	cout<<"row "<<i<<" changed."<<endl;
#endif
	doPsolve = true;
      }
      else{//unchanged
	//do nothing, just process the next row
#ifdef   DEBUGPS 
	cout<<"row "<<i<<" remains the same."<<endl;
#endif
      }
      
    } //end one run
    
  } //until no change
  
  
  //compact the row major matrix by deleting the rows with cnt=0
  if( root )  compact();
  
  delete [] rows;
  
  return st;
  
}



//process one row1
ROWSTATE CRowMajorMatrix::process_one_row(int root,
					  const int& r,
					  int& row_cnt,  char row_sense, 
					  double& row_rhsx,
					  int* row_ind, double* row_val,
					  const CColType* ctype, 
					  double* bdl, double* bdu)
{  
#ifdef   DEBUGPS 
  cout<<"Before processing, the row looks like: "<<endl;
  print_one_row(row_cnt, row_ind, row_val, row_sense, row_rhsx);
#endif
  
  ROWSTATE st = UNCHANGED;
  int i;
  
  if( root ){
    
    //we first remove the fixed vars from the row
    int nnz = 0;
    for(i=0; i<row_cnt; i++){
      if(fabs(bdu[row_ind[i]] - bdl[row_ind[i]]) < tol){ //fixed var
	row_rhsx -= row_val[i] * bdl[row_ind[i]];
      }
      else{
	row_ind[nnz] = row_ind[i];
	row_val[nnz] = row_val[i];
	nnz++;
      }
    }
    
#ifdef   DEBUGPS 
    cout<<"Removed "<<row_cnt - nnz<<" fixed vars from row "<<r<<endl;
    cout<<"After compacting, the row looks like: "<<endl;
    print_one_row(nnz, row_ind, row_val, row_sense, row_rhsx);
#endif
    
    row_cnt = nnz;
  }
  
  
 
  //we tehn change the row to the sense "L", so we only need to consider
  //"L" and "E" two cases
  bool flip = false;
  if(row_sense == 'G'){
#ifdef   DEBUGPS 
    cout<<"Flip the row sign."<<endl;
#endif
    Flip(row_cnt, row_ind, row_val, &row_sense, &row_rhsx);
    flip = TRUE;
  }
  
  
  double coef, newb;
  int idx;
  
  double Lmin = 0, Lmax = 0; //minimum and maximum possible values
  
  for(i = 0; i < row_cnt; i++) {
    if (row_val[i] > tol) {
      Lmin += row_val[i] * bdl[row_ind[i]];
      Lmax += row_val[i] * bdu[row_ind[i]];
    }
    else {
      Lmin += row_val[i] * bdu[row_ind[i]];
      Lmax += row_val[i] * bdl[row_ind[i]];
    }
  }
  
#ifdef   DEBUGPS
  cout<<"The max and min possible values are: "<<Lmax<<" "<<Lmin<<endl;
  cout<<"The right hand side value is: "<<row_rhsx<<endl;
#endif
  
  if( root ) {
    
    /*1. remove empty row ************************************/
    if (row_cnt == 0) {
      
#ifdef   DEBUGPS   
      cout<<"An empty row is found"<<endl;
#endif
      
      if (row_rhsx < -tol || (row_rhsx > tol && row_sense == 'E')) {
#ifdef   DEBUGPS
	cout<<"empty row is infeasible"<<endl;
#endif
	st = INFEASIBLE;
      }
      else {
#ifdef   DEBUGPS
	cout<<"empty row is redaundant"<<endl;
#endif
	st = REDUNDANT;
      }
      
      if (flip) {
	Flip(row_cnt, row_ind, row_val, &row_sense, &row_rhsx);
      }
      
      return st;
      
    }
  }
  

  
  
  /*2. remove singleton row ************************************/

  if( root ){
  if(row_cnt == 1){
    
#ifdef   DEBUGPS
    cout<<"A singleton row is found."<<endl;
#endif
    
    idx = row_ind[0];
    coef = row_val[0];
    
    if( fabs( coef ) < tol ){ //coef = 0
      
      if( row_sense == 'E' ){
	if( fabs ( row_rhsx ) >= tol ){
	  st = INFEASIBLE;
	}
	else{
	  st = REDUNDANT;
	}
      }
      else{ //'L'
	if( row_rhsx < -tol ){
	  st = INFEASIBLE;
	}
	else{
	  st = REDUNDANT;
	}
      }
      
      //flip back
      if (flip) {
	Flip(row_cnt, row_ind, row_val, &row_sense, &row_rhsx);
      }
      
      return st;
    }
    
      
    newb = row_rhsx/coef;
    
    //change idx's bounds
    if(row_sense == 'E'){ //"=" change two bounds
      
      if( ctype[idx] != COL_TYPE_CONTINUOUS){ //integer
	
	if(ffloor(newb) < bdu[idx] - tol){ //change upper bound
#ifdef   DEBUGPS
	  cout<<"int varaible "<<idx<<" 's upper bound is changed from "<<
	    bdu[idx]<<" to "<<ffloor(newb)<<endl;
#endif
	  bdu[idx] = ffloor(newb);
	  st = CHANGED;
	  nBoundChanges++;
	}
	
	if( fceil(newb) > bdl[idx] + tol){ //change upper bound
#ifdef   DEBUGPS
	  cout<<"int varaible "<<idx<<" 's lower bound is changed from "<<
	    bdl[idx]<<" to "<<fceil(newb)<<endl;
#endif
	  bdl[idx] = fceil(newb);
	  st = CHANGED;
	  nBoundChanges++;
	}

	if( newb > bdu[idx] + tol || newb < bdl[idx] - tol){
#ifdef   DEBUGPS
	  cout<<"int varaible "<<idx<<" 's fixed bound "<<newb
	      <<" is not within "<< bdl[idx] <<" and "<< bdu[idx]<<endl;
#endif
	  st = INFEASIBLE;
	  cout<<"found infeasibility when removing singleton row."<<endl;
	}
      }
      
      else{ //continuous
	if( newb < bdu[idx] - tol){ //change upper bound
#ifdef   DEBUGPS
	  cout<<"varaible "<<idx<<" 's upper bound is changed from "<<
	    bdu[idx]<<" to "<<newb<<endl;
#endif
	  bdu[idx] = newb;
	  st = CHANGED;
	  nBoundChanges++;
	}
	if( newb > bdl[idx] + tol){ //change upper bound
#ifdef   DEBUGPS
	  cout<<"varaible "<<idx<<" 's lower bound is changed from "<<
	    bdl[idx]<<" to "<<newb<<endl;
#endif
	  bdl[idx] = newb;
	  st = CHANGED;
	  nBoundChanges++;
	}
	if( newb > bdu[idx] + tol || newb < bdl[idx] - tol){
#ifdef   DEBUGPS
	  cout<<"varaible "<<idx<<" 's fixed bound "<<newb
	      <<" is not within "<< bdl[idx] <<" and "<< bdu[idx]<<endl;
#endif
	  st = INFEASIBLE;
	  cout<<"found infeasibility when removing singleton row."<<endl;
	}
      }
    }
      
      
    else if( coef>tol ){//"<=", change bdu
      
      if( ctype[idx] != COL_TYPE_CONTINUOUS){ //integer
	
	if(ffloor(newb) < bdu[idx] - tol){
#ifdef   DEBUGPS
	  cout<<"int varaible "<<idx<<" 's upper bound is changed from "<<
	    bdu[idx]<<" to "<<ffloor(newb)<<endl;
#endif  
	  bdu[idx] = ffloor(newb);
	  st = CHANGED;
	  nBoundChanges++;
	}
      }
      
      
      else{//continuous
	if(newb < bdu[idx] - tol){
#ifdef   DEBUGPS
	  cout<<"varaible "<<idx<<" 's upper bound is changed from "<<
	    bdu[idx]<<" to "<<newb<<endl;
#endif  
	  bdu[idx] = newb;
	  
	  bdu[idx] = newb;
	  st = CHANGED;
	  nBoundChanges++;
	}
      }
    }
        
    
    else{// "<="change bdl
      
      if( ctype[idx] != COL_TYPE_CONTINUOUS){ //integer
	if(fceil(newb) >  bdl[idx] + tol){
#ifdef   DEBUGPS
	  cout<<"int varaible "<<idx<<" 's lower bound is changed from "<<
	    bdl[idx]<<" to "<<fceil(newb)<<endl;
#endif
	  bdl[idx] = fceil(newb) ;
	  st = CHANGED;
	  nBoundChanges++;
	}
      }
      
      else{//continuous
	if(newb >  bdl[idx] + tol){
#ifdef   DEBUGPS
	  cout<<"varaible "<<idx<<" 's lower bound is changed from "<<
	    bdl[idx]<<" to "<<newb<<endl;
#endif
	  bdl[idx] = newb;
	  st = CHANGED;
	  nBoundChanges++;
	}
      }  
    }
    
    
    if(st != INFEASIBLE){ //check new bounds
      if( bdu[idx] + tol < bdl[idx] ){
	st = INFEASIBLE;
#ifdef   DEBUGPS
	cout<<"contradict bounds on "<<idx<<" "
	    <<bdl[idx]<<" > "<<bdu[idx]<<endl;
#endif
      }
    }
    
    //flip back
    if (flip) {
      Flip(row_cnt, row_ind, row_val, &row_sense, &row_rhsx);
    }
    
    //remove this row
    if( root ) matcnt[r] = 0;
    
    return st;
  }
  }
  
  
  
  
  
  /*3-4. Identification infeasibility and redundancy ***************/
  
  if( root ){
    
    if( row_sense == 'L' ){
      if(Lmin > row_rhsx){ //infeasibility
	st = INFEASIBLE;
      }
      if(Lmax < row_rhsx){ //reduandant
	st = REDUNDANT;
      }
    }
    else{ //'='
      if( Lmin > row_rhsx || Lmax < row_rhsx ){
	st = INFEASIBLE;
      }
    }
    
    if(st == INFEASIBLE || st == REDUNDANT){
      //flip back
      if (flip) {
	Flip(row_cnt, row_ind, row_val, &row_sense, &row_rhsx);
      }
      return st;
    }  
  }
  
  
    
  /*5. Coefficient Modification ***************/

  //XXX Should we only do this if we are at the root, since 
  //  it may screw up the SOPLEX solver?

  //XXXif( root ) {
  double dmax=0, dmin=0;
  double diff;
  int ncochanges = 0;
  
  if (Lmax > row_rhsx + tol) {
    for (i = 0; i < row_cnt; i++) {
      //do only this is not a fixed vars or infeasible var
      if (bdl[ row_ind[i] ] < bdu[ row_ind[i] ] - tol) { 
	if (ctype[row_ind[i]] == COL_TYPE_BINARY) {
	  if (row_val[i] > tol) {
	    if ((diff = row_rhsx - Lmax + row_val[i]) > tol) {
	      row_val[i] -= diff;
	      ncochanges++;
	      dmax += diff;
	    }
	  }
	  else {
	    if ((diff = row_rhsx - Lmax - row_val[i]) > tol) {
	      row_val[i] += diff;
	      ncochanges++;
	      dmin += diff;
	    }
	  }
	}
      }
    }
    
    row_rhsx -= dmax;
    Lmax -= dmax;
    Lmin += dmin;

    nCoefChanges += ncochanges;

    if(dmin > tol || dmax > tol){
      
#ifdef   DEBUGPS 
      cout<<"coefs are modified: "<<endl;
      print_one_row(row_cnt, row_ind, row_val, row_sense, row_rhsx);
#endif
      //cout<<ncochanges<<" coefs are modified."<<endl;
      
      //we do not change status to CHANGED here !
    } 
  }
  
  //XXX}
  
  
  
  
  
  
  /*6. Improving bounds *************************************/
  
  
  //fixing binary variables
  for (i = 0; i < row_cnt; i++) {
    idx = row_ind[i];
    coef = row_val[i];
    
    //do only this is not a fixed vars or infeasible var
    if (bdl[idx] < bdu[idx] - tol) { 
      
      if (ctype[idx] == COL_TYPE_BINARY) {
	if (coef > tol) {
	  if (Lmin + coef > row_rhsx + tol) {
#ifdef   DEBUGPS
	    cout<<"bin varaible "<<idx<<" is fixed at its lower bound 0"<<endl;
#endif
	    bdu[idx] = 0;  //fixed at lower bound
	    st = CHANGED;
	    nBoundChanges++;
	  }
	}
	else {
	  if (Lmin - coef > row_rhsx + tol){   
#ifdef   DEBUGPS
	    cout<<"bin varaible "<<idx<<" is fixed at its upper bound 1"<<endl;
#endif
	    bdl[idx] = 1;
	    st = CHANGED;
	    nBoundChanges++;
	  }
	}
      }
    }
  }
  
  
  //if(root){
  
  //modify bounds
  if (Lmin > -FATCOPsinf) { //if minimum possible value is not -inf
    
    for (i = 0; i < row_cnt; i++) {
      
      idx = row_ind[i];
      coef = row_val[i];
      
      if( fabs(coef) < tol ) continue;

      if (bdl[idx] < bdu[idx] - tol) {
	
	if (coef > tol) {
	  newb = ((row_rhsx - Lmin)/coef) + bdl[idx];
	  
	  if (ctype[idx] == COL_TYPE_CONTINUOUS) {
	    if (newb < bdu[idx] - tol) {
#ifdef   DEBUGPS
	      cout<<"varaible "<<idx<<" 's upper bound is changed from "<<
		bdu[idx]<<" to "<<newb<<endl;
#endif
	      bdu[idx] = newb;
	      st = CHANGED;
	      nBoundChanges++;
	    }
	  }
	  
	  else { // integer, need rounding
	    if( ffloor(newb) < bdu[idx] - tol) {
#ifdef   DEBUGPS
	      cout<<"int varaible "<<idx<<" 's upper bound is changed from "<<
		bdu[idx]<<" to "<<ffloor(newb)<<endl;
#endif
	      bdu[idx] = ffloor(newb);
	      st = CHANGED;
	      nBoundChanges++;
	    }
	  }
	}          
	
	else { //coef negative
	  newb = ((row_rhsx - Lmin) / coef) + bdu[idx];
	  
	
	  if (ctype[row_ind[i]] == COL_TYPE_CONTINUOUS) {
	    if (newb > bdl[idx] + tol) {
#ifdef   DEBUGPS
	      cout<<"varaible "<<idx<<" 's lower bound is changed from "<<
		bdl[idx]<<" to "<<newb<<endl;
#endif
	      bdl[idx] = newb;
	      st = CHANGED;
	      nBoundChanges++;
	    }
	  }     
	  else { //integer var
	    if ( fceil(newb) > bdl[idx] + tol) {
#ifdef   DEBUGPS
	      cout<<"int varaible "<<idx<<" 's lower bound is changed from "<<
		bdl[idx]<<" to "<<fceil(newb)<<endl;
#endif     
	      bdl[idx] = fceil(newb);
	      st = CHANGED;
	      nBoundChanges++;
	    }
	  }  
	}
      }
    }
  }
  
  
  if (row_sense == 'E') { //need to consider Lmax
    if (Lmax < FATCOPsinf) { //the maximum possible value is not inf
      for (i = 0; i < row_cnt; i++) {
	
	idx = row_ind[i];
	coef = row_val[i];

	if( fabs(coef) < tol ) continue;
	
	if (bdl[idx] < bdu[idx] - tol) {
	  if (coef > tol) { //positive coef
	    newb = (row_rhsx - Lmax + coef*bdu[idx])/coef;
	
	
	    if (ctype[idx] == COL_TYPE_CONTINUOUS) {
	      if (newb > bdl[idx] + tol) {
#ifdef   DEBUGPS
		cout<<"varaible "<<idx<<" 's lower bound is changed from "<<
		  bdl[idx]<<" to "<<newb<<endl;
#endif
		bdl[idx] = newb;
		st = CHANGED;
		nBoundChanges++;
	      }
	    }
	    else { //integer, need rounding
	      if ( fceil(newb) > bdl[idx] + tol) {
#ifdef   DEBUGPS
		cout<<"int varaible "<<idx<<" 's lower bound is changed from "<<
		  bdl[idx]<<" to "<<fceil(newb)<<endl;
#endif
		bdl[idx] = fceil(newb);
		st = CHANGED;
		nBoundChanges++;
	      }
	    }
	  }
	  
	  else { //coef < 0 
	    newb = (row_rhsx - Lmax + coef*bdl[idx])/coef;
	    if (ctype[idx] == COL_TYPE_CONTINUOUS) {
	      if (newb < bdu[idx] - tol) {
#ifdef   DEBUGPS
		cout<<"varaible "<<idx<<" 's upper bound is changed from "<<
		  bdu[idx]<<" to "<<newb<<endl;
#endif
		bdu[idx] = newb;
		st = CHANGED;
		nBoundChanges++;
	      }
	    }
	    else { //integer
	      if (ffloor(newb) < bdu[idx] - tol) {
#ifdef   DEBUGPS
		cout<<"int varaible "<<idx<<" 's upper bound is changed from "<<
		  bdu[idx]<<" to "<<ffloor(newb)<<endl;
#endif
		bdu[idx] = ffloor(newb);
		st = CHANGED;
		nBoundChanges++;
	      }
	    }
	  }
	}
      }
    }
  }
  
  //  }
  
  //flip back
  if (flip) {
    Flip(row_cnt, row_ind, row_val, &row_sense, &row_rhsx);
  }
  
  
#ifdef   DEBUGPS 
  cout<<"After processing, the row looks like: "<<endl;
  print_one_row(row_cnt, row_ind, row_val, row_sense, row_rhsx);
#endif
  
  
  return st;
  
}

