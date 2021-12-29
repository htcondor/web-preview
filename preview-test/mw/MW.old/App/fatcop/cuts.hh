// This is the cut object class, updated on 6/22/99

#ifndef CUTS_H
#define CUTS_H

/*** INCLUDES ***/

extern "C" {
#include <stdlib.h>
#include <stdio.h>
}
#include <iostream.h>

class MWRMComm;

/*** TYPES ***/

typedef double CUT_PRIORITY;

typedef int CSense;
#define INVALID_SENSE    0
#define LESS_OR_EQUAL    1
#define EQUAL_TO         2
#define GREATER_OR_EQUAL 3

class CCut
{
private:
  long id;
  CUT_PRIORITY priority;
  int times_matched;
#if defined( TIMER )
  CTimePeriod generation_time;
#endif

  int nvars;
  int nnzero;
  int *ind;      // indices of the coeffs
  double *coeff; // array of size nnzero
  CSense sen;
  double rhs;

  unsigned hash;      // hash value 

public:
  CCut( void ) : id( 0 ), priority( 0 ), times_matched( 0 ), 
#if defined( TIMER )
    generation_time( 0 ), 
#endif
    nvars( 0 ), 
    nnzero( 0 ), ind( 0 ), coeff( 0 ), sen( INVALID_SENSE ), rhs( 0 ), 
    hash( 1 ) {}
  CCut(const CCut&);
  
  ~CCut() 
    { 
      if( ind ) delete [] ind ; ind = 0;
      if( coeff ) delete [] coeff ;  coeff = 0;
    }

  long get_id( void ) const { return id; }
  CUT_PRIORITY get_priority( void ) const { return priority; }
  int get_times_matched( void ) const { return times_matched; } 
#if defined( TIMER )
  CTimePeriod get_generation_time( void ) const { return generation_time; }
#endif
  
  int get_nvars( void ) const { return nvars; }
  int get_nnzero( void ) const { return nnzero; }
  const int *get_ind( void ) const { return ind; }
  const double *get_coeff( void ) const { return coeff; }
  CSense get_sense( void ) const { return sen; }
  double get_rhs( void ) const { return rhs; }

  void set_id( long i ) { id = i; }
  void set_priority( CUT_PRIORITY p ) { priority = p; }
  void set_times_matched( int t ) { times_matched = t; }
#if defined( TIMER )
  void set_generation_time( CTimePeriod t ) { generation_time = t; }
#endif
  
  void set_nvars( int n ) { nvars = n; }
  void set_nnzero( int nz ) { nnzero = nz; }
  void set_ind( int *i ) { ind = i; }
  void set_coeff( double *c ) { coeff = c; }
  void set_sense( CSense s ) { sen = s; }
  void set_rhs( double r ) { rhs = r; }

  ostream &operator>>( ostream &o ) const;

  void free( void )
    {
      if( ind ) delete [] ind ; ind = NULL;
      if( coeff ) delete [] coeff ; coeff = NULL;
    }

  void compute_hash( void );
  const unsigned get_hash( void ) const { return hash; }

  void MWPack( MWRMComm * );
  void MWUnpack( MWRMComm * );

  CCut& operator =(const CCut&);

  void write( FILE *fp );
  void read( FILE *fp );
  
};
ostream &operator<<( ostream &o, const CCut &cut );

#endif
