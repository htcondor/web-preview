//---------------------------------------------------------------------------
// Title    :  cutpool.hh
//
// Author   :  Qun Chen
//
// Purpose  :  a class managing cuts generated
//
// History  :  When      Who      What
//             07/06/99  Qun      Created.
//
// Copyright:  FATCOP group, UW-Madison
//---------------------------------------------------------------------------

#ifndef CUTPOOL_HH
#define CUTPOOL_HH

#include <iostream.h>
#include "cuts.hh"

class cutPool
{
public:
  
  //------------------------------------------------------------------
  //               constructors and destructor 
  //------------------------------------------------------------------
  cutPool(int ms = 1000);
  ~cutPool();      
  
  //------------------------------------------------------------------
  //               inquiry
  //------------------------------------------------------------------
  int size() const;
  int maxSize() const;
  
  void getCuts(CCut**& cuts, int& n) const;

  //------------------------------------------------------------------
  //               operators
  //-------------------------------------------------------------------
  CCut*& operator[](int);
  const CCut*& operator[](int) const;
  
  //------------------------------------------------------------------
  //               modifications
  //------------------------------------------------------------------ 
  void insert(CCut*);
  void insert(CCut**, int);
  void clear();


  void print();

  void write( FILE *fp );
  void read( FILE *fp );
private:
  
  int size_;
  int maxSize_;
  int hand; //point to the oldest cut

  CCut** array;
  
};

#endif






