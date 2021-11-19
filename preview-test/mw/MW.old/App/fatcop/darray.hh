//---------------------------------------------------------------------------
// Title    :  darray.hh
//
// Author   :  Qun Chen
//
// Purpose  :  A templated dynamic array.  The size of this array can always
//             increase, the only upper bound is memory restrictions.
//
// Usage    :  Assme operator = has been defined in T
//             Assume operator== has been defined in T, for method contains()
//             Assume operator< and > have been defined, for method sort()
//             example: dArray <int> intArray;
//
// History  :  When      Who      What
//             07/01/99  Qun      Created.
//
// Copyright:  FATCOP group, UW-Madison
//---------------------------------------------------------------------------

#ifndef DARRAY_HH
#define DARRAY_HH

#include <iostream.h>
#include <memory.h>

typedef double Key;  //"key" by which array is managed

#define TEMPLATE  template <class T> 

TEMPLATE class dArray
{
public:

  //------------------------------------------------------------------
  //               constructors and destructor 
  //------------------------------------------------------------------
  dArray();
  dArray(int,int);
  dArray(const dArray&);
  ~dArray();
  

  //------------------------------------------------------------------
  //               inquiry
  //------------------------------------------------------------------
  int size() const;
  int maxSize() const;

  bool empty() const;
  bool contains(const T&) const; //assume == has been defined in T
  int index(const T&) const; //find the first index of given item in array
  
  //------------------------------------------------------------------
  //               operators
  //------------------------------------------------------------------
  T& operator[](int);
  const T& operator[](int) const;

  dArray& operator=(const dArray&);
  

  //------------------------------------------------------------------
  //               modifications
  //------------------------------------------------------------------
  void insert(int, const T&);
  void prepend(const T&);
  void append(const T&);
  
  void remove(int);
  void remove(int, T&);
  void pop();
  void pop(T&);
  void removeLast();
  void removeLast(T&);
  
  void reSize(int); 

  void clear();
  void qsort_by_key(int order=1);
  void qsort(int order=1); //order>=0: increasing order<0: decreasing
  void qsort(int order,int left,int right);
  void qsort_by_key(int order,int left,int right);


  //------------------------------------------------------------------
  //               others
  //------------------------------------------------------------------
  void set_array_key(Key(*)(T)); //set up the current key function

  void print();
  
  //------------------------------------------------------------------
  //               data members and private functions
  //------------------------------------------------------------------
  
protected:
  
  int size_;
  int maxSize_;
  T* array;
  
  // A pointer to a user defined function that takes an T
  // and returns the "key" for this item.
  Key (*item_key)(T);
  
  
};











//************************************************************************
//
//                               definition                              *
//
//************************************************************************








extern void Error( bool condition, const char *errMsg , bool quit);

const double INCFACTOR = 1.2;

//------------------------------------------------------------------
//               constructors and destructor 
//------------------------------------------------------------------
TEMPLATE dArray<T>::dArray()
{
  size_ = 0;
  maxSize_ = 0;
  array = NULL;

  item_key = NULL;
}


TEMPLATE dArray<T>::dArray(int s, int ms)
{
  size_ = s;
  maxSize_ = ms;
  array = new T[maxSize_];
  
  Error( (NULL==array), "dArray can't allocate new memory.", 1); 

  item_key = NULL;
}


TEMPLATE dArray<T>::dArray(const dArray& da)
{
  //purify complains if don't initialize this values
  size_ = 0;
  maxSize_ = 0;
  array = NULL;
  item_key = NULL;
  
  *this = da;
}


TEMPLATE dArray<T>::~dArray()
{
  if(array) delete [] array;
}



//------------------------------------------------------------------
//               inquiry
//------------------------------------------------------------------
TEMPLATE int dArray<T>::size() const
{
  return size_;
}


TEMPLATE int dArray<T>::maxSize() const
{
  return maxSize_;
}


TEMPLATE bool dArray<T>::empty() const
{
  return (size_ <= 0);
}


//assume operator == has been defined in T
TEMPLATE bool dArray<T>::contains(const T& item) const
{
  bool result = false;
  
  for(int i = 0; i<size_; i++){
    if(item==array[i]){
      result = true;
      break;
    }
  }
  
  return result;
}


//assume operator == has been defined in T
TEMPLATE int dArray<T>::index(const T& item) const
{
  int result = -1;
  
  for(int i = 0; i<size_; i++){
    if(item==array[i]){
      result = i;
      break;
    }
  }
  
  return result;
}

  
  
//------------------------------------------------------------------
//               operators
//------------------------------------------------------------------
TEMPLATE T& dArray<T>::operator[](int idx)
{
  Error( (idx<0 || idx>=size_), 
	 "dArray operator[], index out of range.", 1);
  
  return array[idx];
}



TEMPLATE const T& dArray<T>::operator[](int idx) const
{
  Error( (idx<0 || idx>=size_), 
	 "dArray operator[], index out of range.", 1);
  
  return array[idx];
} 



TEMPLATE dArray<T>& dArray<T>::operator=(const dArray<T>& rhs)
{
  reSize(rhs.size_);
  memcpy(array, rhs.array, size_*sizeof(T)) ;
  item_key = rhs.item_key;
  return *this ;
}
  

TEMPLATE void dArray<T>::print()
{
  for(int i=0; i<size_; i++){
    cout<<array[i]<<" ";
  }
  cout<<endl;
  
  return;
}


		       
  
//------------------------------------------------------------------
//               modifications
//------------------------------------------------------------------

//insert "item" before index "idx"
//example: before insertion: 1 6 4
//         insert "5" before index "1"
//         after insertion: 1 5 6 4
TEMPLATE void dArray<T>::insert(int idx, const T& item)
{
  Error( (idx<0 || idx>size_), 
	 "dArray insert, index out of range.", 1);
 
  reSize(size_+1);
  
  int cur = size_ - 2;
  while(idx <= cur){
    array[cur+1] = array[cur];
    cur--;
  }
  
  array[idx] = item;
  
  return;
}


TEMPLATE void dArray<T>::append(const T& item)
{
  insert(size_,item);
  return;
}


TEMPLATE void dArray<T>::prepend(const T& item)
{
  insert(0,item);
  return;
}

  
TEMPLATE void dArray<T>::remove(int idx)
{
  Error( (idx<0 || idx>=size_), 
	 "dArray remove, index out of range.", 1);
 
  for(int i=idx; i<size_ - 1; i++){
    array[i] = array[i+1];
  }
  
  size_--;
  
  return;
}


TEMPLATE void dArray<T>::remove(int idx, T& item)
{
  Error( (idx<0 || idx>=size_), 
	 "dArray remove, index out of range.", 1);
  
  item = array[idx];
  
  remove(idx);
  
  return;
}



TEMPLATE void dArray<T>::pop()
{
  remove(0);
  return;
}


TEMPLATE void dArray<T>::pop(T& item)
{
  remove(0,item);
  return;
}


TEMPLATE void dArray<T>::removeLast()
{
  remove(size_-1);
  return;
}


TEMPLATE void dArray<T>::removeLast(T& item)
{
  remove(size_-1,item);
  return;
}


TEMPLATE void dArray<T>::clear()
{
  size_ = 0;
  return;
}

TEMPLATE void dArray<T>::qsort_by_key(int order = 1)
{
  Error((NULL == item_key), 
	"dArray qsor_by_key, Key function is not defined", 1);
  qsort_by_key(order, 0, size_-1);
  return;
}


//quick sort. order>=0: increasing order<0: decreasing
//assume operator > and < have been defined in T
TEMPLATE void dArray<T>::qsort(int order = 1)
{
  qsort(order,0,size_-1);
  return;
}


//------------------------------------------------------------------
//               others
//------------------------------------------------------------------
TEMPLATE void dArray<T>::set_array_key(Key(* f)(T)){ //set up the current key function

  item_key = f;
  
  return;
  
}

//------------------------------------------------------------------
//               private function
//------------------------------------------------------------------
TEMPLATE void dArray<T>::reSize(int newSize)
{
  Error( newSize<0, "dArray reSize, negative new size", 1);
  
  if(newSize > maxSize_){//need to reallocate memory
    maxSize_ = (int) INCFACTOR*newSize + 1; //allocate a liitle bit more memory
    
    T* oldArray = array;
    array = new T[maxSize_];
    Error( (NULL==array), "dArray can't allocate new memory.", 1); 
    
    for( int i = 0; i < size_; i++ ){
      array[i] = oldArray[i];
    }
    
    delete [] oldArray; //take back memory
  }
  
  size_ = newSize;
  
  return;
}



TEMPLATE void dArray<T>::qsort(int order,int left, int right)
{

  T temp;

  int i=left;
  int j=right;
  int mid = (int) (left+right)/2;

  T pivot = array[mid];

  do{
    //look for a pair to swap
    if(order<0){ //decreasing
      while(array[i] > pivot) i++;
      while(array[j] < pivot) j--;
    }
    else{         //increasing
      while(array[i] < pivot) i++;
      while(array[j] > pivot) j--;
    }
      
    if(i<=j){ //do a swap
      temp = array[i];
      array[i]=array[j];
      array[j]=temp;
      //continue to look for a pair to swap
      i++;
      j--;
    }
  } while(i<j);
  
  if(left<j)   qsort(order,left,j);
  if(i<right)  qsort(order,i,right);

  return;
  
}


TEMPLATE void dArray<T>::qsort_by_key(int order,int left, int right)
{

  T temp;
  
  int i=left;
  int j=right;
  int mid = (int) (left+right)/2;

  T pivot = array[mid];

  do{
    //look for a pair to swap
    if(order<0){ //decreasing
      while((*item_key)(array[i]) > (*item_key)(pivot)) i++;
      while((*item_key)(array[j]) < (*item_key)(pivot)) j--;
    }
    else{         //increasing
      while((*item_key)(array[i]) < (*item_key)(pivot)) i++;
      while((*item_key)(array[j]) > (*item_key)(pivot)) j--;
    }
      
    if(i<=j){ //do a swap
      temp = array[i];
      array[i]=array[j];
      array[j]=temp;
      //continue to look for a pair to swap
      i++;
      j--;
    }
  } while(i<j);
  
  if(left<j)   qsort_by_key(order,left,j);
  if(i<right)  qsort_by_key(order,i,right);

  return;
  
}


#endif


