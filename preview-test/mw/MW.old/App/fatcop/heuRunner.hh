//This class is defined to incorporate heuristics into FATCOP
//note member functions must be declared as virtual in order that
//dynamic linking can be done
//three interfaces are provide now

#ifndef HEURUNNER_HH
#define HEURUNNER_HH


//to deliver mip data when using soplex
#include "spxsolver.hh"


class heuRunner
{

public:
 
  //perform heuristics before branch-and-bound
  virtual double preHeu();
  
  //rounding heuristics
  virtual double roundingHeu(const DVector&, const SPxLP&, 
			     const DataArray<int>&);

  //searching heuristics
  virtual double searchingHeu(const DVector&, const SPxLP&, 
			      const DataArray<int>&);

    
};


#endif
