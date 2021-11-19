#include <fstream.h>

#if 0
// Jeff doesn't have access to these shared objects!

#include "fcpsolver.hh"


//shared obj name
const char* heuSharedObj = "/p/gams/systems/sol/gams/spd/heuPointer.so";
//heuristics function name
const char* heuFunction = "heuRunnerPtr";


//rounding heuristics.  This module is dynamically loaded
double FCpSolver::performRounding(const dArray<double>& x, CProblemSpec* form,
				  int* intInfo) 
{
  /*
  int (*fptr)(int);
  void *dlthing;
  
  //dynamically load the routine to be called 
  
  dlthing = dlopen(heuSharedObj, RTLD_NOW);
  if (dlthing == NULL) {
    cout<<dlerror();
    exit(1);
  }
  fptr = (int(*)(int)) dlsym(dlthing, heuFunction);
  if (fptr == NULL) {
    fprintf(stderr, "dlsym called failed\n");
    exit(1);
  }
  
  heuRunner* myRunner = (heuRunner*) (*fptr)(1);
  double roundResult = myRunner->roundingHeu(x,form,intInfo);
  
  //unload the dynamically loaded routine 
  dlclose(dlthing);
  
  return roundResult;
  */
  return 0;
  
}


double FCpSolver::performPreHeu() {
  /*  
  int (*fptr)(int);
  void *dlthing;
  
  // dynamically load the routine to be called 
  
  dlthing = dlopen(heuSharedObj, RTLD_NOW);
  if (dlthing == NULL) {
    cout<<dlerror();
    cout<<"preHeuristics is not preformed due to the error"<<endl;
    return inf;
  }
  
  fptr = (int(*)(int)) dlsym(dlthing, heuFunction);
  if (fptr == NULL) {
    fprintf(stderr, "dlsym called failed\n");
    cout<<"preHeuristics is not preformed due to the error"<<endl;
    return inf;
  }
  
  heuRunner* myRunner = (heuRunner*) (*fptr)(1);
  double preHeuResult = myRunner->preHeu();
  
  //unload the dynamically loaded routine 
  dlclose(dlthing);
  
  return preHeuResult;
  */
  return 0;

}

 
//searching heuristics.  This module is dynamically loaded
double FCpSolver::performSearching(double* x, CProblemSpec* form,
				   int* intInfo) 
{
  /*
  int (*fptr)(int);
  void *dlthing;
  
  //dynamically load the routine to be called 

  dlthing = dlopen(heuSharedObj , RTLD_NOW);
  if (dlthing == NULL) {
    cout<<dlerror();
    exit(1);
  }
  fptr = (int(*)(int)) dlsym(dlthing, heuFunction);
  if (fptr == NULL) {
    fprintf(stderr, "dlsym called failed\n");
    exit(1);
  }
  
  heuRunner* myRunner = (heuRunner*) (*fptr)(1);
  double searchResult = myRunner->searchingHeu(x,form,intInfo);
  
  // unload the dynamically loaded routine 
  dlclose(dlthing);
  
  return searchResult;
  */
  return 0;
  
}
#endif
