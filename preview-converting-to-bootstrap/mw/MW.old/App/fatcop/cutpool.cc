#include "cutpool.hh"
//#include "error.hh"
extern void Error( bool condition, const char *errMsg , bool quit);



//------------------------------------------------------------------
//               constructors and destructor 
//------------------------------------------------------------------

cutPool::cutPool(int ms = 1000)
{
  size_ = 0;
  maxSize_ = ms;
  hand = 0;
  
  array = new CCut*[maxSize_];
  for(int i=0; i<maxSize_; i++){
    array[i] = NULL;
  }
}



cutPool::~cutPool()
{
  clear();
  delete [] array;
}

  

//------------------------------------------------------------------
//               inquiry
//------------------------------------------------------------------
int cutPool::size() const
{
  return size_;
}


int cutPool::maxSize() const
{
  return maxSize_;
}


void cutPool::getCuts(CCut**& cuts, int& n) const
{
  cuts = array;
  n = size_;
  return;
}
 
//------------------------------------------------------------------
//               operators
//-------------------------------------------------------------------
CCut*& cutPool::operator[](int idx)
{
  Error( (idx<0 || idx>=size_), 
	 "cutPool operator[], index out of range.", 1);
  return array[idx];
}

  
const CCut*& cutPool::operator[](int idx) const
{
  Error( (idx<0 || idx>=size_), 
	 "cutPool operator[], index out of range.", 1);
  return array[idx];
}



//------------------------------------------------------------------
//               modifications
//------------------------------------------------------------------ 
void cutPool::insert(CCut* c)
{
  if(NULL == c) return;

  if(size_ == maxSize_) {
    delete c; c=0;
    return;
  }
  

  unsigned int chash = c->get_hash();
  
  //prevent duplication, assume we trust our hash function
  for(int i=0; i<size_; i++){
    if(array[i]->get_hash() == chash){
      delete c;  c=0;
      return;
    }
  }

  //if(array[hand]!=NULL) delete array[hand];
  
  array[hand] = c;

  if(size_ < maxSize_) size_++;
  hand = (hand+1)%maxSize_;
  return;
}


void cutPool::insert(CCut** cuts, int n)
{
  for(int i=0; i<n; i++){
    insert(cuts[i]);
  }
  return;
}


void cutPool::clear(){
 
  for(int i=0; i<size_; i++){
    if(array[i]){
      //cout<<"destroy cut "<<array[i]->get_id()<<endl;
      delete array[i];
      array[i] = NULL;
    }
  }
  // don't -- delete [] array;
  
  size_ = 0;
  hand = 0;
}


void cutPool::print()
{
  cout<<"maximum pool size "<<maxSize_<<endl;
  cout<<"number of cuts in pool "<<size_<<endl;
  cout<<"currently hand is in "<<hand<<endl;
  
  cout<<"cuts are:"<<endl;
  for(int i=0; i<size_; i++){
    cout<<array[i]->get_id()<<" ";
  }
  cout<<endl;
  
  return;
}


void cutPool::write( FILE *fp )
{
  fprintf( fp, "%d\n", size_ );
  fprintf( fp, "%d\n", hand );

  for( int i = 0; i < size_; i++ )
    array[i]->write( fp );
}

void cutPool::read( FILE *fp )
{
  fscanf( fp, "%d", &size_ );
  fscanf( fp, "%d", &hand );

  for( int i = 0; i < size_; i++ )
    {
      array[i] = new CCut;
      array[i]->read( fp );
    }

}
