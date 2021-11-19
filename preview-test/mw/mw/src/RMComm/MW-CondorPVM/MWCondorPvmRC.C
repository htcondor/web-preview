#include "MWCondorPvmRC.h"
#include <MW.h>
#include <MWDriver.h>
#include <MWTask.h>
#include <MWWorker.h>

#ifdef PVM_MASTER
MWRMComm * MWDriver::RMC = new MWPvmRC;
MWRMComm * MWTask::RMC = MWDriver::RMC;
#else
MWRMComm * MWWorker::RMC = new MWPvmRC;
MWRMComm * MWTask::RMC = MWWorker::RMC;
#endif

/* Implementation of comm / rm class for pvm. */

void 
MWPvmRC::exit( int exitval ) {

	MWprintf ( 50, "Before pvm_exit\n" );
	pvm_catchout( 0 );
	pvm_exit();
	MWprintf ( 50, "After pvm_exit\n" );
	::exit(exitval);
}

int  
MWPvmRC::setup( int argc, char *argv[], int *id, int *master_id ) {
	
	MWprintf ( 10, "In MWPvmRC::setup()\n");

	*id = pvm_mytid();
	MWprintf(10, "MyPid is %d(%x)\n", *id, *id );

	if ( ( *master_id = pvm_parent() ) == PvmNoParent ) {
        MWprintf ( 40, "I have no parent, therefore I'm the master.\n" );
		is_master = TRUE;
		*master_id = 0;
    } else {
		is_master = FALSE;
		return 0;
	}
    
	pvm_catchout( stderr );
		/* We tell PVM to tell us when hosts are added.  We will be sent
		   a message with tag HOSTADD.  The -1 turns the notification on.
		*/
	pvm_notify ( PvmHostAdd, HOSTADD, -1, NULL );

	return 0;
}

int 
MWPvmRC::config( int *nhosts, int *narches, MWWorkerID ***workers ) {

	if ( !is_master ) {
		MWprintf ( 10, "Slaves not allowed in this privaledged call!\n" );
		return -1;
	}

	struct pvmhostinfo *hi= NULL;
	int r;
	
	MWprintf ( 70, "In MWPvmRC::config()\n" );

	if ( *workers ) {
		MWprintf ( 10, "workers should be NULL when called.\n" );
		return -1;
	}

	if ( (r = pvm_config ( nhosts, narches, &hi ) ) < 0 ) {
		MWprintf ( 10, "Return value of %d from pvm_config!\n", r );
		return -1;
	}
	
	(*workers) = new MWWorkerID*[*nhosts];
	for ( int i=0 ; i<*nhosts ; i++ ) {
		(*workers)[i] = new MWWorkerID;
		(*workers)[i]->set_id1( hi[i].hi_tid & PVM_MASK );
		(*workers)[i]->set_arch ( atoi ( hi[i].hi_arch ) );
			/* Do domething creative with speed later! */
	}

	return 0;
} 

int
MWPvmRC::start_worker ( MWWorkerID *w ) {

	if ( !is_master ) {
		MWprintf ( 10, "Slaves not allowed in this privaledged call!\n" );
		return -1;
	}

	MWprintf ( 50, "In MWPvmRC::start_worker()\n" );
	if ( !w ) {
		MWprintf ( 10, "w cannot be NULL!\n" );
		return -1;
	}

		/* First, we will unpack some goodies that pvm gave us: */
    int val;
    char name[64], arch[64];
    int status, dsig;

		/* The following is packed by the multishadow along with the 
		   HOSTADD message.  We unpack it here to put into the MWWorkerID */
    unpack ( &val, 1, 1 );
    unpack ( name );
    unpack ( arch );
    unpack ( &status, 1, 1 );
	unpack ( &dsig, 1, 1 );  // new in 3.4.1
    val &= PVM_MASK;   // remove that annoying high bit
    MWprintf ( 40, "Received a host: " );
    MWprintf ( 40, "Host %x added (%s), arch: %s, status:%d\n", 
			   val, name, arch, status );

	w->set_id1( val & PVM_MASK );
	w->set_arch( atoi(arch) );

	// 9/5/00 Changed by Jeff Linderoth at Jim Basney's suggestion.
	w->set_machine_name( name );

		/* We can do this now because the request will go down regardless
		   of whether or not the spawn succeeds... */
	hostadd_reqs[w->get_arch()]--;
	return do_spawn( w );
}

int
MWPvmRC::init_beginning_workers( int *nworkers, MWWorkerID ***workers ) {

	if ( !is_master ) {
		MWprintf ( 10, "Slaves not allowed in this privaledged call!\n" );
		return -1;
	}

	int i, narches;
	MWprintf ( 50, "In MWPvmRC::init_beginning_workers\n" );

		/* config allocates memory for workers */
	if ( config( nworkers, &narches, workers ) < 0 ) {
		return -1;
	}
	
		/* num_arches is a member of MWCommRC */
	if ( (num_arches != -1) && (num_arches != narches) ) {
			/* We were told by someone that we had the wrong number of 
			   arch classes.  Complain here. */
		MWprintf ( 10, "num_arches was set to %d, pvm just told us %d!\n", 
				   num_arches, narches );
		return -1;
	}
	num_arches = narches;

    MWprintf ( 40, "Number of hosts at start: %d.\n", *nworkers );
    MWprintf ( 40, "Number of arch. classes:  %d.\n", num_arches );

	hostadd_reqs = new int[num_arches];
	for ( i=0 ; i<num_arches ; i++ ) {
		hostadd_reqs[i] = 0;
	}

	for ( i=0 ; i<(*nworkers) ; i++ ) {
		do_spawn( (*workers)[i] );
	}
	
	return 0;
}

int
MWPvmRC::do_spawn ( MWWorkerID *w ) {

	int spawned_tid;
	char arch[4];
	int archnum = w->get_arch();
	sprintf ( arch, "%d", archnum );

	if ( worker_executables[archnum][0] == '\0' ) {
		MWprintf( 10, "No worker executable defined for arch %d!\n", archnum );
		return -1;
	}

	MWprintf ( 40, "About to call pvm_spawn (%s)...\n", 
			   worker_executables[archnum] );
	
#if ! defined( STANDALONE )
    int status = pvm_spawn( worker_executables[archnum], 
							NULL, 2, arch, 1, &spawned_tid );
#else    
    int status = pvm_spawn( worker_executables[archnum], 
							NULL, 1, "wheel", 1, &spawned_tid );
#endif
	
	if ( status == 1 ) {
        MWprintf ( 40, "Success spawning as tid %x.\n", spawned_tid );
        
		w->set_id2( spawned_tid & PVM_MASK );
		w->set_id1( pvm_tidtohost( spawned_tid ) & PVM_MASK );

		setup_notifies ( spawned_tid );

		return spawned_tid;
	}
	else if ( status <= 0 ) {
        MWprintf ( 40, "Huh? Error: status:%d, tid:%d\n", 
				   status, spawned_tid );
		MWprintf ( 40, "There was a problem spawning on %x.\n", w->get_id1() );

			/* We make sure that id2 is -1, signifying error! */
		w->set_id2( -1 );
		return -1;
    }
	return -1;
}

int
MWPvmRC::removeWorker ( MWWorkerID *w ) {
	char *machine = w->machine_name();
	int info;
	MWprintf ( 70, "Removing worker %s.\n", machine );
	int retval = pvm_delhosts( &machine, 1, &info );
	if ( retval < 0 ) {
		MWprintf ( 10, "pvm_delhosts returns an error of %d!\n", retval );
	}
	if ( info < 0 ) {
		MWprintf ( 10, "pvm_delhosts of %s has error %d.\n", machine, info );
	}
	delete w;
	return retval;
}

int
MWPvmRC::hostaddlogic( int *num_workers ) {

		/* This awfully complex function determines the conditions 
		   under which it being called, and then calls pvm_addhosts()
		   asking for the correct number of hosts.
		   
		   - num_workers - an array of size num_arches that contains 
		      the number of workers in each arch class.
		   - hostadd_reqs: The number of outstanding HOSTADDs out there
		   - target_num_workers: Set by the user...
		*/

		/* If we're in STANDALONE mode, we never ask for hosts,
		   so we basically ignore this whole function: */
#if !defined (STANDALONE)

	if ( !hostadd_reqs ) {
			/* if this doesn't exist yet, we won't do anything */
		return 0;
	}

		/* number of hosts to ask for at a time. The idea is that we'll
		   have double this outstanding at a time - and this amounts 
		   to 12 for 1, 2, or 3 arch classes. */

	//const int HOSTINC = 6 / num_arches;   

	// Jeff changed this -- we always ask for six extra for each arch class...
	const int HOSTINC = 6;

	int i;
	int *howmany = new int[num_arches];  // how many to ask for
	int total_workers = 0;
	int total_reqs = 0;
	int total_howmany = 0;

	MWprintf ( 60, "hal: target: %d.\n", target_num_workers );

	for ( i=0 ; i<num_arches ; i++ ) {
		howmany[i] = 0;
		MWprintf ( 60, "hal: %d workers in class %d\n",
				   num_workers[i], i );
		MWprintf ( 60, "   -- outstanding reqs: %d\n", hostadd_reqs[i] );
		total_workers += num_workers[i];
		total_reqs += hostadd_reqs[i];
	}

	MWprintf ( 30, "hal: tot_w: %d, tot_r: %d\n", total_workers, total_reqs );
		
		/* We have a few goals to balance: we want to keep around 10 
		   outstanding requests (per arch), but we only really want to 
		   ask for hosts in increments of HOSTINC.  Asking for 1 each time 
		   involves a LOT of communication with the schedd.  So...we'll 
		   ask for HOSTINC*2 at the start, and then every HOSTINC we get 
		   we'll ask for HOSTINC more. */

		/* If we're at our target, we're done!  In reality, this is
		   going to happen VERY RARELY! */
	if ( total_workers + total_reqs == target_num_workers ) {
		MWprintf ( 60, "hal: at target!\n" );
		delete [] howmany;
		return 0;
	}

	if ( total_workers > target_num_workers ) {
		MWprintf ( 60, "hal: We've got %d workers, we only need %d!\n", 
				   total_workers, target_num_workers );
		delete [] howmany;
		return (total_workers - target_num_workers);
	}

	for ( i=0 ; i<num_arches ; i++ ) {
			/* If we're behind, kick start it. */
		if ( hostadd_reqs[i] < HOSTINC ) {
			howmany[i] = HOSTINC*2;
			MWprintf ( 30, "hal: low: (%d<%d), asking for %d, class \"%d\"\n", 
					   hostadd_reqs[i], HOSTINC, howmany[i], i );
		}
			/* Need to ask for more... */
		else if ( hostadd_reqs[i] == HOSTINC ) {
			howmany[i] = HOSTINC;
			MWprintf ( 30, "hal: time to add, asking for %d, class \"%d\"\n", 
					   howmany[i], i );
		} 
		else {
			MWprintf ( 30, "hal: (%d>%d), doing nada\n", 
					   hostadd_reqs[i], HOSTINC );
		}
		total_howmany += howmany[i];
	}

		/* Shoot, this one's easy... */
	if ( total_howmany == 0 ) {
		MWprintf ( 30, "hal: We don't need to add hosts...\n" );
		delete [] howmany;
		return 0;
	}

		/* Now that we think we know how many to ask for, we have to 
		   be sure not to overshoot our targets.  */
	if ( target_num_workers < total_workers+total_howmany+total_reqs ) {
		int desired = target_num_workers - (total_workers+total_reqs);
		MWprintf ( 30, "hal: overshot by %d.\n", total_howmany - desired );
		MWprintf ( 30, "hal: %d = %d - (%d + %d);\n", desired, 
				   target_num_workers, total_workers, total_reqs );
		
			/* Rebalance!  We'll start with what we've got, and add
			   numbers to the arch class with the least workers. */
		total_howmany = 0;
		int *bigt = new int[num_arches];
		for ( i=0 ; i<num_arches ; i++ ) {
			bigt[i] = num_workers[i] + hostadd_reqs[i];
			howmany[i] = 0;
		}
		while ( desired > 0 ) {
			int dest = min ( bigt, num_arches );
			bigt[dest]++;
			howmany[dest]++;
			desired--;
		}
		delete [] bigt;

		MWprintf ( 30, "hal: final rebalancing mess:\n" );
		for ( i=0 ; i<num_arches ; i++ ) {
			MWprintf ( 30, "hal: arch %d gets %d.\n", i, howmany[i] );
		}
	}

		/* We're FINALLY ready to do the deed! */
	for ( i=0 ; i<num_arches ; i++ ) {
		if ( howmany[i] > 0 ) {
			int status = ask_for_host( howmany[i], i );
			if ( status > 0 ) {
				hostadd_reqs[i] += howmany[i];
			}
		}
	}

	delete [] howmany;

#endif /* ifndef STANDALONE */
	return 0;

}

int 
MWPvmRC::min( int *arr, int len ) {
		// return index of min element in array of length len.
	int min = 0;
	for ( int i=1 ; i<len ; i++ ) {
		if ( arr[i] < arr[min] ) {
			min = i;
		}
	}
	return min;
}

int
MWPvmRC::setup_notifies ( int task_tid ) {
		/* Takes a *task* tid, and sets up all notifies at once. */

	MWprintf ( 40, "Setting up notifies for tid %x.\n", task_tid );
	int pvmd_tid = pvm_tidtohost( task_tid );
	
    pvm_notify ( PvmHostDelete, HOSTDELETE, 1, &pvmd_tid );
	pvm_notify ( PvmTaskExit, TASKEXIT, 1, &task_tid );
#if !defined( STANDALONE )
    pvm_notify ( PvmHostSuspend, HOSTSUSPEND, 1, &pvmd_tid );
	pvm_notify ( PvmHostResume,  HOSTRESUME, 1, &pvmd_tid );
#endif

	return 0;
}

int 
MWPvmRC::ask_for_host( int howmany, int arch ) {

	int status;
	MWprintf ( 50, "Asking for %d host(s) with arch %d\n", howmany, arch );
	
	char **arches = new char*[howmany];
	for ( int i=0 ; i<howmany ; i++ ) {
		arches[i] = new char[4];
		sprintf( arches[i], "%d", arch );
	}
	int *infos = new int[howmany];
	
	status = pvm_addhosts ( arches, howmany, infos );
	
	for ( int j=0 ; j<howmany ; j++ ) {
		delete [] arches[j];
	}
	delete [] arches;
	delete [] infos;

	if ( status < 0 ) {
		MWprintf ( 10, "pvm_addhosts returns %d, may not be able to "
				   "aquire requested resources...\n", status );
	} else {
		MWprintf ( 90, "pvm_addhosts called with status = %d\n", status );
	}
    
	return status; // JGS
}	

void 
MWPvmRC::conf() {

    int num_curr_hosts, num_curr_arch;

    struct pvmhostinfo *hi;

        // Foolish pvm_config allocs its own memory...
    pvm_config ( &num_curr_hosts, &num_curr_arch, &hi );

    MWprintf ( 40, "* Pvm says that there are %d machines and %d arches.\n", 
             num_curr_hosts, num_curr_arch );
    
    MWprintf ( 40, "* These machines have the following configurations:\n" );
    MWprintf ( 40, "* tid\t\tname\t\t\tarch\tspeed\n" );
    MWprintf ( 40, "* ---\t\t----\t\t\t----\t-----\n" );

    for ( int i=0 ; i<num_curr_hosts ; i++ ) {
        MWprintf ( 40, "* %x\t%s\t%s\t%d\n", hi[i].hi_tid, hi[i].hi_name, 
                 hi[i].hi_arch, hi[i].hi_speed );
    }

    MWprintf ( 40, "\n" );
}


int 
MWPvmRC::read_RMstate ( FILE *fp = NULL )
{
    // No Pvm specific functions
    return 0;
}


int 
MWPvmRC::write_RMstate ( FILE *fp = NULL )
{
    // No Pvm specific functions
    return 0;
}
