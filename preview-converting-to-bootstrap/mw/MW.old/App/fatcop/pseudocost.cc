#include <math.h>
#include <assert.h>
#include <MW.h>
#include <MWRMComm.h>
#include "pseudocost.hh"
#include <lpsolver.h>

#define mmin(a,b) (a<b)?a:b;
#define mmax(a,b) (a>b)?a:b;


const double ratio_of_weaken_obj = 1;


//------------------------------------------------------------------
//               constructors and destructor
//------------------------------------------------------------------
PseudoCostManager::PseudoCostManager(CProblemSpec* form,
				     CLPSolverInterface *lps,
				     int init_strategy )
{
  way_of_ini = init_strategy; //1: solve LP explicitly
                   //0: use objective coefficient
  formulation = form;
  
  int ncols = formulation->get_ncols();

  // We trade time for speed here
  workingPCostArray = new pCost[ncols];
  newPCostArray = new pCost[ncols];
  
  for( int i = 0; i < ncols; i++)
    {
      //initilize the pseudocost to the coefficient in the object function
      //double cu = fabs(lp->obj(idx)) + ((*priority)[i])*coef1;
      double cu = fabs((formulation->get_objx())[i])*ratio_of_weaken_obj;
      double cd = cu;
      pCost temp( i, cu, cd, 0, 0);
      workingPCostArray[i] = temp;
      pCost temp2( i, 0.0, 0.0, 0, 0 );
      newPCostArray[i] = temp2;
    }

  lpsolver = lps;
  itlim = INT_MAX;
  way_of_ini = 1;
}

PseudoCostManager::PseudoCostManager( CProblemSpec* form )
{
  // Right now we don't use this.

  //  We could consider a strategy where the first "tasks"
  //  are to compute a number of the unknown (but necessary) pseudocosts.

  way_of_ini = -1;
  formulation = form;
  
  int ncols = formulation->get_ncols();

  // We trade time for speed here
  workingPCostArray = new pCost[ncols];
  newPCostArray = new pCost[ncols];
  
  for( int i = 0; i < ncols; i++)
    {
      //initilize the pseudocost to the coefficient in the object function
      //double cu = fabs(lp->obj(idx)) + ((*priority)[i])*coef1;
      double cu = fabs((formulation->get_objx())[i])*ratio_of_weaken_obj;
      double cd = cu;
      pCost temp( i, cu, cd, 0, 0);
      workingPCostArray[i] = temp;
      pCost temp2( i, 0.0, 0.0, 0, 0 );
      newPCostArray[i] = temp2;
    }

  itlim = INT_MAX;
  way_of_ini = 1;

}


PseudoCostManager::~PseudoCostManager()
{
  if(workingPCostArray) delete[] workingPCostArray;
  if(newPCostArray) delete [] newPCostArray;  
}
  
//------------------------------------------------------------------
//               methods
//------------------------------------------------------------------
void PseudoCostManager::updateFromLP( const int& branchingIdx, const double& oldxlpval,
				      const double& parentLpVal,const double& lpVal,
				      const char& whichBound )
{
  int idx = branchingIdx;
  assert( idx >= 0 );

  int n;
  //cout<<"##########"<<f<<endl;

  double f = oldxlpval - (int) oldxlpval;
      
  if( whichBound == 'U' ){//up

    double newPcost = (lpVal-parentLpVal)/f;

    n = workingPCostArray[idx].nBranUp;
    workingPCostArray[idx].costUp =  (n*(workingPCostArray[idx]).costUp
				      +newPcost)/(n+1);
    workingPCostArray[idx].nBranUp = n+1;

    n = newPCostArray[idx].nBranUp;
    newPCostArray[idx].costUp = (n*(newPCostArray[idx]).costUp
				 +newPcost)/(n+1);
    newPCostArray[idx].nBranUp = n+1;
    
  }
  else{
    double newPcost = (lpVal-parentLpVal)/(1.0-f);
    n = workingPCostArray[idx].nBranDown;
    workingPCostArray[idx].costDown =  (n*(workingPCostArray[idx]).costDown
					+newPcost)/(n+1);
    
    workingPCostArray[idx].nBranDown = n+1;

    n = newPCostArray[idx].nBranDown;
    newPCostArray[idx].costDown = (n*(newPCostArray[idx]).costDown
				 +newPcost)/(n+1);
    newPCostArray[idx].nBranDown = n+1;

  }
  
  return;
}


void PseudoCostManager::computeEstimates( const double xlp [], 
					  double zlp,
					  int branchingIdx,
					  double & dnest, double & upest )
{
  // To implement this function, query the LP solver to get all the information
  //  you need.  But only do this if you know you will need the estimates
  //  for the nodes.  (Maybe only compute estimates at the very end)...

  // Or compute estimates at the master before putting the nodes in the list?
  // Right now, I don't implement this function.

  



}


void PseudoCostManager::selectVar( const dArray<int>&BranchingCandidates, 
				   const double xlp[], double zlp, 
				   int &branchingIndex,
				   double &dndeg, double &updeg )
{
    
  int i,res = 0;
  double s,maxScore = -DBL_MAX;
  double sol,f;
  double bestdown, bestup;
  int nbc = BranchingCandidates.size();

#if defined( DEBUG_PSEUDO )
  cout << " Pseudocosts.  Selecting variable -- " << nbc << " branching Candidiates" 
       << endl;
#endif

  //initialize the pseudocost by solving LPs explicitly
  if( way_of_ini > 0 )
    {
      dArray<int> iniArray;
      for(i=0; i<nbc;i++)
	{
	  int ix = BranchingCandidates[i];
	  if( way_of_ini == 1 )
	    {
	      if( ( workingPCostArray[ix].nBranUp == 0 ) 
		  || ( workingPCostArray[ix].nBranDown == 0 ) ) 
		{
		  iniArray.append( ix );
		}
	    }
	  else
	    {
	      iniArray.append( ix );
	    }	  
	}

#if defined( DEBUG_PSEUDO )
      cout << "  of which " << iniArray.size() << " must be computed explicitly" << endl;
#endif

      if(iniArray.size()>0){
	computeFromScratch( iniArray, xlp, zlp );
      }
    }
     
  for(i=0; i<nbc;i++){
  
    int ix = BranchingCandidates[i];
    sol = xlp[ix];
    
    //XXX??? Jeff -- Qun doesn't use the fractionality of the variable?
    // (Pseudocosts should be per unit change).
    f = sol - int(sol);
    
    double down = workingPCostArray[ix].costDown * f;
    double up = workingPCostArray[ix].costUp * (1.0-f);

    double first = coef1*mmin(down,up);
    double second = coef2*mmax(down,up);

    s = first + second;
    
    //cout<<"^^^^"<<sol<<" "<<f<<" "<<down<<" "<<up<<" "<<s<<" "<<maxScore<<endl;
    

    if(s>maxScore || i==0){
      res = BranchingCandidates[i];
      maxScore = s;
      bestdown = down;
      bestup = up;
    }
  }
  
  branchingIndex = res;
  dndeg = bestdown;
  updeg = bestup;
}


  
void PseudoCostManager::computeFromScratch(const dArray<int>& iniArray,
					   const double xlp[],
					   double zlp )
{
  int nv = iniArray.size();
  
  if (nv <= 0)  return;
 
  int i;
  int* ix = new int[nv];
  for(i=0; i<nv; i++){
    ix[i] = iniArray[i];
  }
  
  double* dndeg = new double[nv];
  double* updeg = new double[nv];  


  CLPStatus lpstat = lpsolver->LPComputeDeg( itlim, nv,
					     ix, dndeg, updeg );
  
  for(i=0; i<nv; i++){

    int idx = ix[i];
    double f = xlp[idx] - (int) xlp[idx];

    workingPCostArray[idx].costUp   = (updeg[i] - zlp)/(1.0-f);
    workingPCostArray[idx].costDown = (dndeg[i] - zlp)/f;

#if defined( DEBUG_PSEUDO )
    cout << "Variable: " << idx << " Initalized to ( " 
	 <<  workingPCostArray[idx].costDown << ", "
	 <<  workingPCostArray[idx].costUp << " )" << endl;
#endif

    workingPCostArray[idx].nBranUp  = 1;
    workingPCostArray[idx].nBranDown= 1;

    newPCostArray[idx].costUp   = (updeg[i] - zlp)/(1.0-f);
    newPCostArray[idx].costDown = (dndeg[i] - zlp)/f;
    newPCostArray[idx].nBranUp  = 1;
    newPCostArray[idx].nBranDown= 1;
  }
  
  if(ix)    delete [] ix;
  if(dndeg) delete [] dndeg;
  if(updeg) delete [] updeg;
  
  return;
}

void PseudoCostManager::updateWorking( int ix, int nbu, double uv,
				       int nbd, double dv )
{
  // Only should be packed if there is something to pack
  assert( nbu + nbd > 0 );

#if defined( DEBUG_PSEUDO )
  cout << " Updating Pseudocost for variable: " << ix << " ( " << nbd << ", "
       << dv << ", " << nbu << ", " <<  uv << ")" << endl;
#endif

  if( nbu > 0 )
    {
      int n = workingPCostArray[ix].nBranUp;
      workingPCostArray[ix].costUp = ( n * (workingPCostArray[ix]).costUp + 
				       nbu * uv )/(n+nbu);
      workingPCostArray[ix].nBranUp = n+nbu;
    }
  if( nbd > 0 )
    {
      int n = workingPCostArray[ix].nBranDown;
      workingPCostArray[ix].costUp = ( n * (workingPCostArray[ix]).costDown + 
				       nbd * dv )/(n+nbd);
      workingPCostArray[ix].nBranDown = n+nbd;
    }
}


void PseudoCostManager::MWPackWorking( MWRMComm *RMC )
{

  int nCols = formulation->get_ncols();

  //XXX Keep track of this as you go instead of figuring it out  
  int numWorkingNonZero = 0;
  for( int i = 0; i < nCols; i++ )
    if(  workingPCostArray[i].nBranUp + workingPCostArray[i].nBranDown > 0 )
      numWorkingNonZero++;

  int numToPass = numWorkingNonZero;

  RMC->pack(&numToPass, 1, 1);
  
  if( numToPass > 0 ){
    int* idx = new int[numToPass];
    int* nbu = new int[numToPass];
    int* nbd = new int[numToPass];
    double* cu = new double[numToPass];
    double* cd = new double[numToPass];
    
    int tmp = 0;
    for( int i=0; i<nCols; i++)
      {
	if( workingPCostArray[i].nBranUp +  workingPCostArray[i].nBranDown > 0 )
	  {
	    idx[tmp] = workingPCostArray[i].idx;
	    cu[tmp] = workingPCostArray[i].costUp;
	    cd[tmp] = workingPCostArray[i].costDown;
	    nbu[tmp] = workingPCostArray[i].nBranUp;
	    nbd[tmp++] = workingPCostArray[i].nBranDown;
	  }
      }
    
    assert( tmp == numToPass );
    
    
  
    RMC->pack(idx,numToPass,1);
    RMC->pack(nbu,numToPass,1);
    RMC->pack(nbd,numToPass,1);
    RMC->pack(cu,numToPass,1);
    RMC->pack(cd,numToPass,1);
    
    delete [] idx;
    delete [] nbu;
    delete [] nbd;
    delete [] cu;
    delete [] cd;
  }
  
  return;
  
}


void PseudoCostManager::MWUnpackWorking( MWRMComm *RMC )
{
  int numPassed;
  
  RMC->unpack( &numPassed, 1, 1);
  
  if( numPassed > 0 ){
    int* idx = new int[numPassed];
    int* nbu = new int[numPassed];
    int* nbd = new int[numPassed];
    double* cu = new double[numPassed];
    double* cd = new double[numPassed];
    
    RMC->unpack(idx,numPassed,1);
    RMC->unpack(nbu,numPassed,1);
    RMC->unpack(nbd,numPassed,1);
    RMC->unpack(cu,numPassed,1);
    RMC->unpack(cd,numPassed,1);
    
    for(int i=0; i<numPassed; i++){
      int ix = idx[i];
      workingPCostArray[ix].idx = idx[i];
      workingPCostArray[ix].costUp = cd[i];
      workingPCostArray[ix].costDown = cd[i];
      workingPCostArray[ix].nBranUp = nbu[i];
      workingPCostArray[ix].nBranDown = nbd[i];
    }
    
    delete [] idx;
    delete [] nbu;
    delete [] nbd;
    delete [] cu;
    delete [] cd;
  }
  
  return;
}

void PseudoCostManager::MWPackNew( MWRMComm *RMC )
{
  
  int nCols = formulation->get_ncols();
  
  //XXX Keep track of this as you go instead of figuring it out  
  int numNewNonZero = 0;
  for( int i = 0; i < nCols; i++ )
    if(  newPCostArray[i].nBranUp +  newPCostArray[i].nBranDown > 0 )
      numNewNonZero++;
  
  int numToPass = numNewNonZero;

  RMC->pack(&numToPass, 1, 1);
  
  if( numToPass > 0 ){
    
    int* idx = new int[numToPass];
    int* nbu = new int[numToPass];
    int* nbd = new int[numToPass];
    double* cu = new double[numToPass];
    double* cd = new double[numToPass];
    
    int tmp = 0;
    for(int i=0; i< nCols; i++)
      {
	if(  newPCostArray[i].nBranUp +  newPCostArray[i].nBranDown > 0 )
	  {      
	    idx[tmp] = newPCostArray[i].idx;
	    cu[tmp] = newPCostArray[i].costUp;
	    cd[tmp] = newPCostArray[i].costDown;
	    nbu[tmp] = newPCostArray[i].nBranUp;
	    nbd[tmp++] = newPCostArray[i].nBranDown;
	    
	    newPCostArray[i].clear();	
	  }
      }
    
    assert( tmp == numToPass );

    RMC->pack(idx,numToPass,1);
    RMC->pack(nbu,numToPass,1);
    RMC->pack(nbd,numToPass,1);
    RMC->pack(cu,numToPass,1);
    RMC->pack(cd,numToPass,1);
    
    delete [] idx;
    delete [] nbu;
    delete [] nbd;
    delete [] cu;
    delete [] cd;
  }
  
  return;
}


void PseudoCostManager::MWUnpackNew( MWRMComm *RMC )
{

  int numPassed;
  
  RMC->unpack( &numPassed, 1, 1);
  
  if( numPassed > 0 ){
    int* idx = new int[numPassed];
    int* nbu = new int[numPassed];
    int* nbd = new int[numPassed];
    double* cu = new double[numPassed];
    double* cd = new double[numPassed];
    
    RMC->unpack(idx,numPassed,1);
    RMC->unpack(nbu,numPassed,1);
    RMC->unpack(nbd,numPassed,1);
    RMC->unpack(cu,numPassed,1);
    RMC->unpack(cd,numPassed,1);
    
    for(int i=0; i<numPassed; i++){
      updateWorking( idx[i], nbu[i], cu[i], nbd[i], cd[i] );
    }
    
    delete [] idx;
    delete [] nbu;
    delete [] nbd;
    delete [] cu;
    delete [] cd;
  }
  
  return;
}


void PseudoCostManager::clearNew()
{
  int nCols = formulation->get_ncols();

  for(int i=0; i< nCols; i++)
    {
      if(  newPCostArray[i].nBranUp +  newPCostArray[i].nBranDown > 0 )
	{      	  
	  newPCostArray[i].clear();	
	}
    }
}



#if 0
pseudoCost& pseudoCost::operator+(const pseudoCost& rhs)
{
  
  int n1, n2;
  double v1, v2;
  if(pCostArray && rhs.pCostArray){
    for(int i=0; i<nInts; i++){
      //cout<<"%%%%"<<n1<<" "<<n2<<endl;
      n1 = pCostArray[i].nBranUp;
      n2 = rhs.pCostArray[i].nBranUp;
      v1 = pCostArray[i].costUp;
      v2 = rhs.pCostArray[i].costUp;
      pCostArray[i].costUp = (n1*v1 + n2*v2)/(n1+n2);
      pCostArray[i].nBranUp = ((n1 + n2)<=2)?1:(n1+n2);
      
      n1 = pCostArray[i].nBranDown;
      n2 = rhs.pCostArray[i].nBranDown;
      v1 = pCostArray[i].costDown;
      v2 = rhs.pCostArray[i].costDown;
      pCostArray[i].costDown = (n1*v1 + n2*v2)/(n1+n2);
      pCostArray[i].nBranDown = ((n1 + n2)<=2)?1:(n1+n2);
    }
  }
  
  return *this;
}
#endif

#if 0
pseudoCost& pseudoCost::operator+=(const pseudoCost& rhs)
{
  return *this + rhs;
}
#endif

//### qun
//for checkpointing
void PseudoCostManager::write( FILE *fp )
{
  int i;
  
  //write number of Working PCosts
  fprintf ( fp, "%d \n", nWorkingPCosts );

  //write workingPCostArray
  for(i=0; i<nWorkingPCosts; i++){
    fprintf ( fp, "%d %f %f %d %d \n", 
	      workingPCostArray[i].idx,
	      workingPCostArray[i].costUp,
	      workingPCostArray[i].costDown,
	      workingPCostArray[i].nBranUp,
	      workingPCostArray[i].nBranDown );
  }
  
  
  //write number of New PCosts;
  fprintf ( fp, "%d \n", nNewPCosts );
  
  //write newPCostArray
  for(i=0; i<nNewPCosts; i++){
    fprintf ( fp, "%d %f %f %d %d \n", 
	      newPCostArray[i].idx,
	      newPCostArray[i].costUp,
	      newPCostArray[i].costDown,
	      newPCostArray[i].nBranUp,
	      newPCostArray[i].nBranDown );
  }
  
  return;
}



void PseudoCostManager::read( FILE *fp )
{
  int i;
  
  //read number of Working PCosts
  fscanf ( fp, "%d", &nWorkingPCosts );

  //read workingPCostArray
  for(i=0; i<nWorkingPCosts; i++){
    fscanf ( fp, "%d %lf %lf %d %d", 
	     &workingPCostArray[i].idx,
	     &workingPCostArray[i].costUp,
	     &workingPCostArray[i].costDown,
	     &workingPCostArray[i].nBranUp,
	     &workingPCostArray[i].nBranDown );
  }
  
  
  //read number of New PCosts;
  fscanf ( fp, "%d \n", &nNewPCosts );
  
  //read newPCostArray
  for(i=0; i<nNewPCosts; i++){
    fscanf ( fp, "%d %lf %lf %d %d", 
	     &newPCostArray[i].idx,
	     &newPCostArray[i].costUp,
	     &newPCostArray[i].costDown,
	     &newPCostArray[i].nBranUp,
	     &newPCostArray[i].nBranDown );
  }
  
  return;
}


