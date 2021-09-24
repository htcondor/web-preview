
// This header file is for "PARINO" compatability.  Maybe it should
//  be cleaned up and renamed?

#ifndef PRINO_H
#define PRINO_H

#include <iostream.h>
#include <stdlib.h>

// Forward declaration
class MWRMComm;

#ifndef UNDEFINED
#define UNDEFINED -1
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef EPSILON
#define EPSILON 1.0e-6
#endif

/*** MACROS ***/
#include <math.h>
#define ABS( x )      ( ( x ) < 0 ? -( x ) : ( x ) )
#define SMALL( x )    ( ABS( x ) < EPSILON )
#define FLOOR( x )    floor( (double) ( x ) )
#define CEIL( x )     ceil( (double) ( x ) )
#define INTEGRAL( x ) ( SMALL( ( x ) - FLOOR( x ) ) || \
                        SMALL( CEIL( x ) - ( x ) ) )

// for PARINO compatability
#define NEWV(aclass, arr_sz) (new aclass[arr_sz])
#define DELETEV(aptr) delete [] (aptr)
#if ! defined( MIN )
#define MIN( a, b )   ( ( (a) < (b) ) ? (a) : (b) )
#endif

#define PRINT( ofile, str, arr, n, max_n )  do { \
    (ofile) << (str) << ": ( " << (n) << " ) = ["; \
    if( !(arr) ) \
    { \
        (ofile) << "NULL"; \
    } \
    else \
    { \
        for( int k = 0; k < (n); k++ ) \
        { \
            if( k % (max_n) == 0 ) (ofile) << endl << "    "; \
            (ofile) << (arr)[k] << " "; \
        } \
    } \
    (ofile) << endl << "]" << endl; \
} while( 0 )


// Types of Columns
typedef int CColType;
enum
{
    COL_TYPE_INVALID,
    COL_TYPE_BINARY,
    COL_TYPE_INTEGER,
    COL_TYPE_CONTINUOUS
};

// Types of Rows
typedef int CRowType;
enum
{
    ROW_TYPE_INVALID,
    ROW_TYPE_MIXUB,
    ROW_TYPE_MIXEQ,
    ROW_TYPE_NOBINUB,
    ROW_TYPE_NOBINEQ,
    ROW_TYPE_ALLBINUB,
    ROW_TYPE_ALLBINEQ,
    ROW_TYPE_SUMVARUB,
    ROW_TYPE_SUMVAREQ,
    ROW_TYPE_VARUB,
    ROW_TYPE_VAREQ,
    ROW_TYPE_VARLB,
    ROW_TYPE_BINSUMVARUB,
    ROW_TYPE_BINSUMVAREQ,
    ROW_TYPE_BINSUM1VARUB,
    ROW_TYPE_BINSUM1VAREQ,
    ROW_TYPE_BINSUM1UB,
    ROW_TYPE_BINSUM1EQ,
    ROW_TYPE_BINSUM1LB,
    ROW_TYPE_BINSUM1UBK,
    ROW_TYPE_BINSUM1EQK,
    ROW_TYPE_UNCLASSIFIED
};

// ### qun
//whether one row is infeasible, or redaundant, or changed
//or unchanged
//for preprocessing 
enum ROWSTATE {INFEASIBLE, REDUNDANT, CHANGED, UNCHANGED};

/****************************************************************************/
class CVLB
{
 public:
  CVLB( void ) : var(-1), val(-1.0) {}
  
 public:
  int get_var( void ) const { return var; }
  double get_val( void ) const { return val; }

 public:
  void set_var( int v ) { var = v; }
  void set_val( double v ) { val = v; }

 public:
  //frined ostream &operator<<( ostream &o, const CVLB& vl  );
  
  void print(){
    cout << " VAR= " << var << " VAL= " << val <<"    "<<endl;
  }    
  
  /*
    public:
    CData &operator>>( CData &data ) const { return data << var << val;}
    CData &operator<<( CData &data ) { return data >> var >> val;}
    ostream &operator>>( ostream &o ) const
    { return o << " VAR= " << var << " VAL= " << val; }
    
  */
 protected:
  int var;            /* ind of associated 0-1 variable */
  double val;            /* value of associated bound */
};


/*
ostream &operator<<( ostream &o, const CVLB& vl  )
{
o<<" VAR= " << vl.var << " VAL= " << vl.val;
  return o;
  }
*/


class CVUB : public CVLB
{
};


#define ACTIVE 1
#define INACTIVE 0
struct CGUBInfo
{
    int nzcnt;                /* number of nonzero coefficients */
    int *ind;                 /* indices of the nonzero coefficients */
    int *val;                 /* values of the nonzero coefficients */
    char sense;               /* sense of the GUB constraint */
    int rhs;                  /* rhs of the GUB constraint */
    int status;               /* status: ACTIVE or INACTIVE */
    int lpix;                 /* LP solver index */
    CGUBInfo( void ) :
      nzcnt(0), ind(0), val(0), sense(0), rhs(-1), status(INACTIVE), lpix(0) {}
    ~CGUBInfo() { 
      delete [] ind;
      ind = 0;
      
      delete [] val;
      val = 0;
    }
};

struct CGUBTable
{
    int cnt;             /* number of GUB constraints */
    CGUBInfo **gub;      /* array of pointers to GUB constraints */
    CGUBTable( void ) : cnt(0), gub(0) {}
    ~CGUBTable()
    {
      // cherr << "CGUBTable: deletion to be completed.!!!" << endl;
      // DELETEV( gub ); //XXX To be fixed -- must delete the matrix, not
			//XXX just the vector
    }
};

//-----------------------------------------------------------------------------
class CLPSolution
{
public:
  int get_nvars( void ) const { return nvars; }
  int get_nrows( void ) const { return nrows; }
  const double *get_xlp( void ) const { return xlp; }
  double get_zlp( void ) const { return zlp; }
  const double *get_pi( void ) const { return pi; }
  const double *get_slack( void ) const { return slack; }
  const double *get_dj( void ) const { return dj; }
  const int *get_cstat( void ) const { return cstat; }
  const int *get_rstat( void ) const { return rstat; }

  double *acc_xlp( void ) { return xlp; }
  double *acc_pi( void ) { return pi; }
  double *acc_slack( void ) { return slack; }
  double *acc_dj( void ) { return dj; }
  int *acc_cstat( void ) { return cstat; }
  int *acc_rstat( void ) { return rstat; }
  
public:
  CLPSolution( void ) : nvars( UNDEFINED ), nrows( UNDEFINED ), xlp( NULL ),
    pi( NULL ), slack( NULL ), dj( NULL ),
    cstat( NULL ), rstat( NULL ) { }

  CLPSolution( int n ) : nrows( -1 ), xlp( NULL ), 
      pi( NULL ), slack( NULL ), 
      dj( NULL ), cstat( NULL ), rstat( NULL ) 
    { nvars = n; }

  CLPSolution( int n, const double v[] ) : nrows( UNDEFINED ), pi( NULL ), 
      slack( NULL ), dj( NULL ), cstat( NULL ), rstat( NULL ) 
  { 
    nvars = n; 
    // Make your own copy
    xlp = new double [nvars];
    for( int i = 0; i < n; i++ )
      xlp[i] = v[i];
  }

  CLPSolution( int n, const double v[], const double v2[] ) : 
    nrows( UNDEFINED ), pi( NULL ), slack( NULL ), cstat( NULL ), 
    rstat( NULL ) 
    { 
    nvars = n; 
    xlp = new double [nvars];
    dj = new double [nvars];
    for( int i = 0; i < nvars; i++ )
      {	
	xlp[i] = v[i]; 
	dj[i] = v2[i];
      } 
  }

  CLPSolution( int n, int m, const double tx[], const double tpi[],
	       const double tslack[], const double tdj[] ) : 
    cstat( NULL ), rstat( NULL ) 
  { 
    nvars = n; 
    nrows = m;
    if( tx != NULL )
      xlp = NEWV( double, nvars );
    else
      xlp = NULL;
    if( tpi != NULL )
      pi = NEWV( double, nrows );
    else
      pi = NULL;
    if( tslack != NULL )
      slack = NEWV( double, nrows );
    else
      slack = NULL;
    if( tdj != NULL )	
      dj = NEWV( double, nvars );
    else
      dj = NULL;

    for( int i = 0; i < nvars; i++ )
      {	
	if( xlp ) xlp[i] = tx[i]; 
	if( dj ) dj[i] = tdj[i];
      } 

    for( int i = 0; i < nrows; i++ )
      {	
	if( pi ) pi[i] = tpi[i]; 
	if( slack ) slack[i] = tslack[i]; 
      } 
  }

  ~CLPSolution() 
  {     
    DELETEV( xlp ); 
    DELETEV( pi );
    DELETEV( slack ); 
    DELETEV( dj );
    DELETEV( cstat ); 
    DELETEV( rstat );    
  }

  ostream &operator>>( ostream &o ) const
  { PRINT( o, "LPSolution", xlp, nvars, 10 ); return o; }

  void init( int nc, int nr, int max_cuts )
  {
    nvars = nc;
    nrows = nr;

    xlp = NEWV( double, nvars );
    pi = NEWV( double, nrows + max_cuts );
    slack = NEWV( double, nrows + max_cuts );
    dj = NEWV( double, nvars );
    cstat = NEWV( int, nvars );
    rstat = NEWV( int, nrows + max_cuts );
  }

private:
  int nvars;
  int nrows;
  double *xlp; // array of size nvars
  double zlp;  // objective value
  double *pi;  // dual LP solution
  double *slack; // primal slack
  double *dj;  // reduced costs
  int *cstat;  // CPLEX column status
  int *rstat;  // CPLEX row status

  
};

//-----------------------------------------------------------------------------

/****************************************************************************/
class CMatrix
{
//###new public interface
public:
  CMatrix(int n, int m, int nz, int* beg, int* cnt,int* ind, double* val):
    ncols(n), nrows(m), matvalsz(nz), matbeg(beg), matcnt(cnt),
    matind(ind), matval(val) {};
      
public: // Read-only access
  int get_ncols( void ) const { return ncols; }
  int get_nrows( void ) const { return nrows; }
  int get_nnzero( void ) const { return matvalsz; }
  int get_matvalsz( void ) const { return matvalsz; }
  const int *get_matbeg( void ) const { return matbeg; }
  const int *get_matcnt( void ) const { return matcnt; }
  const int *get_matind( void ) const { return matind; }
  const double *get_matval( void ) const { return matval; }


public: //access
  int *acc_matbeg( void )  { return matbeg; }
  int *acc_matcnt( void )  { return matcnt; }
  int *acc_matind( void )  { return matind; }
  double *acc_matval( void )  { return matval; }

  
protected:
  CMatrix( void ) :
    ncols(0), nrows(0), matvalsz(0), matbeg(0), matcnt(0),
    matind(0), matval(0) {}
  virtual ~CMatrix()
  {
    delete [] matbeg; 
    //free(matbeg);
    matbeg = 0;

    delete [] matcnt;
    //free(matcnt);
    matcnt = 0;

    //free(matind);
    delete [] matind;
    matind = 0;    

    //free(matval);
    delete [] matval;
    matval = 0;
  }

public:
  void init_matrix( int nc, int nr, int nv,
		    int *mb, int *mc, int *mi, double *mv )
  {
    if( ncols > 0 || nrows > 0 || matbeg )
      cerr << "Error: CMatrix::init_matrix() already inited" << endl;
    
    ncols = nc; nrows = nr; matvalsz = nv;
    matbeg = mb; matcnt = mc; matind = mi; matval = mv;
  }

protected:
  int     ncols;
  int     nrows;
  int     matvalsz; // Size of matval or matind
  int    *matbeg;
  int    *matcnt;
  int    *matind;
  double *matval;
};

/****************************************************************************/
class CColMajorMatrix; // Forward decl
class CRowMajorMatrix; // Forward decl

//---------------------------------------------------------------------------
class CColMajorMatrix : public CMatrix
{
//###new public interface
public:
  CColMajorMatrix(int n, int m, int nz, int* beg, int* cnt,
		  int* ind, double* val, CColType* ct,
		  double* obj, double* lb, double* ub):
    CMatrix(n,m,nz,beg,cnt,ind,val),ctype(ct),objx(obj),
    bdl(lb),bdu(ub){};


public:
  CColMajorMatrix( void ) :
    ctype(0), objx(0), bdl(0), bdu(0) {}
  ~CColMajorMatrix()
  { 
    //free(ctype);
    delete [] ctype;
    ctype = 0;

    //free(objx);
    delete [] objx;
    objx = 0;

    //free(bdl);
    delete [] bdl;
    bdl = 0;

    //free(bdu);
    delete [] bdu;
    bdu = 0;
  }

public:
  void init( int nc, int nr, int nv,
	     int alloc_mem=1,
	     int *mb=0, int *mc=0, int *mi=0, double *mv=0,
	     CColType *ct=0, double *ob=0, double *l=0, double *u=0 );

public:
  int get_nrows() const { return nrows;};
  int get_ncols() const { return ncols;};
      

public:
  void negate_objx( void )
  { if( objx ) for( int i = 0; i < get_ncols(); i++ ) objx[i] *= -1; }
public:
  const CColType *get_ctype( void ) const { return ctype; }
  const double *get_objx( void ) const { return objx; }
  const double *get_bdl( void ) const { return bdl; }
  const double *get_bdu( void ) const { return bdu; }

public: //XXX When time permits, try to avoid the write-access methods.
  double *acc_bdl( void ) { return bdl; }
  double *acc_bdu( void ) { return bdu; }

public:
  void classify( void );
  void preprocess( const char senx[], bool row_redundant[] );
  void convert( const CRowMajorMatrix *rowmajor );

  ostream &operator>>( ostream &out ) const;

protected:
  CColType *ctype;
  double *objx;
  double *bdl;
  double *bdu;


 public:
  // For passing
  void MWPack( MWRMComm * );
  void MWUnpack( MWRMComm * );

  void write( FILE * );
  void read( FILE * );

};

//---------------------------------------------------------------------------
class CRowMajorMatrix : public CMatrix
{
  //###new public interface
public:
  CRowMajorMatrix(int n, int m, int nz, int* beg, int* cnt,
		  int* ind, double* val, CRowType* rt,
		  double* rhs, char* s):
    CMatrix(n,m,nz,beg,cnt,ind,val),rtype(rt), rhsx(rhs), senx(s){};
  
public:
  CRowMajorMatrix( void ) :
    rtype(0), rhsx(0), senx(0) {}
  ~CRowMajorMatrix()
    {
      //free(rtype);
      delete [] rtype;
      rtype = 0;

      //free(rhsx);
      delete [] rhsx;
      rhsx = 0;

      //free(senx);
      delete [] senx;
      senx = 0;
    }
  
public:
  void init( int nc, int nr, int nv,
	     int alloc_mem=1,
	     int *mb=0, int *mc=0, int *mi=0, double *mv=0,
	     CRowType *rt=0, double *rh=0, char *sn=0 );

public:
  const CRowType *get_rtype( void ) const { return rtype; }
  const double *get_rhsx( void ) const { return rhsx; }
  const char *get_senx( void ) const { return senx; }


public: //access
  double *acc_rhsx( void )  { return rhsx; }

  
public:
  void classify( const CColType ctype[] );
  //void preprocess( const CColType ctype[], double *bdl, double *bdu );
  void convert( const CColMajorMatrix *colmajor );

  ostream &operator>>( ostream &out ) const;

  //### qun
  //added for presolve
  int preprocess( const CColType ctype[], double *bdl, double *bdu, int root);

  ROWSTATE process_one_row(int root,
			   const int& i,
			   int& row_cnt, char row_sense, 
			   double& row_rhsx,
			   int* row_ind, double* row_val,
			   const CColType* ctype, 
			   double* bdl, double* bdu);
  void Flip (int nzcnt, 
	     int *rind, double *rval, 
	     char *sense, double *rhs);

private:
  void del_empty_rows( void );

protected:
  CRowType *rtype;
  double *rhsx;
  char *senx;


 public:
  //Qun add for parallel program
  void MWPack( MWRMComm * );
  void MWUnpack( MWRMComm * );

  //Qun: for preprocessing
  void compact();  

  // For checkpointing
  void write( FILE * );
  void read( FILE * );
};

//--------------------------------------------------------------------------- 
/****************************************************************************/
class CProblemSpec
{

//###new public interface
public:
  CProblemSpec( int objsense, int n, int m, int nz, int matbeg[],
		int matind[], int matcnt[], double matval[], 
		CColType ctype[], double obj[], double lb[],
		double ub[], double rhs[], char sen[] );
  
  //copy constructor
  CProblemSpec( const CProblemSpec&);
  
  void copy ( const CProblemSpec& );

public:
  CProblemSpec( void ) :
    objsen(-1), vlb(0), vub(0) {};
  
  ~CProblemSpec();
  
  //  { if(vlb) DELETEV( vlb ); if(vub) DELETEV( vub ); };
  

public:
  void flip_maxmin( void ) { objsen *= -1; colmajor.negate_objx(); }

public:
  int read_mps( char *fname );
  int preprocess( int *nbc, int *ncc, int *nrr, int root = 1 );


public: // Read-only access
  int get_nvars( void ) const { return colmajor.get_ncols(); }
  int get_ncols( void ) const { return colmajor.get_ncols(); }
  int get_nrows( void ) const { return colmajor.get_nrows(); }
  int get_nnzero( void ) const { return colmajor.get_nnzero(); }  
  int get_objsen( void ) const { return objsen; }
  const double *get_objx( void ) const { return colmajor.get_objx(); }
  const double *get_rhsx( void ) const { return rowmajor.get_rhsx(); }
  const char   *get_senx( void ) const { return rowmajor.get_senx(); }
  const double *get_bdl( void ) const { return colmajor.get_bdl(); }
  const double *get_bdu( void ) const { return colmajor.get_bdu(); }
  const CColMajorMatrix *get_colmajor( void ) const { return &colmajor; }
  const CRowMajorMatrix *get_rowmajor( void ) const { return &rowmajor; }
  const CVLB *get_vlb( void ) const { return vlb; }
  const CVUB *get_vub( void ) const { return vub; }
  const CGUBTable *get_gubtable( void ) const { return &gubtable; }
  
public: // Write access
  double *acc_bdl( void ) { return colmajor.acc_bdl(); }
  double *acc_bdu( void ) { return colmajor.acc_bdu(); }

  ostream &operator>>( ostream &out ) const;


  /// Returns true if xlp is an (integer) feasible solution
  bool feasibleSolution( const double *xlp );

  /// Number of integer variables
  int nInts;
  
  /// Array containing indices of integer variables
  int *intConsIndex;
 
protected:
  int       objsen; // Object sense: Max or Min?
  CColMajorMatrix colmajor;
  CRowMajorMatrix rowmajor;
  CVLB      *vlb;
  CVUB      *vub;
  CGUBTable gubtable;

private:

  void colmajor_to_rowmajor( int ncols,
			   const int colbeg[], const int colind[],
			   const double colval[],
			   int nrows, int rowbeg[], int rowind[],
			   double rowval[],
			   int rowcnt[], int nvals );

  void setupVarBounds(CVLB* vlb, CVUB* vub, int n, int m, 
		      const int row_matbeg[], 
		      const int row_matind[], const double row_matval[],
		      const int row_matcnt[], const CRowType rtype[],
		      const CColType ctype[] );

  CRowType classify_one_row(int nzcnt, const int rind[], const double rval[], 
			  char sense, double rhs, const CColType ctype[] );

  void Flip (int nzcnt, int *rind, double *rval, char *sense, double *rhs);

  //char* request_mem(int size);
  

 public:
  //Qun add for parallel program
  void MWPack( MWRMComm * );
  void MWUnpack( MWRMComm * );

  // For checkpointing
  void write( FILE * );
  void read( FILE * );

private:  
  //Qun add for debugging preprocessing
  void print();
  void printLP();
  void printIntVec(const int* a, int n);
  void printDoubleVec(const double* d, int n);
  void printCharVec(const char* s, int n);
  int get_nfvars();

};

/****************************************************************************/

ostream &operator<<( ostream &out, const CProblemSpec &problem );
ostream &operator<<( ostream &out, const CColMajorMatrix &c );
ostream &operator<<( ostream &out, const CRowMajorMatrix &r );

#endif
