#include "branching.hh"
#include "PRINO.hh"
#include "pseudocost.hh"
#include <MW.h>

#define mmax(a,b) (a<b)?b:a;
#define mmin(a,b) (a<b)?a:b;



//global key function for sorting branching candidates 

const double* lpsolution;  //global variable

//input: idx is index of a branching candidate
//output: integer infeasibility of the branching candidate
//exampe: lpsolution[0] = 1.1, lpsolution[1] = 1.5, then
//        maxInfea( 0 ) = 0.4; maxInfea( 1 ) = 0;
//we are going to sort the branching candidates in ascending order,
//so variable 1 is before variable 0 in the candidate list
double get_closeness_to_half( int idx )
{
  
  if( NULL == lpsolution ){
    cerr<<" In branching key function : lpsolution is not defined " << endl;
    exit(1);
  }
  
  double sol = lpsolution[ idx ];
  
  double tmp = sol - int( sol ) - 0.5;
 
  return ( tmp >= 0 )? tmp : -tmp;
  
};



//------------------------------------------------------------------
//               constructors and destructor
//------------------------------------------------------------------
BranchManager::BranchManager(CProblemSpec* form, 
			     int br,
			     PseudoCostManager *pcm,
			     int* pri )

{
  formulation = form;
  branchingRule = br;  
  pseudocosts = pcm;
  priority = pri;

  branTime = 0.0;

}


//no memory need to be freed
BranchManager::~BranchManager()
{
}

void BranchManager::setBranTime( double t )
{
  branTime = t;
}

double BranchManager::getBranTime( void )
{
  return( branTime );
}

void BranchManager::setMaxBranVars( int m )
{
  maxBranVars = m;
}

int BranchManager::getMaxBranVars()
{
  return maxBranVars;
}


void BranchManager::select_a_var( double *xlp, 
				  double zlp, 
				  int &branchingIndex,
				  double &dndeg,
				  double &updeg,
				  double &dnest,
				  double &upest )				
{

  clock_t startTick = clock();
  int i;


  // Figure out "candidate" variables.
  dArray <int> BranchingCandidates;

  // For example -- for "strong" branching, you may only wish to
  // consider branching on the "most fractional", or use some
  // other measure.  With priorities, you need to have only candidates
  // that are fractional and in the "top" priority group.

  //XXX Right now, all fractional variables are candidates!

  // Branching candidates are in terms of the ORIGINAL 
  //  Variables...


  for( int j = 0; j < formulation->nInts; j++ )
    {
      if( ! INTEGRAL( xlp[formulation->intConsIndex[j]] ) )
	{
	  BranchingCandidates.append( formulation->intConsIndex[j] );
	}
    }


  // If user priority is supplied, we will branch only on the
  // variables that have the lowest priority
  if( priority ){

    dArray <int> tmpArray = BranchingCandidates;

    //find the set of branching candidates with lowest priority
    int minPri;
    
    //find minimum priority
    for( i=0; i<tmpArray.size(); i++ ){
      if(i == 0){
	minPri = priority[ tmpArray[i] ];
      }
      else{
	if( priority[ tmpArray[i] ] < minPri ){
	  minPri = priority[ tmpArray[i] ];
	}
      }
    }
    
    //fill BranchingCandidates with vars having minimum priority
    
    BranchingCandidates.clear(); //clear all candidate vars
    
    for( i = 0; i < tmpArray.size(); i++){
      if( priority[ tmpArray[i] ] == minPri ){
	BranchingCandidates.append( tmpArray[i] );
      }
    }
  }
  
  
  if( BranchingCandidates.size() == 0 )
    {
      branchingIndex = -1;
      goto EXIT;
    }
  
  if( BranchingCandidates.size() == 1 ) 
    {
      branchingIndex = BranchingCandidates[0];
      goto EXIT;
    }

  // Must the number of candidate variables be limited?
  if( maxBranVars < BranchingCandidates.size() )
    {
      lpsolution = xlp; //let global variable lpsolution point to xlp
  
      BranchingCandidates.set_array_key( &get_closeness_to_half );
      BranchingCandidates.qsort_by_key();

      for( int i = 0; i < BranchingCandidates.size() -  maxBranVars; i++ )
	{
	  BranchingCandidates.removeLast();
	}
    }
  
  switch(branchingRule){
  case 0:  //peudocost 
    pseuCost( BranchingCandidates, xlp, zlp,
	      branchingIndex, dndeg, updeg, dnest, upest );
    break;
  case 1:  //natural order
    naturalOrder( BranchingCandidates, xlp, zlp,
		  branchingIndex, dndeg, updeg, dnest, upest );
    break;
  case 2:  //max infeasibility
    maxIntInfea( BranchingCandidates, xlp, zlp,
		 branchingIndex, dndeg, updeg, dnest, upest );
    break;
  case 3:  //min infeasibility
    minIntInfea( BranchingCandidates, xlp, zlp,
		 branchingIndex, dndeg, updeg, dnest, upest );
    break;
  default:
    cerr<<"wrong branching algorithm."<<endl;
    cerr<<"use default branching algorithm: pseudocost"<<endl;
    pseuCost( BranchingCandidates, xlp, zlp, 
	      branchingIndex, dndeg, updeg, dnest, upest );
    break;
  }

 EXIT:
  clock_t finishTick = clock();
  double tbt = (double) (finishTick-startTick)/CLOCKS_PER_SEC; 
  MWprintf( 75, "Branching operation took %.2lf secs\n", tbt );
  branTime += tbt;
  
}



void BranchManager::naturalOrder(const dArray <int>& BranchingCandidates, 
				 const double xlp[], double zlp,
				 int &branchingIndex,
				 double &dndeg, double &updeg, 
				 double &dnest, double &upest )
{
  branchingIndex = BranchingCandidates[0];
  return;
}

void BranchManager::maxIntInfea( const dArray <int>& BranchingCandidates,
				 const double xlp [], double zlp,
				 int &branchingIndex,
				 double &dndeg, double &updeg,
				 double &dnest, double &upest )
{
  int idx;
  int index = -1;
  double sol;
  double f;
  double max = -1.0;
    
  for( int i=0; i< BranchingCandidates.size(); i++ ){
	
    idx = BranchingCandidates[ i ];
    sol = xlp[ idx ];
    f = sol - int( sol );
    f = (f < 0.5) ? f : 1-f;
		
    if ( f > max ){
      max = f; 
      index = idx;
    }
  }
    
  branchingIndex = index;
  
};

 
void BranchManager::minIntInfea( const dArray <int>& BranchingCandidates,
				 const double xlp [], double zlp,
				 int &branchingIndex,
				 double &dndeg, double &updeg,
				 double &dnest, double &upest )
{
  int idx;
  int index = -1;
  double sol;
  double f;
  double min = 2;
  
  for(int i=0; i< BranchingCandidates.size(); i++){
    
    idx = BranchingCandidates[ i ];
    sol = xlp[ idx ];
    f = sol - int( sol );
    f = (f < 0.5) ? f : 1-f;
    
    if ( f < min ) {
      min = f;
      index = idx;
    }
  }
  
  branchingIndex = index;
  
};



void BranchManager::pseuCost( const dArray<int>&BranchingCandidates, 
			      const double xlp[], double zlp, 
			      int &branchingIndex,
			      double &dndeg, double &updeg, 
			      double &dnest, double &upest )
  
{

  pseudocosts->selectVar( BranchingCandidates,
			  xlp,	
			  zlp,
			  branchingIndex,
			  dndeg,
			  updeg );

  pseudocosts->computeEstimates( xlp, zlp, branchingIndex, dnest, upest );
  
}




