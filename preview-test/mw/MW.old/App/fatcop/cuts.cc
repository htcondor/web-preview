#include  <stdio.h>
#include <iostream.h>
#include "cuts.hh"
#include "PRINO.hh"
#include <MWRMComm.h>


// This is a large prime number (2^30 - 35)
#define NUMKEYS 1073741789

#if ! defined( ABS )
#define ABS( x )      ( ( x ) < 0 ? -( x ) : ( x ) )
#endif

CCut::CCut(const CCut& tcut)
{
  
  *this = tcut;
}

void CCut::MWUnpack( MWRMComm *RMC )
{
  RMC->unpack( &nnzero, 1, 1 );
  
  ind = new int [nnzero];
  coeff = new double [nnzero];
  RMC->unpack( ind, nnzero, 1 );
  RMC->unpack( coeff, nnzero, 1 );
  
  RMC->unpack(&sen,1,1);
  RMC->unpack(&rhs,1,1);

  int ihash;  
  RMC->unpack(&ihash,1,1);
  hash = (unsigned) ihash;  
  
  return;
}

void CCut::MWPack( MWRMComm *RMC )
{
  RMC->pack( &nnzero, 1, 1 );
  
  RMC->pack( ind, nnzero, 1 );
  RMC->pack( coeff, nnzero, 1 );
  
  RMC->pack(&sen,1,1);
  RMC->pack(&rhs,1,1);

  int ihash = (int) hash;  
  RMC->pack(&ihash,1,1);
  
  return;
}

void CCut::write( FILE *fp )
{
  fprintf( fp, "%d\n", nnzero );
  for( int i = 0; i < nnzero; i++ )
    fprintf( fp, "%d %lf\n", ind[i], coeff[i] );

  int ihash = (int) hash;
  fprintf( fp, "%d %lf %d\n", sen, rhs, ihash );
}

void CCut::read( FILE *fp )
{
  fscanf( fp, "%d", &nnzero );
  
  ind = new int [nnzero];
  coeff = new double [nnzero];

  for( int i = 0; i < nnzero; i++ )
    fscanf( fp, "%d %lf", &(ind[i]), &(coeff[i]) );

  int ihash;
  fscanf( fp, "%d %lf %d", &sen, &rhs, &ihash );
  hash = (unsigned) ihash;
}


void CCut::compute_hash( void )
{

  unsigned tmp1;
  hash = 1U;

  register int i;
  for( i = 0; i < nnzero; i++ )
    {
      hash *= (unsigned) (ind[i] + 1);
      
      hash %= NUMKEYS;
      
      tmp1 = (unsigned) ( ABS( coeff[i] ) );
      if( tmp1 == 0 )
        tmp1 = 5U;

      hash *= tmp1;

      hash %= NUMKEYS;
      
    }

  
  tmp1 = (unsigned) sen;
  if( tmp1 == 0 )
    tmp1 = 1U;

  hash *= tmp1;

  hash %= NUMKEYS;

  tmp1 = (unsigned) ( ABS( rhs ) );
  if( tmp1 == 0 )
    tmp1 = 1U;

  hash *= tmp1;
  
  hash %= NUMKEYS;

  hash *= nnzero;
  hash %= NUMKEYS;

  if( hash == 0 )
    {
      cerr << "hashed something to zero?" << endl;
    }
}

/****************************************************************************
*
****************************************************************************/
ostream &CCut::operator>>( ostream &o ) const
{
    o << "ID=" << id << ", PR=" << priority;
    o << ", times matched=" << times_matched;
    o << ", SEN=" << sen;
    o << ", nvars = " << nvars;
    o << ", nnzero = " << nnzero;
    o << ", sen = " << sen;
    o << ", rhs = " << rhs;
    o << ", hash = " << hash;
    o << endl;
    PRINT( o, "indices:", ind, nnzero, 10 );
    PRINT( o, "coeff:", coeff, nnzero, 10 );
    return o;
}

ostream &operator<<( ostream &o, const CCut &cut ) { return cut >> o; }




CCut& CCut::operator =(const CCut& tcut)
{
  int i;
  free();

  id = tcut.id;
  priority = tcut.priority;
  times_matched =  tcut.times_matched;
#if defined( TIMER )
  generation_time = tcut.generation_time;
#endif

  nvars = tcut.nvars;
  nnzero = tcut.nnzero;
 
  sen = tcut.sen;
  rhs = tcut.rhs;

  hash = tcut.hash;      // hash value
  
  ind = new int[nnzero];     
  for(i=0; i<nnzero; i++){
    ind[i] = tcut.ind[i];
  }
  
  coeff = new double[nnzero];
  for(i=0; i<nnzero; i++){
    coeff[i] = tcut.coeff[i];
  }

  return *this;
}

