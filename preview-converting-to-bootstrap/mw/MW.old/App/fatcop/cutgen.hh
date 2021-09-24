/*---------------------------------------------------------------------------
--  FILENAME:  INCLUDE/cutgen.h
--  AUTHOR:    Kalyan S. Perumalla
               Jeff T. Linderoth     
---------------------------------------------------------------------------*/
/*****************************************************************************
*
*****************************************************************************/
#ifndef CUTGEN_H
#define CUTGEN_H


/*** INCLUDES ***/

#include <iostream.h>
#include "PRINO.hh"
#include "cuts.hh"

/*** CONSTANTS ***/


/*** TYPES ***/

typedef bool CBool;

#ifndef EPSILON
#define EPSILON 1e-10
#endif



//-----------------------------------------------------------------------------
class CCutGenerator
{
 public:
  CCutGenerator( void ) {}
  virtual ~CCutGenerator() {}

 public:
  /**
     Given a problem formulation and solution,
     find cuts.
   */
  virtual int generate( const CProblemSpec *p,
			const CLPSolution *sol,
			int max_cuts, CCut **cuts ) = 0;
};



/// This is the class implementing MINTOs lifted knapsack cover generation.
class CKnapsackCutGen : public CCutGenerator
{
public:
  CKnapsackCutGen( void ) { };
  CKnapsackCutGen( int n )
  {
    setup_knapcover( n );
  }
  ~CKnapsackCutGen()
  {
    cleanup_knapcover();
  }
  
 public:
  int generate( const CProblemSpec *p, const CLPSolution *sol,
		int max_cuts, CCut **cuts );
  
 protected:

#define KNAP_THRESHOLD -INFBOUND
#define KNAP_TOLERANCE 0.05

#ifndef UNDEFINED
#define UNDEFINED -1
#endif

#define COMPVAL 0x400000
#define KP_IND(X) \
  ((kp_ind[X] < COMPVAL) ? kp_ind[X] : kp_ind[X] - COMPVAL)

  bool knapsack_cover
    (
     const CProblemSpec *p,
     const double *xlp,
     const double *dj,
     int nz,              // First five arguments specify the knapsack
     int *ind,            // row
     double *coef,
     char sense,
     double rhs,      
     int *cut_nz,           // These return the cut -- including a 
     int **cut_ind,         // "priority"
     double **cut_coef,
     char *cut_sense,
     double *cut_rhs,
     double *priority
     );
  
  int kp_nzcnt;
  double *kp_obj;
  int *kp_ind;
  double *kp_val;
  double kp_rhs;
  bool *mark;

  void setup_knapcover( int n )
    {      
      kp_obj = new double [n];
      kp_ind = new int [n];      
      kp_val = new double [n];
      mark = new bool [n];      
    }
  
  void cleanup_knapcover( void )
    {
      if ( kp_obj ) delete [] kp_obj; kp_obj = 0;      
      if ( kp_ind ) delete [] kp_ind; kp_ind = 0;      
      if ( kp_val ) delete [] kp_val; kp_val = 0;
      if ( mark ) delete [] mark; mark = 0;
    }
  
  void flip( int nz, int *ind, double *coef, double *rhs )
    {
      for( int i = 0; i < nz; i++)
	coef[i] = -coef[i];
      *rhs = -(*rhs);
    }
  
  void complement( int nz, int *ind, double *coef, double *rhs )
    {
      for( int i = 0; i < nz; i++)
	if ( coef[i] < -EPSILON )
	  {
	    ind[i] += COMPVAL;
	    coef[i] = -coef[i];
	    *rhs += coef[i];
	  }
    }

  void recomplement( int nz, int *ind, double *coef, double *rhs )
    {
      for( int i = 0; i < nz; i++)
	if ( ind[i] >= COMPVAL )
	  {
	    ind[i] -= COMPVAL;
	    coef[i] = -coef[i];
	    *rhs += coef[i];
	  }
    }

// This is for sorting...

#define ORDERSIZE  15000
  
  int b1[ORDERSIZE+1];
  int b0[ORDERSIZE];
  double a1[ORDERSIZE+1];
  double c1[ORDERSIZE];  

  CBool ordervars( double *a, int *b, double *c, int n );


// This is for solving the knapsack problem...

#define KNAPSIZE  10000

  int asum;
  double dp1[KNAPSIZE];
  double dp2[KNAPSIZE];
  
  void knapinit( void ) { asum = 0; }

  CBool knapvar( double c, int a );
  
  int knapvalue( double b )
    {
      int k;
      for( k = 1; dp1[k] < b + EPSILON; k++ ) ;
      return( k-1 );
    }

};


//-----------------------------------------------------------------------------
class CFlowCutGen : public CCutGenerator
{
 public:
  CFlowCutGen( void ) 
    {
      sign = cand = mark = NULL;
      x = up = y = ratio = xcoe = ycoe = mt = lo = NULL;
    };
  CFlowCutGen( int n )
  {
    setup_flowcover( n );
  }
  ~CFlowCutGen()
  {
    cleanup_flowcover();
  }
  
 public:
  int generate( const CProblemSpec *p, const CLPSolution *sol,
	       int max_cuts, CCut **cuts );

 private:

#define FLOW_TOLERANCE 0.05

#define CONTPOS  1
#define BINPOS   2
#define CONTNEG -1
#define BINNEG  -2

#define PRIME      1
#define SECONDARY  2

#define INCUT      1
#define INCUTDONE  2
#define INLMIN     3
#define INLMINDONE 4
#define INLMINMIN  5

  int *sign;
  int *cand;
  int *mark;
  double *up;
  double *x;
  double *y;
  double *ratio;
  
  double *xcoe;
  double *ycoe;

  double *mt;
  double *lo;

  CBool flow_cover
    (
     const CProblemSpec *prob,
     const CLPSolution *sol,
     int nz,
     int *ind,
     double *coef,
     char sense,
     double rhs,
     int *cut_nz,               // These return the cut -- including a 
     int **cut_ind,             // priority.  (They also allocate the
     double **cut_coef,          // required memory).
     char *cut_sense,
     double *cut_rhs,
     double *priority
     );

  void liftcoe1
    (
     double *cyj,
     double *cxj,
     double upj,
     double xj,
     double yj,
     int splus,
     double tmp1,
     double lambda
     );

  void liftcoe2
    (
     double *cxj,
     double upj,
     int splus,
     double tmp,
     double lambda
     );

  void setup_flowcover( int n )
    {
      up = NEWV( double, n );

      sign = NEWV( int, n );
      cand = NEWV( int, n );
      mark = NEWV( int, n );

      x = NEWV( double, n );
      y = NEWV( double, n );
#if defined( DEBUG_SOLARIS2 )
      chout << "x is initialized to " << x << endl;
      chout << "y is initialized to " << y << endl;
      chout << "up is initialized to " << up << endl;
      chout << "mark is initialized to " << mark << endl;
#endif
      ratio =  NEWV( double, n );
      xcoe =  NEWV( double, n );
      ycoe =  NEWV( double, n );
      mt =  NEWV( double, n );
      lo =  NEWV( double, n );
    }

  void cleanup_flowcover( void )
    {
      if( sign ) DELETEV( sign ); sign = NULL;
      if( cand ) DELETEV( cand ); cand = NULL;
#if defined( DEBUG_SOLARIS2 ) 
      chout << "At the end, x is " << x << endl;
      chout << "At the end, y is " << y << endl;
      chout << "At the end, up is " << up << endl;
      chout << "At the end, mark is " << mark << endl;
#endif
      if( mark ) DELETEV( mark ); mark = NULL;
      if( up ) DELETEV( up ); up = NULL;
      if( x ) DELETEV( x ); x = NULL; 
      if( y ) DELETEV( y ); y = NULL;
      if( ratio ) DELETEV( ratio ); ratio = NULL;
      if( xcoe ) DELETEV( xcoe ); xcoe = NULL;
      if( ycoe ) DELETEV( ycoe ); ycoe = NULL;
      if( mt ) DELETEV( mt ); mt = NULL;
      if( lo ) DELETEV( lo ); lo = NULL;
    }

  void flip( int nz, int *ind, double *coef, double *rhs )
    {
      for( int i = 0; i < nz; i++)
	coef[i] = -coef[i];
      *rhs = -(*rhs);
    }
};




//-----------------------------------------------------------------------------
//
// This is the class implementing MINTOs surrogate knapsack cover generation.
//  (It will be easy to add too).
//

class CSKnapsackCutGen : public CKnapsackCutGen
{
 public:
  CSKnapsackCutGen ( void ) {};
  CSKnapsackCutGen( int n1, int n2 ) : CKnapsackCutGen( n2 )
  {
    sk_ind = NEWV( int, n1 );
    sk_val = NEWV( double, n1 );
  }
  ~CSKnapsackCutGen()
  {
    DELETEV( sk_ind );
    DELETEV( sk_val );
  }

public:
  int generate( const CProblemSpec *p, const CLPSolution *sol,
		int max_cuts, CCut **cuts );

private:
  int sk_nzcnt;
  int *sk_ind;
  double *sk_val; 
  char sk_sense;
  double sk_rhs; 

  CBool sknapsack_cover 
  (
   const CProblemSpec *prob,
   const CLPSolution *sol,
   int nz,
   int *ind,
   double *coef,
   char sense,
   double rhs,
   int *cut_nz,               // These return the cut -- including a 
   int **cut_ind,             // priority.  (They also allocate the
   double **cut_coef,          // required memory).
   char *cut_sense,
   double *cut_rhs,
   double *priority
   );
  
};

#if 0
//-----------------------------------------------------------------------------
//
// This is the class implementing Ceria's Lift and project cut generation
//  Right now it is only implemented in XPRESS.  (and it doesn't work)
//

#if defined( XPRESS )

class CLiftAndProjectCutGen : public CCutGenerator
{
public:
  CLiftAndProjectCutGen( int ncol, int nrow, int max_cuts );
  ~CLiftAndProjectCutGen();

  int generate( const CProblemSpec *p, const CLPSolution *sol,
		int max_cuts, CCut cuts[] );
private: 


  /* From xpgl.h */

#define MAXCOLCLP 10000    // Max # of columns in "cutgen" LP
  /*#define DEBUG_XP         0 */
#define zero             0.0
#define MACHEPSILON      0.00000001
#define BOUNDTOL 100
#define CLPMETHOD 0
#define QUALITY_TOL 0.5
#define MAX_LEAVING 10


  // A candidate fractional solution
typedef struct {
  double value;
  int index;
} CANDIDATE;

  // The solution
typedef struct {
  double *value;
  double *slack;
  int    *basis;
  double  obj;
  double  quality;
} SOLUTION;

  /* From liftandp.h */

  // This one generates the cuts
int  gencut(double *,
	    int *,
	    int *,
	    int,
	    double *,
	    int *,
	    int,
	    int,
            char *,
	    double *, //gub
	    double *, //glb
	    double *,
	    int *,
	    int *,
	    int *,
	    double *,
	    int *,
	    double *,
	    int *,
	    int ,
	    int ,
	    int );

void compress_rows(int nr,
		   int *rowbeg,
		   char *sense,
		   double *rhs,
		   int *rowind,
		   double *rowval,
		   int nz,
		   int *rstat,
		   double *slack);
  
  /* From xpivot4.c */

int Select_couple (int  start_col,
                   int  end_col,
                   double *x,
                   double *dobj,
                   double tolerance);

int Find_fractional ();

int Find_min (CANDIDATE *candidate,
              int        candidate_list,
              int       *index);



  /* From util.h */

  // We may not need much of this...

struct Bitvec_piece1 {
  unsigned int bit0:1, bit1:1, bit2:1, bit3:1, bit4:1, 
               bit5:1, bit6:1, bit7:1, bit8:1, bit9:1, 
               bit10:1, bit11:1, bit12:1, bit13:1, bit14:1, 
               bit15:1, bit16:1, bit17:1, bit18:1, bit19:1, 
               bit20:1, bit21:1, bit22:1, bit23:1, bit24:1, 
               bit25:1, bit26:1, bit27:1, bit28:1, bit29:1, 
               bit30:1, bit31:1;
};

typedef struct Bitvec_piece1 Bitvec_piece;
typedef Bitvec_piece *Bitvec;

#define setbitvec(bv, i, BIT) \
switch ( (i)%32 ) {                         \
   case 0:  bv[(i)/32].bit0=BIT; break;   case 1:  bv[(i)/32].bit1=BIT; break;   \
   case 2:  bv[(i)/32].bit2=BIT; break;   case 3:  bv[(i)/32].bit3=BIT; break;   \
   case 4:  bv[(i)/32].bit4=BIT; break;   case 5:  bv[(i)/32].bit5=BIT; break;   \
   case 6:  bv[(i)/32].bit6=BIT; break;   case 7:  bv[(i)/32].bit7=BIT; break;   \
   case 8:  bv[(i)/32].bit8=BIT; break;   case 9:  bv[(i)/32].bit9=BIT; break;   \
   case 10:  bv[(i)/32].bit10=BIT; break; case 11:  bv[(i)/32].bit11=BIT; break;   \
   case 12:  bv[(i)/32].bit12=BIT; break; case 13:  bv[(i)/32].bit13=BIT; break;   \
   case 14:  bv[(i)/32].bit14=BIT; break; case 15:  bv[(i)/32].bit15=BIT; break;   \
   case 16:  bv[(i)/32].bit16=BIT; break; case 17:  bv[(i)/32].bit17=BIT; break;   \
   case 18:  bv[(i)/32].bit18=BIT; break; case 19:  bv[(i)/32].bit19=BIT; break;   \
   case 20:  bv[(i)/32].bit20=BIT; break; case 21:  bv[(i)/32].bit21=BIT; break;   \
   case 22:  bv[(i)/32].bit22=BIT; break; case 23:  bv[(i)/32].bit23=BIT; break;   \
   case 24:  bv[(i)/32].bit24=BIT; break; case 25:  bv[(i)/32].bit25=BIT; break;   \
   case 26:  bv[(i)/32].bit26=BIT; break; case 27:  bv[(i)/32].bit27=BIT; break;   \
   case 28:  bv[(i)/32].bit28=BIT; break; case 29:  bv[(i)/32].bit29=BIT; break;   \
   case 30:  bv[(i)/32].bit30=BIT; break; case 31:  bv[(i)/32].bit31=BIT; break;   \
}                     


#define getbitvec(bv, i, BIT) \
switch ( (i)%32 ) {                                                                    \
   case 0: BIT =   bv[(i)/32].bit0; break;    case 1: BIT =   bv[(i)/32].bit1; break;   \
   case 2: BIT =   bv[(i)/32].bit2; break;    case 3: BIT =   bv[(i)/32].bit3; break;   \
   case 4: BIT =   bv[(i)/32].bit4; break;    case 5: BIT =   bv[(i)/32].bit5; break;   \
   case 6: BIT =   bv[(i)/32].bit6; break;    case 7: BIT =   bv[(i)/32].bit7; break;   \
   case 8: BIT =   bv[(i)/32].bit8; break;    case 9: BIT =   bv[(i)/32].bit9; break;   \
   case 10: BIT =   bv[(i)/32].bit10; break;  case 11: BIT =   bv[(i)/32].bit11; break;   \
   case 12: BIT =   bv[(i)/32].bit12; break;  case 13: BIT =   bv[(i)/32].bit13; break;   \
   case 14: BIT =   bv[(i)/32].bit14; break;  case 15: BIT =   bv[(i)/32].bit15; break;   \
   case 16: BIT =   bv[(i)/32].bit16; break;  case 17: BIT =   bv[(i)/32].bit17; break;   \
   case 18: BIT =   bv[(i)/32].bit18; break;  case 19: BIT =   bv[(i)/32].bit19; break;   \
   case 20: BIT =   bv[(i)/32].bit20; break;  case 21: BIT =   bv[(i)/32].bit21; break;   \
   case 22: BIT =   bv[(i)/32].bit22; break;  case 23: BIT =   bv[(i)/32].bit23; break;   \
   case 24: BIT =   bv[(i)/32].bit24; break;  case 25: BIT =   bv[(i)/32].bit25; break;   \
   case 26: BIT =   bv[(i)/32].bit26; break;  case 27: BIT =   bv[(i)/32].bit27; break;   \
   case 28: BIT =   bv[(i)/32].bit28; break;  case 29: BIT =   bv[(i)/32].bit29; break;   \
   case 30: BIT =   bv[(i)/32].bit30; break;  case 31: BIT =   bv[(i)/32].bit31; break;   \
}                     

int  alocbitvec(Bitvec *bv, int n);

int  count_bitvec(Bitvec bv, int n);


/************** Min, max, fract. part, harm. mean  ******************/

#define min2(x,y) ( (x) < (y) ) ? (x) : (y)  
#define max2(x,y) ( (x) > (y) ) ? (x) : (y)  
#define fractional_part(x)  min2(  ((x) - floor((x)) ), (1 - (x) + floor((x))) )
#define harm_mean2(x,y) ( (x)*(y) )/ ( (x)+(y) )
#define harm_mean3(x,y,z) ( (x)*(y)*(z) )/ ( (x)*(y)+(x)*(z)+(y)*(z) )
#define harm_mean4(x,y,z,u) ( (x)*(y)*(z)*(u) )/ ( (x)*(y)*(u)+(x)*(z)*(u)+(y)*(z)*(u)+(x)*(y)*(z) )
#define schar signed char 

/*********************** Structures ************************/



/* beg, ind, val:  Sparse CPLEX-like structure for storing
 * the nonzeros only. 
 * delcol: Indicator of deleted columns; NULL, if none.
 * compr: Indicator, whether the storage of the deleted 
 * columns is compressed; if yes, they are not even stored
 * in the {beg, ind, val} structure. */
struct Matrix1 { int cols;
		 int rows;
                 int nonz;
                 int *beg;   
		 int *ind;   
		 double *val;
                 Bitvec delcol; 
                 char compr;  
};
typedef struct Matrix1 Matrix; 



struct Disj1 { 
  int type;
  double greedy_qual;
  double strong_qual;
  int terms;
  Matrix *B;
  double **d;
  double **h;
  double **slackBd;
};
typedef struct Disj1 Disj;


struct Disj_data1 { 
  int type;
  int choice;
  int nrequired;
  int navail;
  int nstrong;
  int nfeas;
  int ntry;
  int nmintry;
  int start;
  int end;
};
typedef struct Disj_data1 Disj_data;

int Allocdisj( Disj **disj_pp, int ndisj,
	       int nq,int q,   int nrow,int row,  
	       int nnonz,int nonz);

int ReAllocdisj( Disj **disj_pp, int ndisj,
		 int nq,int q,   int nrow,int row,  
		 int nnonz,int nonz);

int Allocdisjp( Disj ***disj_ppp, int n);

int Freedisj(Disj *disj_p, int n);

/*********************** Sorting ****************************************/
#if 0
  // We don't need these, I think...
void bubblesort (char *vec,
		 int size,
		 int n,
		 char job, /* = 'i', if increasing; 'd', if decreasing */
		 int (*cmp)(char *, char *),
		 int k,
		 ...);

void sort2 (char *vec,
	    int size,
	    int n,
	    char job, /* = 'i', if increasing; 'd', if decreasing */
	    int (*cmp)(char *, char *),
	    int k,
	    ...);


#define memswap(ptr1, ptr2, temp, size)    memcpy((char *) temp, (char *) ptr1, size); \
                                       memcpy((char *) ptr1, (char *) ptr2, size);   \
                                       memcpy((char *) ptr2, (char *) temp, size);

int comp_double(char *arg1, char *arg2);




/************************** Parsing and reading data ******************************/

#define next_token(String, s_beg, s_end, pattern)   s_beg=(s_end+1)+strspn(String+(s_end+1), pattern); \
                                               s_end=s_beg+strcspn(String+s_beg, pattern)-1;

int strcmp_by_word(char *s0, char *s1);

int  Parse_and_Read_line(FILE *infile, 
			 char *pattern, 
			 int job, 
			 char type, 
			 void *ptr,
			 int list_length,
			 ...);


#endif


/*********************** Printing ****************************************/

void Printivec(FILE *file, 
	       char *vecstr,
	       int *x,
	       int n,
	       int ijob);

void Printbitvec(FILE *file, 
		 char *vecstr,
		 Bitvec x,
		 int n);

void Printscvec(FILE *file, 
	       char *vecstr,
	       signed char *x,
	       int n,
	       int ijob);

void Printdvec(FILE *file,  
	       char *vecstr,
	       double *x,   
	       int n,
	       int ijob);


void Print_Matrix(Matrix *M, 
		  char *matrixstr,
		  FILE *file);


  // From lcchoice.h

  void liftchoice(double *,
		  int *,
		  int *,
		  int *,
		  char *,
		  double *,
		  double *,
		  int);

  void cutchoice(double *,
		 int *,
		 int *,
		 char *,
		 double *,
		 double *,
		 int);

  double frac_part(double);


  /* From alocation.h */

  typedef unsigned char boolean;

  int alocc(char **,
	    int);

  int alocsc(signed char **,
	     int);

  int aloci(int **,
	    int);

  int alocbo(boolean **,
	     int);

  int alocd(double **,
	    int);

  int alocdp(double ***,
	     int);

  int alocip(int ***,
	     int);

  
  int aloccp(char ***,
	     int);
  

  void chkerr( int );

  /// Jeff added this one
  void checkerror();

  // Stuff from main and global variables in Seb's code

  int max_fractional;
  int numsol;
  SOLUTION *fractional;


};

#endif // XPRESS
#endif // JEFF (commented out all useless cut generator information)
  
/*** FUNCTIONS/PROTOTYPES ***/
  
#endif /* __CUTGEN_H */

/*** END OF FILE ***/
  
  
