#include "MWFileTypes.h"
#include "MWFileSend.h"
#include "MWFileRC.h"
#include "MWFileRCSymbol.h"
#include "MWFileError.h"
#include "../../MWWorkerID.h"
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>


#ifdef FILE_MASTER
#include </unsup/condor-production/common/include/user_log.c++.h>
#endif

#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>

static enum MWworker_states TranslateState( FileWorkerState state )
{
    switch (state)
    {
	case FILE_IDLE:
		return INITIALIZING;
		break;
	case FILE_SUBMITTED:
		return INITIALIZING;
		break;
	case FILE_RUNNING:
		return WORKING;
		break;
	case FILE_EXECUTE:
		return INITIALIZING;
	case FILE_RESUMED:
		return WORKING;
	case FILE_SUSPENDED:
		return SUSPENDED;
	case FILE_KILLED:
		return EXITED;
	case FILE_SHOT:
		return EXITED;
	default:
		return IDLE;
    }
}


static void Truncate ( char *file )
{
    if ( truncate ( file, 0 ) != 0 );
    	//MWprintf ( 10, "Could not truncate file %s. Errno is %d\n", file, errno );
}

static void HardDeleteFile ( char *file )
{
    if ( !file ) return;
    struct stat statbuf;
    if ( stat ( file, &statbuf ) == 0 )
	unlink ( file );
}

/* Note that now this file exists in proper shape */
static int GetNumWorkers ( char *file, int *ret )
{
    int ret_val = 0;
    struct stat statbuf;

    if ( stat ( file, &statbuf ) == -1 )
    {
	return 0;
    }

    FILE *numwf = Open ( file, "r" );
    if ( numwf == NULL )
    {
	MWprintf ( 10, "Coudn't open the moment_worker_file for reading\n");
	return 0;
    }

    if ( fscanf ( numwf, "%d %d ", &ret_val, ret ) < 0 )
    {
    	MWprintf (10, "Was unable to read the moment worker file\n");
	Close ( numwf );
    	return 0;
    }

    Close ( numwf );
    return ret_val;

}

void
MWFileRC::exit ( int retval )
{

#ifndef FILE_MASTER
    ::exit( retval );
#else
    struct stat statbuf;
    char cmd[_POSIX_PATH_MAX];

    for ( int i = 0; i < max_num_workers; i++ )
    {
	if ( &fileWorkers[i] && fileWorkers[i].state != FILE_FREE )
	    killWorker ( i );
    }

    /* We have to unlink the directories */
    if ( stat ( output_directory, &statbuf ) == 0 )
    {
    	strcpy ( cmd, "/bin/rm -rf " );
    	strcat ( cmd, output_directory );
    	if ( system ( cmd ) < 0 )
	    MWprintf ( 10, "Cannot unlink %s directory\n", output_directory );
    }

    if ( stat ( input_directory, &statbuf ) == 0 )
    {
    	strcpy ( cmd, "/bin/rm -rf " );
    	strcat ( cmd, input_directory );
    	if ( system ( cmd ) < 0 )
	    MWprintf ( 10, "Cannot unlink %s directory\n", input_directory );
    }

    if ( stat ( control_directory, &statbuf ) == 0 )
    {
    	strcpy ( cmd, "/bin/rm -rf " );
    	strcat ( cmd, control_directory );
    	if ( system ( cmd ) < 0 )
	    MWprintf ( 10, "Cannot unlink %s directory\n", control_directory );
    }

    ::exit( retval );
#endif
}



int
MWFileRC::setup( int argc, char *argv[], int *mytid, int *master_tid )
{
    SIMUL_SEND = 100;

    if ( isMaster == TRUE )
    {

    	struct stat statbuf;

	strcpy ( output_directory, "worker_output" );
	strcpy ( input_directory, "worker_input" );
	strcpy ( init_file, "Init_File" );
	strcpy ( control_directory, "submit_files" );
	strcpy ( moment_worker_file, "moment_worker_file" );
	cyclePosition = 0;
	turnNo = 0;
	*mytid = FileRCID;
	*master_tid = FileRCID;
	workerArch = NULL;
	CHECKLOG_FREQ = 100;
	subId = 0;
	old_num_workers = 0;
	max_num_workers = 0;

	if ( stat ( output_directory, &statbuf ) == -1 )
	{
	    /* No output_directory exists. We have to create one */
	    if ( mkdir ( output_directory, S_IRWXU ) < 0 )
	    {
	    	MWprintf ( 10, "Couldn't create directory %s\n", output_directory );
		exit(1);
	    }
	}
	if ( stat ( input_directory, &statbuf ) == -1 )
	{
	    /* No input_directory exists. We have to create one */
	    if ( mkdir ( input_directory, S_IRWXU ) < 0 )
	    {
	    	MWprintf ( 10, "Couldn't create directory %s\n", input_directory );
		exit(1);
	    }
	}
	if ( stat ( control_directory, &statbuf ) == -1 )
	{
	    /* No control_directory exists. We have to create one */
	    if ( mkdir ( control_directory, S_IRWXU ) < 0 )
	    {
	    	MWprintf ( 10, "Couldn't create directory %s\n", control_directory );
		exit(1);
	    }
	}
    }
    else
    {
	// Slave FileRC instance. Only now the argc and argv make any sense
	FileRCID = atoi ( argv[1] );
	expected_number = atoi ( argv[2] );
	strcpy ( init_file, argv[3] );
	strcpy ( output_directory, argv[4] );
	strcpy ( input_directory, argv[5] );
	*master_tid = atoi ( argv[6] );
	*mytid = FileRCID;
	char buf[_POSIX_PATH_MAX];
	sprintf ( buf, "%s/worker_waitfile.%d", input_directory, FileRCID );
	num_sync = 0;
	master_expected_number = GetMasterExpectedNumber ( buf );
	MasterUp = RE_INIT;
    }
    return 0;
}


//---------------------------------------------------------------------
// MWFileRC::init_beginning_workers():
//	Called every time a master gets up either at the init time or 
// aftermath a brutal crash. It is here that we do a resuscicate and
// fill our worker lists.
// We start off by generating all the needed workers.
//---------------------------------------------------------------------
int
MWFileRC::init_beginning_workers ( int *nworkers, MWWorkerID ***workers )
{
	
    int j = 0;

    for ( int i = 0; i < max_num_workers; i++ )
    {
	if ( fileWorkers[i].state == FILE_RUNNING || fileWorkers[i].state == FILE_SUSPENDED || fileWorkers[i].state == FILE_RESUMED || fileWorkers[i].state == FILE_SUSPENDED )
	{
	    j++;
	}
    }
    *nworkers = j;

    if ( j == 0 ) return 0;

    (*workers) = new MWWorkerID*[j];

    j = 0;
    for ( int i = 0; i < max_num_workers; i++ )
    {
	if ( fileWorkers[i].state == FILE_RUNNING || fileWorkers[i].state == FILE_SUSPENDED || fileWorkers[i].state == FILE_RESUMED || fileWorkers[i].state == FILE_SUSPENDED )
	{
	    (*workers)[j] = new MWWorkerID;
	    (*workers)[j]->set_id1 ( fileWorkers[i].id );
	    (*workers)[j]->set_id2 ( fileWorkers[i].id );
	    (*workers)[j]->setState ( TranslateState (fileWorkers[i].state) );
	    setup_notifies ( i );
	    j++;
	}
    }
    MWprintf (10, "Created in init %d workers\n", j );
    return 0;
}

//---------------------------------------------------------------------------
// MWFileRC::start_worker ()
//	In responce to a HOSTADD message. You know in whomRecv who has started.
//---------------------------------------------------------------------------
int
MWFileRC::start_worker ( MWWorkerID *w )
{
    if ( !w )
    {
	MWprintf ( 10, "WorkerID w cannot be null in start_worker\n");
	return -1;
    }

    w->set_id1 ( fileWorkers[whomRecv].id );
    w->set_id2 ( fileWorkers[whomRecv].id );
    w->set_arch ( fileWorkers[whomRecv].arch );
    return 0;
}

int MWFileRC::removeWorker ( MWWorkerID *w )
{
    if ( !w ) 
    {
	MWprintf (10, "Worker ID cannot be NULL in removeWorker\n" );
	return -1;
    }

    char buf[_POSIX_PATH_MAX];

    int condorID = fileWorkers[w->get_id1()].condorID;
    int procID = fileWorkers[w->get_id1()].condorprocID;
    MWprintf ( 10, "Removing workers %d.%d of %d\n", condorID, procID, w->get_id1() );
    sprintf ( buf, "condor_rm %d.%d", condorID, procID );

    if ( system ( buf ) < 0 )
    {
    	MWprintf ( 10, "Could not condor_rm the worker\n" );
    }

    old_num_workers--;
    fileWorkers[w->get_id1()].state = FILE_FREE;
    write_RMstate ( NULL );
    return 0;

}


//---------------------------------------------------------------------
// MWFileRC::do_spawn(  int workerid ):
//	Spawns the worker.
//---------------------------------------------------------------------
int
MWFileRC::do_spawn ( int *workerid, int *archs, int nWorkers )
{
    int cID = -1;
    char sub_file[_POSIX_PATH_MAX];
    char exe[_POSIX_PATH_MAX];
    char logfile[_POSIX_PATH_MAX];
    char master_waitfile[_POSIX_PATH_MAX], worker_waitfile[_POSIX_PATH_MAX];
    FILE *ptr;
    char temp[_POSIX_PATH_MAX];
    int isSubmit = 0;

    for ( int i = 0; i < num_arches; i++ )
    {
	isSubmit = 0;
	int *sameArch = new int[nWorkers];
    	sprintf( sub_file, "./%s/submit_file.%d", control_directory, 
								subId++ );
    	sprintf( exe, "/unsup/condor/bin/condor_submit %s", sub_file );
    	FILE *f = Open( sub_file, "w" );
    	if ( f == NULL )
    	{
	    MWprintf ( 10, "Couldn't open the submitfile for writing\n");
	    return -1;
    	}
    	
    	fprintf( f, "Executable = %s.$$Opsys.$$Arch\n", worker_executables[i] );

	for ( int j = 0; j < nWorkers; j++ )
	{
	    if ( archs[j] == i )
	    {
		sameArch[isSubmit++] = j;
    		sprintf( logfile, "./%s/log_file.%d", control_directory, 
						workerid[j] );
    		sprintf( master_waitfile, "./%s/master_waitfile.%d", 
						output_directory, workerid[j] );
    		sprintf( worker_waitfile, "./%s/worker_waitfile.%d", 
						input_directory, workerid[j] );

    		fprintf( f, "arguments = %d %d %s %s %s -1\n", workerid[j], 
			0, init_file, output_directory, input_directory ); 
    		fprintf( f, "log = %s/log_file.%d\n", control_directory, 
						workerid[j] );
    		fprintf( f, "Output = %s/output_file.%d\n", control_directory, 
						workerid[j] );
    		fprintf( f, "Error = %s/error_file.%d\n", control_directory, 
						workerid[j] );
    		fprintf( f, "Requirements = %s\n", worker_requirements[i] );
		uid_t myid = getuid();
		struct passwd *passwd = getpwuid ( myid );
    		fprintf( f, "NotifyUser = %s\n", passwd->pw_name );
    		fprintf( f, "Queue\n" );

		Truncate( logfile );
		Truncate( master_waitfile );
		Truncate( worker_waitfile );
	    }
	}
	Close( f );

	if ( isSubmit == 0 ) continue;

	MWprintf (10, "About to call condor_submit %s\n", exe);
	if ( (ptr = popen(exe, "r") ) != NULL )
	{
	    while ( fscanf(ptr, "%s", temp ) >= 0 )
	    {
	    	if ( strcmp ( temp, "cluster" ) == 0 )
	    	{
		    fscanf( ptr, "%s", temp );
		    cID = atoi ( temp );
	    	    pclose(ptr);
		    ptr = NULL;
		    MWprintf ( 10, "Spawned to cluster %d\n", cID );
		    break;
		}
	    }
	    if ( ptr ) pclose ( ptr );
	    for (; isSubmit > 0; isSubmit-- )
	    {
	        int tempp = workerid[sameArch[isSubmit - 1]];
	        fileWorkers[tempp].condorID = cID;
		fileWorkers[tempp].condorprocID = isSubmit - 1;
	        fileWorkers[tempp].arch = i;
	        fileWorkers[tempp].counter = 0;
	        fileWorkers[tempp].worker_counter = 0;
	        fileWorkers[tempp].id = tempp;
	        fileWorkers[tempp].state = FILE_IDLE;
	        fileWorkers[tempp].event_no = 0;
	    }
	}
    	else
    	{
		MWprintf ( 10, "Couldn't popen in FileRC\n");
		return -1;
	}
	delete [] sameArch;
    }

    return 1;
}

//------------------------------------------------------------------------
// worker_id is the worker id of the worker
//------------------------------------------------------------------------
int
MWFileRC::setup_notifies ( int worker_tid )
{
	/* Takes a task id and sets up all notifies at once */

    MWprintf ( 40, "Setting up notifies for tid %d.\n", worker_tid );
    
    workerEvents[worker_tid].tag[FileTaskExit] = HOSTDELETE;
    workerEvents[worker_tid].tag[FileTaskSuspend] = HOSTSUSPEND;
    workerEvents[worker_tid].tag[FileTaskResume] = HOSTRESUME;
    workerEvents[worker_tid].tag[FileHostAdd] = HOSTADD;

    return 0;
}


void
MWFileRC::inform_target_num_workers ()
{
    if ( old_num_workers == target_num_workers ) return ;

    int i;

    if ( max_num_workers < target_num_workers )
    {
        FileRCEvent *newEvents = new FileRCEvent[target_num_workers];
	max_num_workers = target_num_workers;
    	for ( i = 0; i < old_num_workers; i++ )
    	{
	    newEvents[i] = workerEvents[i];
    	}
	delete [] workerEvents;
    	workerEvents = newEvents;

    	struct FileWorker *newWorkers = 
				new (struct FileWorker)[target_num_workers];

    	for ( i = 0; i < old_num_workers; i++ )
    	{
	    newWorkers[i] = fileWorkers[i];
    	}
    	for ( i = old_num_workers; i < target_num_workers; i++ )
    	{
	    newWorkers[i].state = FILE_FREE;
	    newWorkers[i].served = -1;
	    newWorkers[i].event_no = 0;
    	}

    	delete []fileWorkers;
    	fileWorkers = newWorkers;
    }

}



int
MWFileRC::initsend ( int useless = 0 )
{
	/* This prepares for a series of packs before we do the actual send */
    delete sendList;

    sendList = new List();
    return 0;
}


//---------------------------------------------------------------------------
//
// 
//---------------------------------------------------------------------------
int
MWFileRC::send ( int to_whom, int msgtag )
{
    char filename[_POSIX_PATH_MAX];
    char control_file[_POSIX_PATH_MAX];
    FILE *fp;
    if ( isMaster == TRUE )
    {
	// In the master mode 
    	sprintf ( filename, "%s/worker_input.%d.%d", input_directory, to_whom,
					fileWorkers[to_whom].worker_counter );
    	sprintf ( control_file, "%s/worker_waitfile.%d", input_directory, 
					to_whom );
    }
    else if ( isMaster == FALSE )
    {
	// We are sending something to the master. So to_whom actually has
	// no meaning.
    	sprintf ( filename, "%s/worker_output.%d.%d", output_directory,
					FileRCID, master_expected_number );
    	sprintf ( control_file, "%s/master_waitfile.%d", output_directory,
					FileRCID );
    }

    fp = Open ( filename, "w" );
    if ( fp == NULL )
    {
	MWprintf ( 10, "Couldn't open file %s for writing work\n", filename );
	return -1;
    }

    fprintf ( fp, "%d ", msgtag );

    while ( sendList->IsEmpty() != TRUE )
    {
	struct FileSendBuffer *buf = ( struct FileSendBuffer *)sendList->Remove();
	switch ( buf->type )
	{
	    case INT:
		fprintf ( fp, "%d %d ", INT, *(int *)buf->data );
		break;
	    case CHAR:
		fprintf ( fp, "%d %c ", CHAR, *(char *)buf->data );
		break;
	    case LONG:
		fprintf ( fp, "%d %ld ", LONG, *(long *)buf->data );
		break;
	    case FLOAT:
		fprintf ( fp, "%d %f ", FLOAT, *(float *)buf->data );
		break;
	    case DOUBLE:
		fprintf ( fp, "%d %lf ", DOUBLE, *(double *)buf->data );
		break;
	    case UNSIGNED_INT:
		fprintf ( fp, "%d %o ", UNSIGNED_INT, *(unsigned int *)buf->data );
		break;
	    case SHORT:
		fprintf ( fp, "%d %hd ", SHORT, *(short *)buf->data );
		break;
	    case UNSIGNED_SHORT:
		fprintf ( fp, "%d %ho ", UNSIGNED_SHORT, *(unsigned short *)buf->data );
		break;
	    case UNSIGNED_LONG:
		fprintf ( fp, "%d %lo ", UNSIGNED_LONG, *(unsigned long *)buf->data );
		break;
	    case STRING:
		fprintf ( fp, "%d %d %s ", STRING, buf->size, 
						(char *)buf->data );
		break;
	    default:
		MWprintf ( 10, "Couldnot decipher a data type while sending\n");
		MWprintf ( 10, "Got %d\n", buf->type );
		Close ( fp );
		return -1;
	}

	delete buf;
    }

    fflush ( fp );
    Close ( fp );

    fp = Open ( control_file, "a" );
    if ( fp == NULL )
    {
	MWprintf ( 10, "Could not open the waitfile %s\n", control_file );
	return -1;
    }

    if ( isMaster == TRUE )
    {
    	fprintf ( fp, "%d %c", fileWorkers[to_whom].worker_counter, OK );
	fileWorkers[to_whom].worker_counter++;
    }
    else
    {
	fprintf ( fp, "%d %c",  master_expected_number, OK );
	master_expected_number++;
    }
    fflush ( fp );
    Close ( fp );

    return 0;
}


int
MWFileRC::recv ( int from_whom, int msgtag )
{
    delete recvList;
    recvList = new List();

    if ( isMaster == TRUE )
	return master_recv ( from_whom, msgtag );
    else
	return worker_recv ( from_whom, msgtag );
}

int MWFileRC::master_recv ( int from_whom, int msgtag )
{
	// msgtag has no meaning as we have to recv all sorts of control info.
    int i;
    bool change = FALSE;

    while ( 1 )
    {
    	if ( turnNo == CHECKLOG_FREQ )
    	{
	    CheckLogFilesRunning();
	    turnNo = 0;
    	}
    	for ( i = cyclePosition; i < target_num_workers; i++ )
    	{
	    cyclePosition++;
	    switch ( fileWorkers[i].state )
	    {
	    	case FILE_RUNNING:
		    change = IsComplete ( i );
		    if ( change == TRUE )
		    {
    			MWprintf ( 10, "Master received something from worker %d\n", i );
			return handle_finished_worker ( i );
		    }
		    break;

		case FILE_EXECUTE:
    		    MWprintf ( 10, "Master found that worker is Started %d\n", i );
		    return handle_executing_worker (i );
		    break;

	    	case FILE_KILLED:
		    if ( fileWorkers[i].served != FILE_KILLED )
		    {
		    	MWprintf ( 10, "Master found that worker was Killed \n");
		    	return handle_killed_worker(i);
		    }
		    break;
		
		case FILE_SUSPENDED:
		    if ( fileWorkers[i].served != FILE_SUSPENDED )
		    {
		    	MWprintf ( 10, "Master found that worker was Suspended \n");
		    	return handle_suspended_worker(i);
		    }
		    break;

		case FILE_RESUMED:
		    if ( fileWorkers[i].served != FILE_RESUMED )
		    {
		    	MWprintf ( 10, "Master found that worker was Resumed \n");
		    	return handle_resumed_worker(i);
		    }
		    break;

		case FILE_SHOT:
		    break;

		default:
		    break;
	    }
	}

	cyclePosition = 0;
	turnNo++;
    }
}


int
MWFileRC::worker_recv ( int from_whom, int msgtag )
{
    //from_whom has no meaning as it is invariably from the master.
    int calculated_number = 0;
    char c;
    char filename[_POSIX_PATH_MAX]={0};
    FILE *op = NULL;
    int cal_sync = 0;


    sprintf ( filename, "%s/worker_waitfile.%d", input_directory, FileRCID );
    op = Open ( filename, "r" );
    while ( 1 )
    {
    	if ( !op )
	{
	    sleep(1);
	    op = Open ( filename, "r" );
	    cal_sync = 0;
	    continue;
	}
	if ( fscanf( op, "%c", &c ) < 0 )
	{
	    Close( op );
	    sleep(1);
	    op = Open ( filename, "r" );
	    cal_sync = 0;
	    continue;
	}
	if ( c == OK )
	{
	    if ( calculated_number != expected_number )
	    {
		if ( calculated_number > expected_number )
		{
		    MWprintf(10, "Fault in master resuscicate logic,exiting\n");
		    Close ( op );
		    return -1;
		}
		calculated_number = 0;
		continue;
	    }
	    else
	    {
		MWprintf ( 10, "Got the work in slave %d\n", expected_number );
		expected_number++;
		Close(op);
		whomRecv = 0;
		return handle_work( msgtag );
	    }
	}
	else if ( c == ' ' )
	{
	}
	else if ( c == SYNC)
	{
	    // Master wants it to ignore some things.
	    calculated_number = 0;
	}
	else if ( c == ESYNC )
	{
	    cal_sync++;
	    if ( num_sync < cal_sync )
	    {
		MWprintf ( 10, "Changed my master_expected_number and %d\n", num_sync + 1 );
	    	master_expected_number = calculated_number;
		num_sync++;
		Close(op);
		return handle_master_executing ();
	    }
	    calculated_number = 0;
	}
	else if ( isdigit ( c ) )
	{
	    calculated_number = calculated_number * 10 + c - '0';
	}
	else
	{
	    Close ( op );
	    return -1;
	}
    }
}


int
MWFileRC::pack ( char *bytes, int nitem, int stride = 1 )
{
    int i;

    if ( nitem < 0 ) return -1;
    if ( nitem == 0 ) return 0;

    if ( sendList == NULL )
    {
	MWprintf ( 10, "Didn't do an init send before packing something\n");
	return -1;
    }

    for ( i = 0; i < nitem; i++ )
    {
	FileSendBuffer *buf = 
		new FileSendBuffer ( (void *)&bytes[i], CHAR, sizeof(char) );
	sendList->Append ( (void *)buf );
    }

    return 0;
}


int
MWFileRC::pack ( float *f, int nitem, int stride = 1 )
{
    int i;

    if ( nitem < 0 ) return -1;
    if ( nitem == 0 ) return 0;

    if ( sendList == NULL )
    {
	MWprintf ( 10, "Didn't do an init send before packing something\n");
	return -1;
    }

    for ( i = 0; i < nitem; i++ )
    {
	FileSendBuffer *buf = 
		new FileSendBuffer ( (void *)&(f[i]), FLOAT, sizeof(float));
	sendList->Append ( (void *)buf );
    }
    return 0;
}


int
MWFileRC::pack ( double *d, int nitem, int stride = 1 )
{
    int i;

    if ( nitem < 0 ) return -1;
    if ( nitem == 0 ) return 0;

    if ( sendList == NULL )
    {
	MWprintf ( 10, "Didn't do an init send before packing something\n");
	return -1;
    }

    for ( i = 0; i < nitem; i++ )
    {
	FileSendBuffer *buf = 
		new FileSendBuffer ( (void *)&(d[i]), DOUBLE, sizeof(double) );
	sendList->Append ( (void *)buf );
    }
    return 0;
}


int
MWFileRC::pack ( int *j, int nitem, int stride = 1 )
{
    int i;

    if ( nitem < 0 ) return -1;
    if ( nitem == 0 ) return 0;

    if ( sendList == NULL )
    {
	MWprintf ( 10, "Didn't do an init send before packing something\n");
	return -1;
    }

    for ( i = 0; i < nitem; i++ )
    {
	FileSendBuffer *buf = 
		new FileSendBuffer ( (void *)&(j[i]), INT, sizeof(int) );
	sendList->Append ( (void *)buf );
    }
    return 0;
}


int
MWFileRC::pack ( unsigned int *ui, int nitem, int stride = 1 )
{
    int i;

    if ( nitem < 0 ) return -1;
    if ( nitem == 0 ) return 0;

    if ( sendList == NULL )
    {
	MWprintf ( 10, "Didn't do an init send before packing something\n");
	return -1;
    }

    for ( i = 0; i < nitem; i++ )
    {
	FileSendBuffer *buf = new FileSendBuffer ( (void *)&(ui[i]), UNSIGNED_INT, sizeof(unsigned int) );
	sendList->Append ( (void *)buf );
    }
    return 0;
}


int
MWFileRC::pack ( short *sh, int nitem, int stride = 1 )
{
    int i;

    if ( nitem < 0 ) return -1;
    if ( nitem == 0 ) return 0;

    if ( sendList == NULL )
    {
	MWprintf ( 10, "Didn't do an init send before packing something\n");
	return -1;
    }

    for ( i = 0; i < nitem; i++ )
    {
	FileSendBuffer *buf = 
		new FileSendBuffer ( (void *)&(sh[i]), SHORT, sizeof(short) );
	sendList->Append ( (void *)buf );
    }
    return 0;
}


int
MWFileRC::pack ( unsigned short *ush, int nitem, int stride = 1 )
{
    int i;

    if ( nitem < 0 ) return -1;
    if ( nitem == 0 ) return 0;

    if ( sendList == NULL )
    {
	MWprintf ( 10, "Didn't do an init send before packing something\n");
	return -1;
    }

    for ( i = 0; i < nitem; i++ )
    {
	FileSendBuffer *buf = 
		new FileSendBuffer ( (void *)&(ush[i]), UNSIGNED_SHORT,
				sizeof(unsigned short) );
	sendList->Append ( (void *)buf );
    }
    return 0;
}


int
MWFileRC::pack ( long *l, int nitem, int stride = 1 )
{
    int i;

    if ( nitem < 0 ) return -1;
    if ( nitem == 0 ) return 0;

    if ( sendList == NULL )
    {
	MWprintf ( 10, "Didn't do an init send before packing something\n");
	return -1;
    }

    for ( i = 0; i < nitem; i++ )
    {
	FileSendBuffer *buf = 
		new FileSendBuffer ( (void *)&(l[i]), LONG, sizeof(long) );
	sendList->Append ( (void *)buf );
    }
    return 0;
}


int
MWFileRC::pack ( unsigned long *ul, int nitem, int stride = 1 )
{
    int i;

    if ( nitem < 0 ) return -1;
    if ( nitem == 0 ) return 0;

    if ( sendList == NULL )
    {
	MWprintf ( 10, "Didn't do an init send before packing something\n");
	return -1;
    }

    for ( i = 0; i < nitem; i++ )
    {
	FileSendBuffer *buf = 
		new FileSendBuffer ( (void *)&(ul[i]), UNSIGNED_LONG, 
						sizeof(unsigned long) );
	sendList->Append ( (void *)buf );
    }
    return 0;
}


int
MWFileRC::pack ( char *str )
{
    if ( str == NULL ) return -1;
    if ( strlen(str) == 0 ) return 0;

    if ( sendList == NULL )
    {
	MWprintf ( 10, "Didn't do an init send before packing something\n");
	return -1;
    }

    FileSendBuffer *buf = 
		new FileSendBuffer ( (void *)str, STRING, 
					strlen(str) + 1 );
    sendList->Append ( (void *)buf );

    return 0;
}


int
MWFileRC::unpack ( char *bytes, int nitems, int stride = 1 )
{
    int i;

    if ( nitems <= 0 ) return 0;

    if ( recvList == NULL || recvList->IsEmpty ( ) == TRUE )
    {
	MWprintf ( 10, "Trying to unpack before receiving \n" );
	return -1;
    }

    for ( i = 0; i < nitems; i++ )
    {
	FileSendBuffer *buf = (FileSendBuffer *)recvList->Remove();
	if ( buf == NULL || buf->type != CHAR )
	{
	    MWprintf ( 20, "Recieve buffer contained something else \n");
	    recvList->Prepend ( (void *)buf );
	    return -1;
	}

	memcpy ( &bytes[i], buf->data, sizeof(char) );
	
	delete buf;
    }

    return 0;
}


int
MWFileRC::unpack ( float *f, int nitems, int stride = 1 )
{
    int i;

    if ( nitems <= 0 ) return -1;

    if ( recvList == NULL || recvList->IsEmpty ( ) == TRUE )
    {
	MWprintf ( 10, "Trying to unpack before receiving \n" );
	return -1;
    }

    for ( i = 0; i < nitems; i++ )
    {
	FileSendBuffer *buf = (FileSendBuffer *)recvList->Remove();
	if ( buf == NULL || buf->type != FLOAT )
	{
	    MWprintf ( 20, "Recieve buffer contained something else \n");
	    recvList->Prepend ( (void *)buf );
	    return -1;
	}

	memcpy ( &f[i], buf->data, sizeof(float) );
	
	delete buf;
    }

    return 0;
}


int
MWFileRC::unpack ( double *d, int nitems, int stride = 1 )
{
    int i;

    if ( nitems <= 0 ) return -1;

    if ( recvList == NULL || recvList->IsEmpty ( ) == TRUE )
    {
	MWprintf ( 10, "Trying to unpack before receiving \n" );
	return -1;
    }

    for ( i = 0; i < nitems; i++ )
    {
	FileSendBuffer *buf = (FileSendBuffer *)recvList->Remove();
	if ( buf == NULL || buf->type != DOUBLE )
	{
	    MWprintf ( 20, "Recieve buffer contained something else \n");
	    recvList->Prepend ( (void *)buf );
	    return -1;
	}

	memcpy ( &d[i], buf->data, sizeof(double) );
	
	delete buf;
    }

    return 0;
}


int
MWFileRC::unpack ( int *j, int nitems, int stride = 1 )
{
    int i;

    if ( nitems <= 0 ) return -1;

    if ( recvList == NULL || recvList->IsEmpty ( ) == TRUE )
    {
	MWprintf ( 10, "Trying to unpack before receiving \n" );
	return -1;
    }

    for ( i = 0; i < nitems; i++ )
    {
	FileSendBuffer *buf = (FileSendBuffer *)recvList->Remove();
	if ( buf == NULL || buf->type != INT )
	{
	    MWprintf ( 20, "Recieve buffer contained something else \n");
	    recvList->Prepend ( (void *)buf );
	    return -1;
	}

	memcpy ( &j[i], buf->data, sizeof(int) );
	
	delete buf;
    }

    return 0;
}


int
MWFileRC::unpack ( unsigned int *ui, int nitems, int stride = 1 )
{
    int i;

    if ( nitems <= 0 ) return -1;

    if ( recvList == NULL || recvList->IsEmpty ( ) == TRUE )
    {
	MWprintf ( 10, "Trying to unpack before receiving \n" );
	return -1;
    }

    for ( i = 0; i < nitems; i++ )
    {
	FileSendBuffer *buf = (FileSendBuffer *)recvList->Remove();
	if ( buf == NULL || buf->type != UNSIGNED_INT )
	{
	    MWprintf ( 20, "Recieve buffer contained something else \n");
	    recvList->Prepend ( (void *)buf );
	    return -1;
	}

	memcpy ( &ui[i], buf->data, sizeof(unsigned int) );
	
	delete buf;
    }
    return 0;
}


int
MWFileRC::unpack ( short *sh, int nitems, int stride = 1 )
{
    int i;

    if ( nitems <= 0 ) return -1;

    if ( recvList == NULL || recvList->IsEmpty ( ) == TRUE )
    {
	MWprintf ( 10, "Trying to unpack before receiving \n" );
	return -1;
    }

    for ( i = 0; i < nitems; i++ )
    {
	FileSendBuffer *buf = (FileSendBuffer *)recvList->Remove();
	if ( buf == NULL || buf->type != SHORT )
	{
	    MWprintf ( 20, "Recieve buffer contained something else \n");
	    recvList->Prepend ( (void *)buf );
	    return -1;
	}

	memcpy ( &sh[i], buf->data, sizeof(short) );
	
	delete buf;
    }

    return 0;
}

int
MWFileRC::unpack ( unsigned short *ush, int nitems, int stride = 1 )
{
    int i;

    if ( nitems <= 0 ) return -1;

    if ( recvList == NULL || recvList->IsEmpty ( ) == TRUE )
    {
	MWprintf ( 10, "Trying to unpack before receiving \n" );
	return -1;
    }

    for ( i = 0; i < nitems; i++ )
    {
	FileSendBuffer *buf = (FileSendBuffer *)recvList->Remove();
	if ( buf == NULL || buf->type != UNSIGNED_SHORT )
	{
	    MWprintf ( 20, "Recieve buffer contained something else \n");
	    recvList->Prepend ( (void *)buf );
	    return -1;
	}

	memcpy ( &ush[i], buf->data, sizeof(unsigned short) );
	
	delete buf;
    }

    return 0;
}


int
MWFileRC::unpack ( long *l, int nitems, int stride = 1 )
{
    int i;

    if ( nitems <= 0 ) return -1;

    if ( recvList == NULL || recvList->IsEmpty ( ) == TRUE )
    {
	MWprintf ( 10, "Trying to unpack before receiving \n" );
	return -1;
    }

    for ( i = 0; i < nitems; i++ )
    {
	FileSendBuffer *buf = (FileSendBuffer *)recvList->Remove();
	if ( buf == NULL || buf->type != LONG )
	{
	    MWprintf ( 20, "Recieve buffer contained something else \n");
	    recvList->Prepend ( (void *)buf );
	    return -1;
	}

	memcpy ( &l[i], buf->data, sizeof(long) );
	
	delete buf;
    }

    return 0;
}


int
MWFileRC::unpack ( unsigned long *ul, int nitems, int stride = 1 )
{
    int i;

    if ( nitems <= 0 ) return -1;

    if ( recvList == NULL || recvList->IsEmpty ( ) == TRUE )
    {
	MWprintf ( 10, "Trying to unpack before receiving \n" );
	return -1;
    }

    for ( i = 0; i < nitems; i++ )
    {
	FileSendBuffer *buf = (FileSendBuffer *)recvList->Remove();
	if ( buf == NULL || buf->type != UNSIGNED_LONG )
	{
	    MWprintf ( 20, "Recieve buffer contained something else \n");
	    recvList->Prepend ( (void *)buf );
	    return -1;
	}

	memcpy ( &ul[i], buf->data, sizeof(unsigned long) );
	
	delete buf;
    }

    return 0;
}


int
MWFileRC::unpack ( char *str )
{
    if ( recvList == NULL || recvList->IsEmpty ( ) == TRUE )
    {
	MWprintf ( 10, "Trying to unpack before receiving \n" );
	return -1;
    }

    FileSendBuffer *buf = (FileSendBuffer *)recvList->Remove();
    if ( buf == NULL || buf->type != STRING )
    {
    	MWprintf ( 20, "Recieve buffer contained something else %d\n", buf->type);
    	recvList->Prepend ( (void *)buf );
    	return -1;
    }

    memcpy ( str, buf->data, sizeof(char) * buf->size );

    delete buf;

    return 0;
}


bool 
MWFileRC::IsComplete ( int i )
{
    char c;
    char f_name[_POSIX_PATH_MAX];

    int number_calculated = 0;
    int num = fileWorkers[i].counter;

    sprintf( f_name, "./%s/master_waitfile.%d", output_directory, i );

    FILE *fp = Open ( f_name, "r" );
    if ( fp == NULL )
    {
	return FALSE;
    }

    while ( fscanf(fp, "%c", &c ) > 0 )
    {
	if ( c == OK )
	{
	    if ( number_calculated == num )
	    {
		Close(fp);
		return TRUE;
	    }
	    else
		number_calculated = 0;
	}
	else if ( c == WRONGINITFILE )
	{
		// The process found a wrong init file.
		// It probably has killed itself. Thus do nothing.
	}
	else if ( c == SYNC )
	{
	    number_calculated = 0;
	}
	else if ( isdigit(c) > 0 )
	{
	    number_calculated = number_calculated * 10 + c - '0';
	}
    }

    Close(fp);
    return FALSE;               
}


int
MWFileRC::handle_finished_worker ( int i )
{
    FILE *fp;
    char filename[_POSIX_PATH_MAX];
    int typeno;

    fileWorkers[i].served = FILE_RUNNING;
    sprintf ( filename, "%s/worker_output.%d.%d", output_directory, fileWorkers[i].id, fileWorkers[i].counter );   
    fp = Open ( filename, "r" );
    if ( fp == NULL )
    {
	MWprintf ( 10, "Couldn't open the worker output file %s for reading\n",
				filename );
	return -1;
    }
    fscanf ( fp, "%d ", &msgTag );

    while ( fscanf ( fp, "%d ", &typeno ) >= 0 )
    {
	FileSendBuffer *buf;

	switch ( typeno )
	{
	    case INT:
		int it1;
		fscanf ( fp, "%d ", &it1 );
		buf = new FileSendBuffer ( (void *)&it1, INT, sizeof(int) );
		recvList->Append ( (void *)buf );
		break;

	    case CHAR:
		char it2;
		fscanf ( fp, "%c ", &it2 );
		buf = new FileSendBuffer ( (void *)&it2, CHAR, sizeof(char) );
		recvList->Append ( (void *)buf );
		break;

	    case LONG:
		long it3;
		fscanf ( fp, "%ld ", &it3 );
		buf = new FileSendBuffer ( (void *)&it3, LONG, sizeof(long) );
		recvList->Append ( (void *)buf );
		break;

	    case FLOAT:
		float it4;
		fscanf ( fp, "%f ", &it4 );
		buf = new FileSendBuffer ( (void *)&it4, FLOAT, sizeof(float) );
		recvList->Append ( (void *)buf );
		break;

	    case DOUBLE:
		double it5;
		fscanf ( fp, "%lf ", &it5 );
		buf = new FileSendBuffer ( (void *)&it5, DOUBLE, sizeof(double));
		recvList->Append ( (void *)buf );
		break;

	    case UNSIGNED_INT:
		unsigned int it6;
		fscanf ( fp, "%o ", &it6 );
		buf = new FileSendBuffer ( (void *)&it6, UNSIGNED_INT, 
						sizeof(unsigned int));
		recvList->Append ( (void *)buf );
		break;

	    case SHORT:
		short it7;
		fscanf ( fp, "%hd ", &it7 );
		buf = new FileSendBuffer ( (void *)&it7, SHORT, sizeof(short));
		recvList->Append ( (void *)buf );
		break;

	    case UNSIGNED_SHORT:
		unsigned short it8;
		fscanf ( fp, "%ho ", &it8 );
		buf = new FileSendBuffer ( (void *)&it8, UNSIGNED_SHORT, 
						sizeof(unsigned short));
		recvList->Append ( (void *)buf );
		break;

	    case UNSIGNED_LONG:
		unsigned long it9;
		fscanf ( fp, "%lo ", &it9 );
		buf = new FileSendBuffer ( (void *)&it9, UNSIGNED_LONG, 
						sizeof(unsigned long));
		recvList->Append ( (void *)buf );
		break;

	    case STRING:
		{
		int sz;
		fscanf ( fp, "%d ", &sz );
		char *it10 = new char[sz+1];
		it10[sz] = '\0';
		for ( int z = 0; z < sz; z++ )
		    fscanf ( fp, "%c", &it10[z] );
		buf = new FileSendBuffer ( (void *)it10, STRING, 
					strlen(it10) + 1);
		recvList->Append ( (void *)buf );
		delete []it10;
		break;
		}

	    default:
		MWprintf (10, "Couldn't decipher the data type in receiving\n");
		MWprintf (10, "Got %d\n", typeno );
		Close ( fp );
		return -1;
		break;
	}
    }
    
    whomRecv = i;
    Close ( fp );
    fileWorkers[i].counter++;
    HardDeleteFile ( filename );
    return 0;
}


int
MWFileRC::bufinfo ( int buf_id, int *len, int *tag, int *sending_host )
{
    *sending_host = whomRecv;
    *tag = msgTag;
    *sending_host = whomRecv;
    return 0;
}

int
MWFileRC::handle_killed_worker ( int i )
{
	// We got the info that a worker has been killed.
	// Inform the upper layer that the worker has disappeared.
    fileWorkers[i].served = FILE_KILLED;

    FileSendBuffer *buf = new FileSendBuffer ( &fileWorkers[i].id, INT,
				sizeof(int) );
    recvList->Append ( (void *)buf );

    whomRecv = i;
    msgTag = workerEvents[i].tag[FileTaskExit];

    --old_num_workers;
	
    fileWorkers[i].state = FILE_FREE;

    return 0;
}

int
MWFileRC::handle_suspended_worker ( int i )
{
	// We got the info from the log file that the worker has been suspended.
	// Inform the upper layer that this is the case.
	// No need to change the state of fileWorkers as that has been
	// modified by the CheckLogFiles.

    fileWorkers[i].served = FILE_SUSPENDED;
    FileSendBuffer *buf = new FileSendBuffer ( &fileWorkers[i].id, INT,
				sizeof(int) );
    recvList->Append ( (void *)buf );

    whomRecv = i;
    msgTag = workerEvents[i].tag[FileTaskSuspend];

    return 0;
}



int
MWFileRC::handle_resumed_worker ( int i )
{
	// We got the info from the log file that the worker has been resumed.
	// Inform the upper layer that this is the case.
	// No need to change the state of fileWorkers as that has been
	// modified by the CheckLogFiles.

    fileWorkers[i].state = FILE_RUNNING;
    fileWorkers[i].served = FILE_RESUMED;
    FileSendBuffer *buf = new FileSendBuffer ( &fileWorkers[i].id, INT,
				sizeof(int) );
    recvList->Append ( (void *)buf );

    whomRecv = i;
    msgTag = workerEvents[i].tag[FileTaskResume];

    return 0;
}

int
MWFileRC::handle_executing_worker ( int i )
{
    // We got the info that the worker i started executin.

    if ( !(&fileWorkers[i]) )
    {
	MWprintf( 10, "How can the corresponding entry in execute worker be null\n");
	return -1;
    }

    fileWorkers[i].served = FILE_EXECUTE;
    fileWorkers[i].state = FILE_RUNNING;
    FileSendBuffer *buf = new FileSendBuffer ( &fileWorkers[i].id, INT,
				sizeof(int) );
    recvList->Append ( (void *)buf );

    whomRecv = fileWorkers[i].id;
    msgTag = workerEvents[fileWorkers[i].id].tag[FileHostAdd];

    MWprintf ( 10, "In MWFile %d %d\n", fileWorkers[i].id, workerEvents[fileWorkers[i].id].tag[FileHostAdd] );
    return 0;
}


int
MWFileRC::handle_master_executing ()
{
    msgTag = MasterUp;
    return 0;
}


int
MWFileRC::handle_work ( int msgtag )
{
	// Called by the worker recv. 
	// Read the worker_input file and prepare the recvList.
    FILE *fp;
    char filename[_POSIX_PATH_MAX];
    int typeno;

    sprintf ( filename, "%s/worker_input.%d.%d", input_directory, FileRCID,
				expected_number - 1 );   
    fp = Open ( filename, "r" );
    if ( fp == NULL )
    {
	MWprintf ( 10, "Couldn't open the worker input file %s for reading\n",
				filename );
	return -1;
    }

    if ( fscanf ( fp, "%d ", &msgTag ) <= 0 ) 
    {
    	Close ( fp );
    	return -1;
    }

    if ( msgtag != -1 && msgTag != msgtag ) 
    {
	MWprintf( 10, "FileRC wrong message sequence got %d \n", msgTag);
	Close ( fp );
	return -1;
    }

    while ( fscanf ( fp, "%d ", &typeno ) > 0 )
    {
	FileSendBuffer *buf;

	switch ( typeno )
	{
	    case INT:
		int it1;
		fscanf ( fp, "%d ", &it1 );
		buf = new FileSendBuffer ( (void *)&it1, INT, sizeof(int) );
		recvList->Append ( (void *)buf );
		break;

	    case CHAR:
		char it2;
		fscanf ( fp, "%c ", &it2 );
		buf = new FileSendBuffer ( (void *)&it2, CHAR, sizeof(char) );
		recvList->Append ( (void *)buf );
		break;

	    case LONG:
		long it3;
		fscanf ( fp, "%ld ", &it3 );
		buf = new FileSendBuffer ( (void *)&it3, LONG, sizeof(long) );
		recvList->Append ( (void *)buf );
		break;

	    case FLOAT:
		float it4;
		fscanf ( fp, "%f ", &it4 );
		buf = new FileSendBuffer ( (void *)&it4, FLOAT, sizeof(float) );
		recvList->Append ( (void *)buf );
		break;

	    case DOUBLE:
		double it5;
		fscanf ( fp, "%lf ", &it5 );
		buf = new FileSendBuffer ( (void *)&it5, DOUBLE, sizeof(double));
		recvList->Append ( (void *)buf );
		break;

	    case UNSIGNED_INT:
		unsigned int it6;
		fscanf ( fp, "%o ", &it6 );
		buf = new FileSendBuffer ( (void *)&it6, UNSIGNED_INT, 
						sizeof(unsigned int));
		recvList->Append ( (void *)buf );
		break;

	    case SHORT:
		short it7;
		fscanf ( fp, "%hd ", &it7 );
		buf = new FileSendBuffer ( (void *)&it7, SHORT, sizeof(short));
		recvList->Append ( (void *)buf );
		break;

	    case UNSIGNED_SHORT:
		unsigned short it8;
		fscanf ( fp, "%ho ", &it8 );
		buf = new FileSendBuffer ( (void *)&it8, UNSIGNED_SHORT, 
						sizeof(unsigned short));
		recvList->Append ( (void *)buf );
		break;

	    case UNSIGNED_LONG:
		unsigned long it9;
		fscanf ( fp, "%lo ", &it9 );
		buf = new FileSendBuffer ( (void *)&it9, UNSIGNED_LONG, 
						sizeof(unsigned long));
		recvList->Append ( (void *)buf );
		break;

	    case STRING:
		{
		int sz;
		fscanf ( fp, "%d ", &sz );
		char *it10 = new char[sz + 1];
		it10[sz] = '\0';
		for ( int z = 0; z < sz; z++ )
		    fscanf ( fp, "%c", &it10[z] );
		buf = new FileSendBuffer ( (void *)it10, STRING, 
					strlen(it10) + 1);
		recvList->Append ( (void *)buf );
		delete []it10;
		break;
		}

	    default:
		MWprintf (10, "Couldn't decipher the data type in receiving\n");
		MWprintf (10, "Got %d\n", typeno );
		Close ( fp );
		return -1;
	}
    }
    
    Close ( fp );
    HardDeleteFile ( filename );
    return 0;
}


//---------------------------------------------------------------------------
// 	MWFileRC::CheckLogFiles():
//	Sofisticated check log function which never fails to detect any event
// in the log.
//---------------------------------------------------------------------------
void 
MWFileRC::CheckLogFilesRunning ( )
{
#ifdef FILE_MASTER
    int i;
    char f_name[_POSIX_PATH_MAX];

    for ( i = 0; i < max_num_workers; i++ )
    {
	//Discuss about this thing.
	if ( fileWorkers[i].state == FILE_FREE ) continue;
	int num_event = 0;

	ReadUserLog log;
	ULogEvent *event;
	ULogEventOutcome outcome;

	sprintf( f_name, "./%s/log_file.%d", control_directory, i );

	if ( access ( f_name, R_OK ) != 0 )
	{
	    fileWorkers[i].state = FILE_IDLE;
	    fileWorkers[i].event_no = 0;
	    continue;
	}

        log.initialize ( f_name );

	outcome = log.readEvent( event );
	for ( ;  outcome == ULOG_OK ;outcome = log.readEvent( event ) )
	{
	    num_event++;
	    if ( fileWorkers[i].event_no >= num_event )
	    {
		delete event;
		continue;
	    }

	    if ( event->eventNumber == ULOG_JOB_ABORTED )
	    {
		fileWorkers[i].state = FILE_SHOT;
		fileWorkers[i].event_no++;
		delete event;
		break;
	    }
	    else if ( event->eventNumber == ULOG_JOB_TERMINATED )
	    {
		fileWorkers[i].state = FILE_KILLED;
		fileWorkers[i].event_no++;
		delete event;
		break;
	    }
	    else if ( event->eventNumber == ULOG_CHECKPOINTED )
	    {
		//fileWorkers[i].state = FILE_SUSPENDED;
		fileWorkers[i].event_no++;
		delete event;
		break;
	    }
	    else if ( event->eventNumber == ULOG_SUBMIT )
	    {
	    	MWprintf ( 10, "Got a submit event\n"); 
		fileWorkers[i].state = FILE_SUBMITTED;
		fileWorkers[i].event_no++;
		delete event;
		break;
	    }
	    else if ( event->eventNumber == ULOG_JOB_EVICTED )
	    {
		JobEvictedEvent *evictEvent = (JobEvictedEvent *)event;
		if ( evictEvent->checkpointed == TRUE ) 
		{
		    fileWorkers[i].state = FILE_SUSPENDED;
		    fileWorkers[i].event_no++;
		    MWprintf ( 10, "Got a good evictevent from %d\n",i );
		}
		else 
		{
		    char exe[_POSIX_PATH_MAX];
		    sprintf ( exe, "condor_rm %d.%d", fileWorkers[i].condorID,
						fileWorkers[i].condorprocID);
		    system ( exe );
		    fileWorkers[i].state = FILE_KILLED;
		    fileWorkers[i].event_no++;
		    MWprintf ( 10, "Got a bad evictevent from %d \n",i );
		}
		delete event;
		break;
	    }
	    else if ( event->eventNumber == ULOG_EXECUTE )
	    {
		if ( fileWorkers[i].state == FILE_SUSPENDED )
		{
		    MWprintf (10,"In LogFiles gotResumed after suspend %d\n",i);
		    fileWorkers[i].state = FILE_RESUMED;
		}
		else
		    fileWorkers[i].state = FILE_EXECUTE;

		fileWorkers[i].event_no++;
		delete event;
		break;
	    }
	    else
	    {
		fileWorkers[i].event_no++;
		delete event;
		break;
	    }
	}
    }
#endif
}


//---------------------------------------------------------------------------
// MWFileRC::CheckFilesResuscicate ():
//	Called when recovering from a crash.
//---------------------------------------------------------------------------
void 
MWFileRC::CheckLogFilesResuscicate ( )
{
#ifdef FILE_MASTER
    int i;
    char f_name[_POSIX_PATH_MAX];

    for ( i = 0; i < max_num_workers; i++ )
    {
	ReadUserLog log;
	ULogEvent *event;
	ULogEventOutcome outcome;

	sprintf( f_name, "./%s/log_file.%d", control_directory, fileWorkers[i].id );

	if ( access ( f_name, R_OK ) != 0 )
	{
	    fileWorkers[i].state = FILE_FREE;
	    fileWorkers[i].event_no = 0;
	    continue;
	}

	setup_notifies ( i );

        log.initialize ( f_name );

	outcome = log.readEvent( event );
	for ( ;  outcome == ULOG_OK ;outcome = log.readEvent( event ) )
	{
	    if ( event->eventNumber == ULOG_JOB_ABORTED )
	    {
		fileWorkers[i].state = FILE_SHOT;
		fileWorkers[i].event_no++;
		delete event;
	    }
	    else if ( event->eventNumber == ULOG_JOB_TERMINATED )
	    {
		fileWorkers[i].state = FILE_KILLED;
		fileWorkers[i].event_no++;
		delete event;
	    }
	    else if ( event->eventNumber == ULOG_CHECKPOINTED )
	    {
		//fileWorkers[i].state = FILE_SUSPENDED;
		fileWorkers[i].event_no++;
		delete event;
	    }
	    else if ( event->eventNumber == ULOG_SUBMIT )
	    {
		fileWorkers[i].state = FILE_SUBMITTED;
		fileWorkers[i].event_no++;
		delete event;
	    }
	    else if ( event->eventNumber == ULOG_JOB_EVICTED )
	    {
		JobEvictedEvent *evictEvent = (JobEvictedEvent *)event;
		if ( evictEvent->checkpointed == TRUE ) 
		{
		    fileWorkers[i].state = FILE_SUSPENDED;
		    fileWorkers[i].event_no++;
		    delete event;
		}
		else 
		{
		    char exe[128];
		    sprintf ( exe, "condor_rm %d.%d", fileWorkers[i].condorID,
						fileWorkers[i].condorprocID);
		    system ( exe );
		    fileWorkers[i].state = FILE_FREE;
		    delete event;
		    break;
		}
	    }
	    else if ( event->eventNumber == ULOG_EXECUTE )
	    {
		fileWorkers[i].state = FILE_RUNNING;
		fileWorkers[i].event_no++;
		delete event;
	    }
	    else
	    {
		fileWorkers[i].event_no++;
		delete event;
	    }
	}
    }

#endif
}


//---------------------------------------------------------------------------
// MWFileRC::resuscicate ( )
//	This is THE function of the whole program. It is this routine that
// gives MWFileRC that robustness that one can only dream right now.
// TODO:- We have to scan the num_target_workers for the number of targets.
//---------------------------------------------------------------------------
void
MWFileRC::resuscicate ()
{
    int  i;
    char c[2]; c[1] = '\0';
    char worker_state_file[256];
    char master_waitfile[256], worker_waitfile[256];

    for ( i = 0; i < max_num_workers; i++ )
    {

	if ( fileWorkers[i].state != FILE_IDLE ) continue;

	sprintf ( worker_state_file, "./%s/log_file.%d", control_directory, fileWorkers[i].id );

	sprintf ( master_waitfile, "./%s/master_waitfile.%d", output_directory, fileWorkers[i].id );
	sprintf ( worker_waitfile, "./%s/worker_waitfile.%d", input_directory, fileWorkers[i].id );
	
	struct stat statbuf;
	if ( stat ( worker_state_file, &statbuf ) == -1 )
	{
	    fileWorkers[i].state = FILE_FREE;
	    continue;
	}

	GetCondorId ( worker_state_file,
			&fileWorkers[i].condorID, &fileWorkers[i].condorprocID);
	fileWorkers[i].worker_counter = GetWorkerCounter ( worker_waitfile );
	fileWorkers[i].counter = GetCounter ( worker_waitfile,master_waitfile );
	MWprintf ( 10, "The counters of %d are %d and %d and %d\n", 
			i, fileWorkers[i].counter, fileWorkers[i].worker_counter, 
			fileWorkers[i].condorID );

	FILE *temp = Open ( worker_waitfile, "a" );
	if ( temp == NULL )
	{
	    MWprintf ( 10, "Oops couldn't open the worker_waitfile for syncing\n");
	    continue;
	}
	fprintf( temp, "%c %d %c", SYNC, fileWorkers[i].counter, ESYNC );
	fflush( temp );
	Close( temp );

    }

    CheckLogFilesResuscicate();
}

//---------------------------------------------------------------------------
//Master::GetCondorId():
//      Read the open log file and get the condor cluster of the log file.
//---------------------------------------------------------------------------
void
MWFileRC::GetCondorId ( char *lgfile, int *cID, int *pID )
{
#ifdef FILE_MASTER

    ReadUserLog log;
    ULogEvent *event;
    ULogEventOutcome outcome;

    log.initialize ( lgfile );
    outcome = log.readEvent( event );
    while ( outcome == ULOG_OK && event->eventNumber != ULOG_SUBMIT )
    {
	outcome = log.readEvent ( event );
    }

    if ( outcome != ULOG_OK ) 
    {
	*cID = -1;
	*pID = -1;
	return;
    }

    *cID = event->cluster;
    *pID = event->proc;
    return;
#endif
    return;
}


int
MWFileRC::ChoseArchNum ()
{
    int i;
    if ( workerArch == NULL )
    {
	workerArch = new int[num_arches];
	for ( i = 0; i < num_arches; i++ )
	    workerArch[i] = 0;
    }

    int min = workerArch[0];
    int ret_val = 0;
    for ( i = 1; i < num_arches; i++ )
    {
	if ( workerArch[i] < min )
	{
	    ret_val = i;
	    min = workerArch[i];
	}
    }
    workerArch[ret_val]++;
    return ret_val;
}


//---------------------------------------------------------------------------
// hostaddlogic():
//	If the number of workers are less than the target number of workers 
// just spawns the remaining. old_num_workers holds the information about the
// the current number of workers.
//---------------------------------------------------------------------------
int
MWFileRC::hostaddlogic ( int *numworkers )
{

    int old;
    if ( old_num_workers == target_num_workers ) return 0;

    old = old_num_workers;
    *numworkers = target_num_workers;
    if ( old_num_workers < target_num_workers )
    {
	inform_target_num_workers () ;

	// We'll have to create new workers but we will do it simul.
	int *ids = new int[target_num_workers - old_num_workers ];
	int *archs = new int[target_num_workers - old_num_workers];
	int index = 0;
	MWprintf (10, "To Create %d workers\n", target_num_workers - old_num_workers );
	for ( int i = old_num_workers; i < target_num_workers; i++ )
	{
    	    for ( int ii = 0; ii < max_num_workers; ii++ )
    	    {
	    	if ( fileWorkers[ii].state == FILE_FREE || fileWorkers[ii].state == FILE_KILLED || fileWorkers[ii].state == FILE_SHOT )
		{
	    	    ids[index] = ii;
		    archs[index++] = ChoseArchNum();
		    fileWorkers[ii].state = FILE_IDLE;
		    break;
		}
    	    }
	}


	if ( index + old_num_workers != target_num_workers )
	{
	    MWprintf ( 10, "Something bit time screwy, should have got free
			fileWorkers for %d workers\n", target_num_workers-old_num_workers);
	    return -1;
	}
	old_num_workers = target_num_workers;

	write_RMstate ( NULL );

	int status = do_spawn ( ids, archs, target_num_workers - old );
    	if ( status >= 0 )
	{
		for ( int kk = old; kk < target_num_workers; kk++ )
		    setup_notifies ( ids[kk - old] );
    	}
    	else
    	{
		MWprintf ( 10, "There was some problem creating the worker \n");
	}
	delete []ids;
	delete []archs;
    }
    return target_num_workers - old_num_workers;
}


//---------------------------------------------------------------------------
// 
int
MWFileRC::config ( int *nhosts, int *narches, MWWorkerID ***w )
{
    
    return 0;
}


//---------------------------------------------------------------------------
// GetWorkerCounter ():
//	Gets the next message to be sent to the worker.
//---------------------------------------------------------------------------
int 
MWFileRC::GetWorkerCounter ( char *file )
{
    int cal = 0;
    int task_num = 0; 
    char c[2]; c[1] = '\0';
    bool end = FALSE;
    bool first = TRUE;

    if ( !file ) return 0;

    FILE *fp = Open ( file, "r" );
    if ( fp == NULL ) return 0;

    while ( fscanf(fp, "%c", &c[0] ) >= 0 )
    {
	if ( isdigit ( c[0] ) > 0 )
	{
	    cal = cal * 10 + atoi(c);
	    end = FALSE;
	}
	else if ( isspace(c[0]) > 0 )
	{
	}
	else if ( c[0] == OK )
	{
	    task_num = cal;
	    cal = 0;
	    end = TRUE;
	    first = FALSE;
	}
	else if ( c[0] == SYNC || c[0] == ESYNC )
	{
	    cal = 0;
	    end = TRUE;
	}
    }
    Close ( fp );
    if ( first == TRUE ) return 0;
    if ( end == TRUE ) 
    {
	return task_num + 1;
    }
    else 
    {
	return task_num;
    }
}



//---------------------------------------------------------------------------
// GetCounter ():
//	Gets the next message to be received from the worker.
//---------------------------------------------------------------------------
int 
MWFileRC::GetCounter ( char *worker_file, char *file )
{

    char c[2]; c[1] = '\0';
    int ret_val = 0;
    int num_cal = 0;

    FILE *fp = Open ( worker_file, "r" );
    if ( fp )
    {
	while ( fscanf ( fp, "%c", &c[0] ) >= 0 )
	{
	    if ( c[0] == SYNC )
	    	num_cal = 0;
	    else if ( isdigit ( c[0] ) > 0 )
	    	num_cal = num_cal * 10 + atoi ( c );
	    else if ( c[0] == ESYNC )
	    {
	    	num_sync++;
	    	ret_val = num_cal;
	    	num_cal = 0;
	    }
    }
    Close ( fp );
  }

    int task_num = 0;
    bool end = TRUE;
    num_cal = 0;

    fp = Open ( file, "r" );
    
    if ( fp )
    {
    	while ( fscanf(fp, "%c", &c[0] ) >= 0 )
    	{
	    if ( isdigit ( c[0] ) > 0 )
	    {
	    	num_cal = num_cal * 10 + atoi(c);
	    	end = FALSE;
	    }
	    else if ( isspace(c[0]) > 0 )
	    {
	    }
	    else if ( c[0] == OK )
	    {
	    	task_num = num_cal;
	    	num_cal = 0;
	    	end = TRUE;
	    }
	    else if ( c[0] == SYNC )
	    {
	    	num_cal = 0;
	    	end = TRUE;
	    }
	    else if ( c[0] == ESYNC )
	    {
	    	num_cal = 0;
	    }
	}
    	Close ( fp );
    }
	
    if ( task_num > ret_val ) return task_num + 1 + SIMUL_SEND;
    return ret_val + 1 + SIMUL_SEND;
}


void
MWFileRC::killWorker ( int i )
{
    char exe[_POSIX_PATH_MAX];
    MWprintf ( 10, "Killing condor job %d.%d\n", fileWorkers[i].condorID,
    						fileWorkers[i].condorprocID );
    sprintf ( exe, "condor_rm %d.%d", fileWorkers[i].condorID,
						fileWorkers[i].condorprocID);
    system ( exe );
}


int
MWFileRC::GetMasterExpectedNumber ( char *filename )
{
    char c[2]; c[1] = '\0';
    FILE *fp = Open ( filename, "r" );
    if ( !fp ) return 0;
    int num_cal = 0;
    int ret_val = 0;


    while ( fscanf ( fp, "%c", &c[0] ) >= 0 )
    {
	if ( c[0] == SYNC )
	    num_cal = 0;
	else if ( isdigit ( c[0] ) > 0 )
	    num_cal = num_cal * 10 + atoi ( c );
	else if ( c[0] == ESYNC )
	{
	    num_sync++;
	    ret_val = num_cal;
	    num_cal = 0;
	}
    }

    Close(fp);
    return ret_val;
}


// Here you have to resuscicate and fill in the appropriate structures
// By This time target_num_workers has been filled up.
// We should not touch target_num_workers. All things go into old_num_workers.
int
MWFileRC::read_RMstate ( FILE *fp = NULL )
{
    int i;
    char target_file[_POSIX_PATH_MAX];

    sprintf ( target_file, "%s/%s", control_directory, moment_worker_file );
    old_num_workers = GetNumWorkers ( target_file, &max_num_workers );

    FileRCEvent *newEvents = new FileRCEvent[max_num_workers];

    for ( i = 0; i < max_num_workers; i++ )
    {
    	newEvents[i].tag[FileTaskExit] = HOSTDELETE;
    	newEvents[i].tag[FileTaskSuspend] = HOSTSUSPEND;
    	newEvents[i].tag[FileTaskResume] = HOSTRESUME;
    	newEvents[i].tag[FileHostAdd] = HOSTADD;
    }
    delete []workerEvents;
    workerEvents = newEvents;

    struct FileWorker *newWorkers =
       		new (struct FileWorker)[max_num_workers];

    for ( i = 0; i < max_num_workers; i++ )
    {
	newWorkers[i].state = FILE_FREE;
	newWorkers[i].served = -1;
	newWorkers[i].event_no = 0;
    }

    delete []fileWorkers;
    fileWorkers = newWorkers;

    // The file worker-map contains the mapping into the ids. 
    read_map ( fileWorkers, target_file );

    if ( old_num_workers == 0 )
    {
	return 0;
    }

    resuscicate();

    return 0;

}

void
MWFileRC::read_map ( struct FileWorker *worker, char *file )
{
    int i, count;
    int temp;
    FILE *fp = Open ( file, "r" );

    /* The first int is the number of workers */
    /* The second the max Num workers which we will ignore at this point */
    if ( fp == NULL )
    {
	MWprintf ( 10, "Coudn't open the moment_worker_file for reading\n");
        return;
    }

    if ( fscanf ( fp, "%d %d ", &count, &i ) < 0 )
    {
        MWprintf (10, "Was unable to read the moment worker file\n");
	Close ( fp );
        return;
    }

    for ( i = 0; i < count; i++ )
    {
    	fscanf ( fp, "%d ", &temp );
	worker[temp].id = temp;
	worker[temp].state = FILE_IDLE;
    }
    Close ( fp );
}

int
MWFileRC::write_RMstate ( FILE *filp = NULL )
{
    
    int i;
    FILE *fp;
    char target_file[_POSIX_PATH_MAX];
    
    sprintf ( target_file, "%s/%s", control_directory, moment_worker_file );
    
    fp = Open ( "temp_File", "w" );
    if  ( fp == NULL )
    {
    	MWprintf ( 10, "Cannot open the file temp_File for writing\n");
	return -1;
    }

    fprintf ( fp, "%d %d ", old_num_workers, max_num_workers );

    for ( i = 0; i < max_num_workers; i++ )
    {
    	if ( fileWorkers[i].state != FILE_FREE && fileWorkers[i].state != FILE_SHOT && fileWorkers[i].state != FILE_KILLED )
	{
	    fprintf ( fp, "%d ", fileWorkers[i].id );
	}
    }

    Close ( fp );

    if ( rename ( "temp_File", target_file ) < 0 )
    {
    	MWprintf (10, "Could not move the new RM_status file\n");
	return -1;
    }
    
    return 0;
}

void
MWFileRC::processExec ( char *exec, char *req )
{
    int i;
    char newstring[1024];
    char arch[100], opsys[100];
    char *index;
    char newexec[_POSIX_PATH_MAX];
    char exe[1024];
    strcpy ( newstring, req );
    for ( i = 0; i < strlen ( newstring ); i++ )
    {
        int k = newstring[i];
        k = toupper(k);
        newstring[i] = k;
    }
    i = 0;
    index = strstr ( newstring, "OPSYS" );
    index += 5;
    while ( *index == ' ' || *index == '=' ) index++;
    while ( *index != ')' ) opsys[i++] = *index++;
    opsys[i] = '\0';

    i = 0;
    index = strstr ( newstring, "ARCH" );
    index += 4;
    while ( *index == ' ' || *index == '=' ) index++;
    while ( *index != ')' ) arch[i++] = *index++;
    arch[i] = '\0';

    sprintf ( newexec, "%s.%s.%s", exec, opsys, arch );
    unlink ( newexec );

#ifdef WINDOWS
    sprintf ( exe, "copy %s %s", exec, newexec );
#else
    sprintf ( exe, "cp %s %s", exec, newexec );
#endif
    system ( exe );

    return;
}
