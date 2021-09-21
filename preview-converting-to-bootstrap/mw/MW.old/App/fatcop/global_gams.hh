//defined global variables and functions for gams interface

#define ANSI
#define MAXLINELENGTH  200

#include <stdio.h>
#include <ctype.h>

#include <stdlib.h>
#include <math.h>
#include <iostream.h>

#include "global.h"
#include "iolib.h" //gams io lib


int n;                  /* number of columns */
int m;                  /* number of rows */
int nz;                 /* number of nonzeroes */
int objvar; 
int objrow;            /* objective variable */
int itlim;
double reslim;
double ginf, gminf;     /* inf, -inf */
int objsen;		/* 1 : minimize, -1 : maximize */

bool isamip;
double optcRel,optcAct;
int nodlim;
int mipstate;
bool priorities;


double *xpri;
//int *ipri;
//int *imap;
//int ndisc = 0;    /* number of discrete variables i.e. nints+nososvars */
int nints = 0;     /* number of binary and integer variables */
//int lpits=0;
//int mipits=0;
//int finalits=0;
//double lpobj=0.0;
double mipobj=0.0;
//double bestobj=0.0;
//double finalobj=0.0;
//bool mipsolavail = false;
//int switch_ndsel=0;
dArray <char> ctype;
dArray <int> sign;


double *aa;             /* matval */
double *x = 0;              /* level */
double *lb, *ub;        /* bounds */
double *rhs;
int *irow;              /* matind - row index */
int *istart;            /* matbeg - start index in aa of col i */
int *icnt;              /* matcnt - count of nz in column i */
double *c;              /* cost row */
double *dj = 0;             /* reduced cost */
double *mypi = 0;           /* duals (metaware objects to pi)  */
double *slack = 0;
int* rowstat = 0;
int* colstat = 0;
//int *sign;             /* type of row: '0'-'E';'1'-'G'; '2'-'l'; '3'-free */
bool reformulated;
double  reforma;
double reformb;


double nolb = -inf;  /* - inf for SOPLEX */
double noub = inf;   /* + inf for SOPLEX */

double input_time = 0.0;
double calc_time = 0.0;

char ch;   /* needed for parsing option file */
int intvarno=0;
//bool revObj;




FATCOP_driver mipWork;


/* GAMS Codes:
   
	solver statusi		model status
	
	1.	normal completion	optimal
  2.	iteration interrupt	locally optimal
  3.	resource interrupt	unbounded
  4.	terminated by solver	infeasible
  5.	eval error limit	locally infeasible
  6.	unknown			intermediate infeasible
  7.	n.a.			intermediate nonoptimal
  8.	preproc error		integer solution
  9.	setup failure		intermediate non-integer
  10.	solver failure		integer infeasible
  11.	internal solver error	no solution
  12.	postproc. error		error unknown
  13.	error system failure	error no solution
*/



typedef struct {
	int solverstatus;
	int modelstatus;
	bool infes;
	bool unb;
	bool nopt;
	char *msg;
} STATMAP;


STATMAP mipstatmap[] = {
  
  /*	solsta	modsta	infes	unb	nopt	msg */
  
	1,	1,	false,	false,	false,	"Found proven optimal solution.\n",
	1,	4, 	true,	false,	true,	"Problem is infeasible.\n",	
	1,	3,	false,	true,	true,	"Problem is unbounded.\n",	
	1,      10,     true,   false,  true,   "Problem is integer infeasible.\n",
	2,      8,      false,  false,  true,   "Found integer solution, node limit reaches.\n",
	2,      9,      false,  false,  true,   "intermediate non-integer, node limit reaches.\n",
	1,	1,	false,	false,	true,	"Solution satisfies tolerance.\n"
};





//###################################################################
//                  FUNCTINS
//###################################################################

//define forward
//functions
void prIntroInfo(); 

void allocate_memory();
char* request_mem(int);


void readat(FILE*, FILE*);
void store_row(int, int, double);
void store_col(int , double, double, int,double);

void reformulate();

void get_options(char*);
void get_char(char**);

void loadToMipSolver();

void write_solution();
void free_memory();


void message(char*);
void error(char*);
void mapmipstat(int, int*, int*, bool*, bool*, bool*);
void mipmessage(int);

void createFinishFile();



//function definitions
void  prIntroInfo()
{
  int iolevel;
  char osname[50];
  char line[80];
  
  gfsysid(osname,&iolevel);
  sprintf(line, "iolevel-%d, %s\n",iolevel, osname);
  
  gfstct("START");
  fprintf(gfiosta,"\n");
  fprintf(gfiosta, "%s", line);
  gfstct("STOPC");
  
}



//1. allocate memory for the matrix
void allocate_memory()
{
  int i;
  
  //matrix
  irow   = (int *)    request_mem((nz)*sizeof(int));
  aa     = (double *) request_mem((nz)*sizeof(double));
  istart = (int *)    request_mem((n+1)*sizeof(int));
  icnt   = (int *)    request_mem((n)*sizeof(int));
  
  //col vector
  c      = (double *) request_mem((n)*sizeof(double));
  lb     = (double *) request_mem((n)*sizeof(double));
  ub     = (double *) request_mem((n)*sizeof(double));
  //need to convert to int* to pass to fcpsolver
  //ctype  = (char *)   request_mem(n*sizeof(char));
  ctype.reSize(n);

  //row vector
  rhs    = (double *) request_mem((m)*sizeof(double));
  //need to convert to char* to pass to fcpsolver
  //sign   = (int *)   request_mem(m*sizeof(int));
  sign.reSize(m);
  
  
  //solution
  x      = (double *) request_mem(n*sizeof(double));
  dj     = (double *) request_mem(n*sizeof(double));
  mypi   = (double *) request_mem(m*sizeof(double));
  slack  = (double *) request_mem(m*sizeof(double));
  colstat= (int *)    request_mem(n*sizeof(int));
  rowstat= (int *)    request_mem(m*sizeof(int));
  
  
  if (isamip) {
    if (nints > 0) {
      xpri = (double *) request_mem(nints*sizeof(double));
      for (i=0;i<nints;++i){
	xpri[i] = 0.0;
      }
    }
  }
  
  /* initialize some arrays */
  for (i=0;i<n;++i)
    c[i] = 0.0;
  
  
  return;
  
}


char* request_mem(int size)
{
  char *memptr;
  
  memptr = (char*) malloc(size);
  if (memptr==NULL) {
    cerr<<"NOT ENOUGH MEMORY"<<endl;
    exit(1);
  }
  return(memptr);
}




//2. read option file
void get_options(char* fln)
{
  FILE *f;
  char s[MAXLINELENGTH+1];
  char *tmp;

  f = fopen(fln,"r");
    
  if (f != NULL)                  /* file is found */
    {
      
      fprintf(gfiolog,"Reading user supplied option file...\n");
      
      fprintf(gfiosta,"Option file:\n");
      fprintf(gfiosta,"-----------\n");
      
      while(fgets(s,MAXLINELENGTH,f))
	{

	  //  printf("%s\n",s);
	  
	  fprintf(gfiosta,"> %s",s);  /* s contains \n, otherwise
					 add to %s */
	  fprintf(gfiolog,"> %s",s);
	  
	  tmp = s;
	  get_char(&tmp);         /* read ahead */
	  if (ch!='*' && ch!='\n')
	    //printf("%s\n",tmp);
	    mipWork.setOption(tmp);
	}
      
      fclose(f);
      fprintf(gfiosta,"\n\n");    /* status file must end with a newline */
    }
  else
    {
      fprintf(gfiosta,"Option file %s not found.\n",fln);
      fprintf(gfiolog,"Option file %s not found.\n",fln);
      fprintf(gfiosta,"Using defaults instead.\n");
      fprintf(gfiolog,"Using defaults instead.\n");
    }
}



void get_char(char** s)
  /* get next char, and if not at end, move pointer to next char */
{
  
  ch = **s;
  if (isascii(ch))
    if (isupper(ch))
      ch = (char) tolower(ch);
  
  return;
 
}



//3. read matrix

void readat(FILE* logfile, FILE* statusfile)
{  
  rowrec row;
  colrec col;
  int nzcol;
  int idummy;
  int nnz;
  int i,j;
  int rowindex;
  
  fprintf(logfile,"Reading data ...\n");
  
  // printf("Reading rows ...\n");

  
  for (i=0;i<m;++i)  {
    gfrrow(&row);
    store_row(i,
	      row.idata[1],   /* sign */
	      row.rdata[2]);    /* rhs */
  }
  
  
  // printf("\nReading columns ...\n");
  
  
  nnz = 0;
  for (j=0;j<n;++j) {
    gfrcol(&col);
    store_col(j,
	      col.cdata[1],            /* lb */
	      col.cdata[3],            /* ub */
	      col.idata[3],            /* variable type */ 
	      col.cdata[4]);           /* variable type */       
    
   
    nzcol = col.idata[1];
    icnt[j] = nzcol;
    istart[j] = nnz;
    
    for (i=0;i<nzcol;++i,++nnz) {
      gfrcof(&aa[nnz],&rowindex,&idummy);
      irow[nnz] = rowindex-1;
    }
    
  }
  
  c[objvar] = 1.0;
  
  printf("\nProblem is loaded\n");
  
  return;
}



void store_row(int row, int type, double b)
{
  if ((row<0)||(row>=m)){
    cerr<<"Invalid row number in store_row"<<endl;
    exit(1);
  }
    
  sign[row] = type;
  rhs[row] = b;
  
  return;
  
}


void store_col(int col, double lo, double up, int coltype, double prior)
{
  if ((col<0)||(col>=n)){
    cerr<<"Invalid column number in store_col"<<endl;
    exit(1);
  }

  
  if (lo==gminf)    lb[col] = nolb;
  else    lb[col] = lo;
  
  if (up==ginf)    ub[col] = noub;
  else    ub[col] = up;
  

  switch(coltype) {
    
  case 0 : case 3: case 4:/* continuous */
    ctype[col] = 'C'; 
    break;  
    
  case 1 : /* binary */
    if (lb[col] == ub[col])
      ctype[col] = 'C';
    else {
      ctype[col] = 'B';

      if (priorities) {
	assert(intvarno<nints);
	xpri[intvarno] = prior;
	++intvarno;
      }
    }
    break;
    
  case 2 : /* integer */
    if (lb[col] == ub[col])
      ctype[col] = 'C';
    else {
      
      ctype[col] = 'I';
      if (priorities) {
	assert(intvarno<nints);
	xpri[intvarno] = prior;
	++intvarno;
      }  
    }
    
    break;
  }

  return;
}


//5. reformulate the problem
void reformulate()
{
  /* try to do a one step reformulation*/
  
  int k;
  int knew, jnew;
  int i,j;
  int kstart, kend;
  int newcnt;
  
  /* assume we fail */
  reformulated = FALSE;
  
  
  /* objective variable has to be a free variable */
  if ( (lb[objvar]!=nolb) || (ub[objvar]!=noub) )
    return;
  
  
  /* objective column should have length 1 */
  if (icnt[objvar]!=1) 
    return;
  
  /* objective row should be an equality */
  k =  istart[objvar];
  objrow = irow[k];
  
  if (sign[objrow] != 0) 
    return;
  /* if gams obj row was formulated like
   *     az + c'x =e= b
   * then the new objective will look like:
   *    (b-c'x)/a
   * so objective coefficients are -c[i]/a
   */
  
  /*
   * REFORMA is (-1/a)
   * REFORMB is b/a
   * True value objective variable is REFORMB+OBJ
   * which is in most cases OBJ!
   */
  reforma = -1.0/aa[k];
  reformb = rhs[objrow]/aa[k];
  
  c[objvar] = 0.0;
  
  /* 
   * now loop over the whole matrix, extracting
   * the GAMS objective. We compress at the same time.
   */
  knew = 0;
  for (j=0, jnew=0; j < n; ++j, ++jnew) {
    if (j==objvar)
      --jnew;
    else {
      kstart = istart[j];
      kend = kstart+icnt[j];
      istart[jnew] = knew;
      newcnt=0;
      for (k=kstart;k<kend;++k) {
	i = irow[k];
	if (i==objrow) 
	  c[jnew] = reforma*aa[k];
	else {
	  aa[knew] = aa[k];
	  if (i < objrow)
	    irow[knew] = i;
	  else
	    irow[knew] = i-1;
	  ++knew;++newcnt;
	}
      }
      if (jnew!=j) {
	lb[jnew] = lb[j];
	ub[jnew] = ub[j];
      }
      icnt[jnew] = newcnt;
    }
  }
  for (i=objrow+1;i<m;++i) {
    rhs[i-1] = rhs[i];
    sign[i-1] = sign[i];
    rowstat[i-1] = rowstat[i];
  }
  
  
  ctype.remove(objvar);
  
  
  --n;
  --m;
  nz = knew;
  reformulated = TRUE;
  
  return;

}



//6. load the problem to mip solver
void loadToMipSolver()
{
  int i;
  
  int* coltype = new int[n];
  char* senx = (char *)   request_mem(m*sizeof(char));
  
  for(i=0; i<n; i++){
    if(ctype[i] == 'C'){
      coltype[i] = COL_TYPE_CONTINUOUS;
    }
    else if(ctype[i] == 'I'){
      coltype[i] = COL_TYPE_INTEGER;
    }
    else{
      coltype[i] = COL_TYPE_BINARY;
    }
  }
  ctype.clear(); //take back memory

  
  for(i=0; i<m; i++){
    if(sign[i] == 0){
      senx[i] = 'E';
    }
    else if(sign[i] == 1){
      senx[i] = 'G';
    }
    else{
      senx[i] = 'L';
    }
  }
  //sign.clear(); //take back memory



  
  istart[n] = nz;
  
  mipWork.load(objsen, n, m, nz,
	       istart, irow, icnt, aa,
	       coltype, c, lb, ub, 
	       rhs, senx);
  
  return;
}




//9. write solutions
void write_solution()
{
  int solstat, modstat;
  int colindic, rowindic;
  int row,col,itsusd,nodes;
  double level, dual, rhsvalue, slck, infeas;
  double tol = 1e-6;
  double lbd, ubd;
  int eqstat, clstat;
  char line[80];
  bool infes_found = FALSE;
  int nodusd;
  bool infes, unb, nonopt;
  int unbrow = -1;
  int unbcol = -1;
  int isunb;
  bool gotbasis = FALSE;
  double bestpos, relgap;
  
 
 
  //printf("Write solution\n");

  
  FATCOP_driver::Status    stat = mipWork.status() ;
 
  switch( stat )
    {
    case FATCOP_driver::SOLVED:
      mipstate=1;
      mapmipstat(mipstate,&solstat,&modstat,&infes,&unb,&nonopt);
      break ;
    case FATCOP_driver::INFEASIBLE:
      mipstate=2;
      mapmipstat(mipstate,&solstat,&modstat,&infes,&unb,&nonopt);
      break ;
    case FATCOP_driver::UNBOUNDED:
      mipstate=3;
      mapmipstat(mipstate,&solstat,&modstat,&infes,&unb,&nonopt);
      break ;
    case  FATCOP_driver::INT_INFEASIBLE:
      mipstate=4;
      mapmipstat(mipstate,&solstat,&modstat,&infes,&unb,&nonopt);
      break ;
    case FATCOP_driver::INT_SOLUTION:
      mipstate=5;
      mapmipstat(mipstate,&solstat,&modstat,&infes,&unb,&nonopt);
      break ;
    case FATCOP_driver::NO_INT_SOLUTION:
      mipstate=6;
      mapmipstat(mipstate,&solstat,&modstat,&infes,&unb,&nonopt);
      break ;  
    default:
      //      mapmipstat(5,&solstat,&modstat,&infes,&unb,&nonopt);
      error("solver error\n");
      break ;
    }
  
  
  
  fprintf(gfiolog,"\n");
  gfstct("START");
  

  mipmessage(mipstate);
  
  itsusd =  mipWork.pivots();
  mipobj = mipWork.value();
  nodes = mipWork.nodes();
  bestpos = mipWork.getBestPos();
  relgap = mipWork.getFinalGap();
  
  
  if (mipstate==1){
    sprintf(line,"MIP Solution  :  %18.6f    (%d iterations, %d nodes)",
	    mipobj,itsusd,nodes);
    message(line);
    sprintf(line,"Best integer solution possible  :  %18.6f",
	    bestpos);
    message(line);
    sprintf(line,"Relative gap  :  %18.6f",
	    relgap);
    message(line);
  }
  if (mipstate==5){  
    sprintf(line,"Best integer solution possible : %18.6f",mipobj);
    message(line);
  }
  
  if (reformulated) 
    if (reformb != 0.0) {
      fprintf(gfiolog,"After adding constant term, objective is %18.6f\n",mipobj+reformb);
      fprintf(gfiosta,"After adding constant term, objective is %18.6f\n",mipobj+reformb);
    }
  
 
  gfwsta(modstat,solstat,itsusd,gfclck(),mipobj+reformb,0);
  
 
  //  if(FATCOP_driver::INT_SOLUTION == stat || FATCOP_driver::SOLVED == stat){

  //  printf("Writing rows\n");
    
    
    for (row=0;row<m+1;++row)
      {
	
	if (reformulated) 
	  if (row==objrow) {   /* insert objective row here */
	    level = -reformb/reforma;
	    dual = -reforma;
	    rowindic = 0;
	    eqstat = 0;
	    gfwrow(level, dual, rowindic, eqstat);
	    // cout<<" ****level "<<level<<" mypi "<<mypi[row]<<" rowindic "<<rowindic<<" eqstat "<<eqstat<<endl;
	  }
	if (row>=m) 
	  continue;
	
	/* ask for rhs and lhs in order to calc the row level !!!!wrong calculation*/
	/*  rhsvalue = lp_1.rhs(row);
	    if(sign[row]==0) level=rhsvalue;
	    if(sign[row]==2) level= rhsvalue-mypi[row];
	    if(sign[row]==1) level=lp_1.lhs(row) - mypi[row];*/

	level = slack[row];
	
	
	
	if(rowstat[row] == 1) //basic
	  {
	    rowindic = 2;
	  }
	else{
	  switch (sign[row]) {
	  case  1 : rowindic = 0; break;
	  case  2 : rowindic = 1; break;
	  case  0 : rowindic = 0; break;
	  default :
	    error("Illegal value in sign\n");
	  }
	}
	
	eqstat = 0;
	
	/* first check if row is unbounded, SOPLEX can not do that*/
	
	
	/* if not then maybe infeasible */
	if (infes) {
	  slck = slack[row];
	  infeas = 0.0;
	  
	  switch (sign[row]) {
	  case 1 :
	    if (slck>0.0) infeas = slck;
	    break;
	  case 2 :
	    infeas = fabs(slck);
	    break;
	  case 0 :
	    if (slck<0.0) infeas = -slck;
	    break;
	  } /* switch */
	  
	  if (infeas > tol) {
	    eqstat = 2;  /* infeasible */
	    infes_found = TRUE;
	  }
	}
	
	/* if not then maybe non-optimal */
	if (nonopt && (eqstat==0)) {
	  switch (objsen) {
	    
	  case 1 :    /* minimization */
	    
	    if ((rowindic==0)&&(mypi[row]< -tol))
	      /* at lowerbound and pi<0 */
	      eqstat = 1;
	    else if ((rowindic==1)&&(mypi[row]>tol))
	      /* at upperbound and pi>0 */
	      eqstat = 1;
	    else if ((rowindic==2)&&(fabs(mypi[row])>tol))
	      /* between bounds and pi <> 0 */
	      eqstat = 1;
	    break;
	    
	  case -1 :    /* maximization */
	    
	    if ((rowindic==0)&&(mypi[row]>tol))
	      /* at lowerbound and pi>0 */
	      eqstat = 1;
	    else if ((rowindic==1)&&(mypi[row]< -tol))
	      /* at upperbound and pi<0 */
	      eqstat = 1;
	    else if ((rowindic==2)&&(fabs(mypi[row])>tol))
	      /* between bounds and pi <> 0 */
	      eqstat = 1;
	    break;
	  } /* switch */
	  
	}
	
	
	gfwrow(level, mypi[row], rowindic, eqstat);
	//cout<<row<<" level "<<level<<" mypi "<<mypi[row]<<" rowindic "<<rowindic<<" eqstat "<<eqstat<<endl;
	
	
      }
    
    //   printf("Writing cols\n");
    
    for (col=0;col<n+1;++col)
      {
	
	if (reformulated)
	  if (col==objvar) {
	    level = mipobj+reformb;
	    dual = 0.0;
	    colindic = 2;
	    clstat = 0;
	    gfwcol(level,dual,colindic,clstat);
	  }
	
	if (col>=n) continue;
	
	
	ubd = ub[col];
	lbd = lb[col];
	
      
      
	if(colstat[col] == 1) //basic
	  colindic = 2;
	else{
	  if(fabs(x[col] - lbd)<tol)  colindic = 0;
	  else if(fabs(x[col] - ubd)<tol)  colindic = 1;
	  else  colindic = 3;
	}
	
	clstat = 0;

	/* first check if variable is unbounded  SOPLEX can not do that*/
	
	
	
	/* if not then maybe infeasible */
	if (infes) {
	  
	  level = x[col];
	  
	  infeas = 0.0;
	  if (level>ubd)
	    infeas = level-ubd;
	  else if (level<lbd)
	    infeas = lbd-level;
	  else
	    infeas = 0.0;
	  if (infeas>tol) {
	    clstat = 2;     /* infeasible */
	    infes_found = TRUE;
	  }
	}
	
	
	/* if not then maybe non-optimal */
	if (nonopt && (clstat==0)) {
	  
	  switch (objsen) {
	    
	  case 1 :    /* minimization */
	    
	    if ((colindic==0)&&(dj[col]< -tol))
	      /* at lowerbound and dj<0 */
	      clstat = 1;
	    else if ((colindic==1)&&(dj[col]>tol))
	      /* at upperbound and dj>0 */
	      clstat = 1;
	    else if ((colindic==2)&&(fabs(dj[col])>tol))
	      /* between bounds and dj <> 0 */
	      clstat = 1;
	    break;
	    
	  case -1 :    /* maximization */
	    
	    if ((colindic==0)&&(dj[col]>tol))
	      /* at lowerbound and dj>0 */
	      clstat = 1;
	    else if ((colindic==1)&&(dj[col]< -tol))
	      /* at upperbound and dj<0 */
	      clstat = 1;
	    else if ((colindic==2)&&(fabs(dj[col])>tol))
	      /* between bounds and dj <> 0 */
	      clstat = 1;
	    break;
	  } /* switch */
	  
	  
	}
	
	gfwcol(x[col],dj[col],colindic,clstat);
	
      }
    
    //  }
  
  double output_time = gfclck()-calc_time-input_time;
    
  gfwrti(input_time);
  gfwcti(calc_time);
  gfwwti(output_time);
    
    
  gfstct("STOPC");
  gfclos();  
  
  return;
}



//10. just need to free solution
void free_memory()
{

  free(x);
  free(dj);
  free(mypi);
  free(slack);
  free(colstat);
  free(rowstat);
  
  return;
  
}








//-----------------------------------------------------------


void error(char* msg)
{
  printf("\n%s\n",msg);
  fprintf(gfiosta,"\n*** %s\n\n",msg);
  gfstct("SYSOUT");
  free_memory() ;
  exit(1);
}




void mapmipstat(int mipstat, int* solstat, int* modstat, bool* infes, 
		bool* unb, bool* nonopt)

{
  if ((mipstat >= 1) && (mipstat <= 6)) {
    if(mipstat==1  &&  mipWork.getFinalGap() > 1e-10 ){
      *solstat = mipstatmap[mipstat+5].solverstatus;
      *modstat = mipstatmap[mipstat+5].modelstatus;
      *infes   = mipstatmap[mipstat+5].infes;
      *unb     = mipstatmap[mipstat+5].unb;
      *nonopt  = mipstatmap[mipstat+5].nopt;
    }
    else{
      *solstat = mipstatmap[mipstat-1].solverstatus;
      *modstat = mipstatmap[mipstat-1].modelstatus;
      *infes   = mipstatmap[mipstat-1].infes;
      *unb     = mipstatmap[mipstat-1].unb;
      *nonopt  = mipstatmap[mipstat-1].nopt;
    }
  }
  
  else error("Illegal MIP status");
}



void mipmessage(int mipstat)
{
  if ((mipstat >= 1) && (mipstat <= 6)) {
    if(mipstat==1  &&  mipWork.getFinalGap() > 1e-10 ){
      message(mipstatmap[mipstat+5].msg);
    }
    else{
      message(mipstatmap[mipstat-1].msg);
    }
  }
  
  else error("Illegal MIP status");
}


void message(char* mess)
{
  fprintf(gfiolog,"%s\n",mess);
  fprintf(gfiosta,"%s\n",mess);
  
  return;
}


void createFinishFile()
{
  
  cout<<"creating a finish log"<<endl;
  
  FILE *fp;
  char outfile_name[100];
 
  
  sprintf(outfile_name, 
	  "/local.wheel/qun/FATCOP/data-structure/submit/gams/finish.gams");
  fp = fopen(outfile_name, "w"); 
  
  if (fp == NULL) cout<<"Can not open the file"<<endl;
  

  fprintf(fp, "finish!!!!!!\n ");

  fclose(fp);

  return;  
}

  





