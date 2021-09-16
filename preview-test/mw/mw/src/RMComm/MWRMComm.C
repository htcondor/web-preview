#include "MWRMComm.h"

MWRMComm::MWRMComm()
{
  target_num_workers = -1;
  num_arches = -1;
  memset ( worker_executables, 0, ( 16 * _POSIX_PATH_MAX ) );
  memset ( worker_requirements, 0, ( 16 * 1024 ));

  worker_checkpointing = false;
}


MWRMComm::~MWRMComm()
{
}

void MWRMComm::exit( int exitval )
{
  ::exit ( exitval );
}

int MWRMComm::write_checkpoint( FILE *fp )
{
  
  MWprintf ( 80, "Writing checkpoint for MWPvmRC layer.\n" );
  fprintf ( fp, "%d %d\n", target_num_workers, num_arches );
  for ( int i=0 ; i<num_arches ; i++ ) {
    fprintf ( fp, "%s\n", worker_executables[i] );
    if ( strlen ( worker_requirements[i] ) > 0 ) 
      fprintf ( fp, "1 %s\n", worker_requirements[i] );
    else
      fprintf ( fp, "0 " );
  }
		
  write_RMstate ( fp );
  return 0;

	      
}

int MWRMComm::read_checkpoint( FILE *fp )
{
  int temp;
  MWprintf ( 50, "Reading checkpoint in MWPvmRC layer.\n" );
  fscanf ( fp, "%d %d", &target_num_workers, &num_arches );
  MWprintf ( 50, "Target num workers: %d    Num arches %d.\n", 
	     target_num_workers, num_arches );
  MWprintf ( 50, "Worker executables:\n" );
  for ( int i=0 ; i<num_arches ; i++ ) {
    fscanf ( fp, "%s\n", worker_executables[i] );
    MWprintf ( 50, "%s\n", worker_executables[i] );
    fscanf ( fp, "%d ", &temp );
    if ( temp == 1 )
      {
	fgets ( worker_requirements[i], 1024, fp );
	worker_requirements[i][strlen(worker_requirements[i])-1] = '\0';
      }
    else
      strcpy ( worker_requirements[i], "" );
    
    MWprintf ( 50, "Reqs: %s\n", worker_requirements[i] );
  }
  read_RMstate ( fp );
  
  return 0;
}

int MWRMComm::read_RMstate( FILE *fp ) 
{ 
  return 0;
}

int MWRMComm::write_RMstate( FILE *fp )
{
  return 0;
}

void MWRMComm::set_worker_attributes( int arch_class, char *exec_name, 
				      char *requirements )
{
  if( arch_class >= num_arches ) {
    MWprintf( 10, "set_worker_attributes(): incrementing num_arches to %d\n", 
	      arch_class + 1 );
    num_arches = arch_class + 1;
  }
  if( exec_name != NULL ) {
    strncpy ( worker_executables[arch_class], exec_name, _POSIX_PATH_MAX );
  }
  if( requirements != NULL ) {
    strncpy ( worker_requirements[arch_class], requirements, 1024 );
    processExec ( worker_executables[arch_class], worker_requirements[arch_class] );
  }
}

void MWRMComm::processExec( char *, char * )
{
}

int MWRMComm::get_num_arches() 
{
  return num_arches;
}

void MWRMComm::set_num_arches( int n )
{
  if( n <= 0 ) {
    MWprintf( 10, "Error!  Trying to set num_arches to %d.  It will be 1 instead\n", n );
    num_arches = 1;
  }
  else if( n > 16 ) {
    MWprintf( 10, "Error!  MW is compiled for a mximum of 16 different arches\n" );
    num_arches = 16;
  }
  else {
    num_arches = n;
  }
  return;
}

void MWRMComm::set_worker_checkpointing( bool wc )
{
  if( wc == true )
    MWprintf( 10, "Warning!  Worker checkpointing not available in this CommRM implementation\n" );
  worker_checkpointing = false;
}
