#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <iostream.h>
#include <fstream.h>
#include <assert.h>
#include <stdarg.h>
#include <errno.h>

#include "MW.h"
#include "MWDriver.h"
#include "MWWorkerID.h"
#include "MWWorker.h"
#include "MWTask.h"

#ifdef FILE_COMM

	#include "CommRM/MW-File/MWFileRC.h"
	/* Our global Resource Manager and Communicator. */
	MWRMComm * MWDriver::RMC = new MWFileRC ( TRUE, 0 );

#endif

#ifdef INDEPENDENT

	#include "CommRM/MW-Independent/MWIndRC.h"
	MWRMComm *GlobalRMComm = new MWIndRC ( );
	MWRMComm * MWDriver::RMC = GlobalRMComm;

#endif

#ifdef PVM

	#include "CommRM/MW-PVM/MWPvmRC.h"
	/* Our global Resource Manager and Communicator. */
	MWRMComm * MWDriver::RMC = new MWPvmRC;

#endif

#ifndef INDEPENDENT
	MWRMComm * MWTask::RMC = MWDriver::RMC;
#endif

MWDriver::MWDriver() 
{
    todo = NULL;
    todoend = NULL;
    running = NULL;
    runningend = NULL;
    workers = NULL;
    
    task_counter = 0;
    
    checkpoint_frequency = 0;
	checkpoint_time_freq = 0;
	next_ckpt_time = 0;
    num_completed_tasks = 0;
	bench_task = NULL;

    // These are for list management
    addmode = ADD_AT_END;
    getmode = GET_FROM_BEGIN;
    machine_ordering_policy = NO_ORDER;
    listsorted = false;
    task_key = NULL;
    worker_key = NULL;

    ckpt_filename = "checkpoint";

	suspensionPolicy = DEFAULT;

}


MWDriver::~MWDriver() 
{
    if ( todo ) {
        MWTask *t = todo;
        MWTask *p = todo;
        while ( t ) {
            t = t->next;
            delete p;
            p = t;
        }
    }
    
    if ( running ) {
        MWTask *t = running;
        MWTask *p = running;
        while ( t ) {
            t = t->next;
            delete p;
            p = t;
        }
    }
    
    if ( workers ) {
		
        MWWorkerID *w = workers;
        MWWorkerID *p = workers;
        while ( w ) {
            w = w->next;
            delete p;
	    
            p = w;
        }
    }

	if ( bench_task ) {
		delete bench_task;
	}
}

void 
MWDriver::go( int argc, char *argv[] ) {

	MWReturn ustat;

    // do some setup when we start:
	
    ustat = master_setup( argc, argv );

    if ( ustat != OK ) 
    {
	RMC->exit(1);
	return; 
    }
    
	// the main processing loop:
    ustat = master_mainloop ();
	
    if( ustat == OK || ustat == QUIT ) 
    {
	// We're done - now print the results.
	printresults();

	// tell the stats class to do its magic
	stats.makestats();
    }
	
		// remove checkpoint file.
    struct stat statbuf;
    if ( stat( ckpt_filename, &statbuf ) == 0 ) 
    {
	if ( unlink( ckpt_filename ) != 0 ) 
	{
		MWprintf ( 10, "unlink %s failed! errno %d\n", 
					   ckpt_filename, errno );
	}
    }

	/* Does not return. */
    RMC->exit(0);
}

MWReturn 
MWDriver::master_setup( int argc, char *argv[] ) 
{
    char wd[_POSIX_PATH_MAX];
    MWReturn ustat = OK;
    struct stat statbuf;
    int myid, master_id;

    MWprintf ( 70, "MWDriver is pid %ld.\n", getpid() );
    
    if ( getcwd( wd, 100 ) == NULL ) 
    {
	MWprintf ( 10, "getcwd failed!  errno %d.\n", errno );
    }
	
    MWprintf ( 70, "Working directory is %s.\n", wd );

    RMC->setup( argc, argv, &myid, &master_id );
    
    if ( stat( ckpt_filename, &statbuf ) == -1 ) 
    {
            // checkpoint file not found - start from scratch:
        MWprintf ( 50, "Starting from the beginning.\n" );

        ustat = get_userinfo( argc, argv );
	if( ustat == OK ) 
	{
	    ustat = create_initial_tasks();
	} 
	else 
	{
	    return ustat;
	}
    }
    else 
    {  // restart from that checkpoint file.
        MWprintf ( 50, "Restarting from a checkpoint.\n" );
        restart_from_ckpt();
    }

    MWprintf ( 70, "Good to go.\n" );
		
    int num_curr_hosts, i;
    MWWorkerID **workers = NULL;

	/* Memory for workers allocated in here: */
    RMC->init_beginning_workers ( &num_curr_hosts, &workers );

    if ( workers ) 
    { /* there are indeed workers... */

	for ( i=0 ; i<num_curr_hosts ; i++ ) 
	{
	    if ( workers[i]->get_id2() == -1 ) 
	    {
		/* There was a problem with this worker! */
		workers[i]->started();
		workers[i]->ended();
		stats.gather( workers[i] );
	    } 
	    else 
	    {
		/* Success! */
		/* Memory now taken care of by list... */
		addWorker ( workers[i] );
	    }
	}
	/* don't forget to remove the array of ptrs .. */
	delete [] workers;
    }

    return OK;
}

MWReturn 
MWDriver::create_initial_tasks() 
{    
    int orig_ntasks;
    MWTask **orig_tasks;
    
    MWReturn ustat = setup_initial_tasks( &orig_ntasks, &orig_tasks );
    if( ustat == OK ) 
    {
	addTasks( orig_ntasks, orig_tasks );    
    }
    else 
    {
	MWprintf( 10, "setup_initial_tasks() returned %d\n", ustat );
    }
    delete [] orig_tasks;

    return ustat;
}

MWReturn 
MWDriver::master_mainloop() 
{

    call_hostaddlogic();

#ifdef INDEPENDENT

    ControlPanel ( );
    return OK;
}

MWReturn
MWDriver::master_mainloop_ind ( int buf_id, int sending_host )
{
    MWWorkerID *w;
    MWReturn ustat = OK;
    int len, tag, info;

#else
    int buf_id;
    int info, len, tag;
    int sending_host;
    MWWorkerID *w;
	
    MWReturn ustat = OK;

        // while there is at least one job in the todo or running queue:
    while ( ( (todo != NULL) || (running != NULL) ) && ( ustat == OK ) ) 
    {

#endif

            // get any message from anybody
        buf_id = RMC->recv( -1, -1 );


        if ( buf_id < 0 ) 
	{
            MWprintf ( 10, "Ack, Problem with RMC->recv!\n" );
#ifdef INDEPENDENT
	    return QUIT;
#else
	    continue;
#endif
        }

        info = RMC->bufinfo ( buf_id, &len, &tag, &sending_host );

        switch ( tag ) {
			
        case INIT: 
		    if ( (w = lookupWorker ( sending_host )) == NULL ) 
		    {
			MWprintf ( 10, "We got an INIT message from a worker (%08x) "
					   "who we don't have records for.\n", sending_host);
			break;
		    }
		    ustat = worker_init( w );
		    break;

	case BENCH_RESULTS:
		    if ( (w = lookupWorker ( sending_host )) == NULL ) 
		    {
			MWprintf ( 10, "We got an BENCH_RESULTS message from (%08x), "
						   "who we don't have records for.\n", sending_host);
			break;
		    }
			ustat = handle_benchmark( w );
		    if ( ustat == OK ) 
		    {
			printWorkers();
			send_task_to_worker( w );
		    } else 
		    {
			ustat = OK;  // we don't want to shut down...
		    }
		    break;

        case RESULTS:
		    if ( (w = lookupWorker ( sending_host )) == NULL ) 
		    {
			MWprintf ( 10, "We got a RESULTS message from a worker (%08x) "
					   "who we don't have records for.\n", sending_host);
			break;
		    }
		    ustat = handle_worker_results ( w );

		    if ( w->is_doomed() ) 
		    {
			/* This worker has been marked for removal */
			w->ended();
			stats.gather( w );
			RMC->removeWorker( w );
		    } 
		    else 
		    {
			if ( ustat == OK ) 
			{
				send_task_to_worker( w );
			}
		    }
		    break;

        case HOSTADD: {
            MWWorkerID *w = new MWWorkerID;
			
			int r=0;
			r = RMC->start_worker( w );
			if ( r < 0 ) {
					/* Still have to take care of w... */
				w->started(); /* So we know that we *tried* to start a */
				w->ended();   /* host, and failed. */
				stats.gather ( w );
			} else {

			  addWorker( w );
			}
			
			call_hostaddlogic();
            break;
        }
        case HOSTDELETE: {
            handle_hostdel();
            break;
        }
        case HOSTSUSPEND: {
            handle_hostsuspend();
            break;
        }
        case HOSTRESUME: {
            handle_hostresume();
            break;
        }
        case TASKEXIT: {
            handle_taskexit();
            break;
        }
        default: {
            MWprintf ( 10, "Unknown message %d.  What the heck?\n", tag );
		}
        } // switch
		
	/* Jeff's addition.  At this point, we need to do some assigning of
	   tasks.  If many tasks are added all in a batch, then many 
	   "SEND_WORK" commands are ignored, and the workers sit
	   idle.  Checking again here whether or not there are workers
	   waiting (or suspended) is a good idea.
	*/

		rematch_tasks_to_workers( w );
		
#if defined( SIMULATE_FAILURE )
			// Fail with probability p
		const double p = 0.0025;
		
		if( drand48() < p ) {
			MWprintf ( 10, "Simulating FAILURE!\n" );
			kill_workers();
			RMC->exit(1);
		}
#endif	

#ifndef INDEPENDENT

	}		

	if( ustat != OK ) {
		MWprintf( 10, "The user signalled %d to stop execution.\n", ustat );
	}
	MWprintf ( 10, "The MWDriver is done.\n" );
	
	kill_workers();
#endif
	
    return ustat;
}

MWReturn 
MWDriver::worker_init( MWWorkerID *w ) 
{  
    char workerhostname[64];
	MWReturn ustat = OK;

		/* We pack the hostname on the worker side... */
    RMC->unpack( workerhostname );
    
    MWprintf ( 40, "The MWDriver recognizes the existence of worker \"%s\".\n",
             workerhostname );
	
	w->set_machine_name( workerhostname );
	unpack_worker_initinfo( w );

	w->get_machine_info();

    int stat = RMC->initsend();
    if( stat < 0 ) {
        MWprintf( 10, "Error in initializing send in pack_worker_init_data\n");
    }
    
    if ( (ustat = pack_worker_init_data()) != OK ) {
		MWprintf ( 10, "pack_worker_init_data returned !OK.\n" );
		w->printself(10);
		w->started();
		w->ended();
		return ustat;
	}

	MWTask *b = get_benchmark_task();
	int benchmarking = b ? TRUE : FALSE;
	
	RMC->pack( &benchmarking, 1 );
	if ( b ) {
		b->pack_work();
	}

	RMC->send( w->get_id2(), INIT_REPLY );
	// Note the fact that the worker is doing a benchmark. 
	w->benchmark();
    return ustat;
}

MWReturn
MWDriver::handle_benchmark ( MWWorkerID *w ) {
	int okay = TRUE;
	double bres = 0.0;

	RMC->unpack( &okay, 1 );
	if ( okay == -1 ) {
			/* Very bad! */
		MWprintf ( 10, "A worker failed to initialize!\n" );
		w->printself(10);
		w->started();
		w->ended();
		stats.gather ( w );
		rmWorker ( w->get_id1() );
		RMC->removeWorker ( w );
		return QUIT;
	} 

	RMC->unpack( &bres, 1 );
	w->set_bench_result( bres );
	w->benchmarkOver();

	//JTL Now you need to order the workerlist
	sort_worker_list();

	w->started();
	return OK;
}

void 
MWDriver::kill_workers() {

	MWprintf( 40, "Killing workers:\n");

    MWWorkerID *w = workers;
	MWWorkerID *t;
    
    while ( w ) {

        RMC->send ( w->get_id2(), KILL_YOURSELF );
		MWprintf ( 40, "  %08x\n", w->get_id2() );

		t = w->next;
		w->printself( 50 );
		w->ended();
		stats.gather( w );
		RMC->removeWorker( w );
		delete w;  // comment this if you uncomment above line!
		w = t;
    }
}

/* XXX add arches to make interface better. */
void 
MWDriver::set_target_num_workers( int iWantThisMany ) {
	if ( iWantThisMany == -1 ) { 
		iWantThisMany = 0x7fffffff; /* MAXINT */
	}
	RMC->set_target_num_workers( iWantThisMany, 0 );
	int na = RMC->get_num_arches();
	if ( na == -1 ) { 
		return; // We're at start; no need to continue on!
	}
	call_hostaddlogic();
}

void 
MWDriver::set_target_num_workers( int *iWantThisMany, int narches ) {
	for ( int i=0 ; i<narches ; i++ ) {
			/*later*/
	}	
}

void 
MWDriver::send_task_to_worker( MWWorkerID *w ) {

  /* Jeff added this!  Due to the "rematch" function,
   * it can be the case that a worker who asks for work might
   * already have some work, in which case we don't want to give
   * him another task.  (If he goes down then only one of the tasks
   * will be rescheduled.
   */

    if( w->currentState() != IDLE ) {
		return;
    }

		/* End of Jeff's kludgy fix */

    MWTask *t = getNextTask();
    if ( t != NULL ) {

        RMC->initsend();
        RMC->pack( &(t->number), 1, 1 );
        
		pack_driver_task_data();

        t->pack_work();
        
        RMC->send( w->get_id2(), DO_THIS_WORK );
        
        w->gottask( t->number );
        w->runningtask = t;
        t->worker = w;
        putOnRunQ( t );
        
        MWprintf ( 10, "Sent task %d to %s.\n", 
				   t->number, w->machine_name() );
        
    }
    else {
        /* this added to make things work better at program's end:
           The end result will be that a suspended worker will have its
           task adopted.  We first look for a task with a suspended worker.
           We call it t.  The suspended worker's 'runningtask' pointer 
           will still point to t, but t->worker will point to this
           worker (w).  Now there will be two workers working on the 
           same task.  The first one to finish reports the results, and
           the other worker's results get discarded when (if) they come in.
        */

        if ( running ) {
            // here there are no tasks on the todo list, but there are 
            // tasks in the run queue.  See if any of those tasks are
            // being held by a suspended worker.  If so, adopt that task.
            for ( t=running ; t != NULL ; t = t->next ) {
                if ( t->worker->isSusp() ) {
                    MWprintf ( 60, "Re-assigning task %d to %s.\n", 
                             t->number, w->machine_name() );
					t->worker->printself( 60 ); // temp
					printWorkers(); // temp
                    w->gottask( t->number );
                    t->worker = w;
                    w->runningtask = t;   
                    // note that old wkr->runningtask points to t also.
                    
                    RMC->initsend();
                    RMC->pack ( &(t->number), 1, 1 );
					pack_driver_task_data();
                    t->pack_work();
                    RMC->send ( w->get_id2(), DO_THIS_WORK );
                    break;
                }
            }
        }
    }
}

void 
MWDriver::handle_hostdel () {
    
        // Here we find out a host has gone down.
	
		// this tells us who died:
	int who;
	RMC->unpack ( &who, 1, 1 );
	
	MWWorkerID *w = rmWorker( who );
	if ( w == NULL ) {
		MWprintf ( 40, "Got HOSTDELETE of worker %08x, but it isn't in "
				   "the worker list.\n", who );
	} else {
		
		MWprintf ( 40, "Got HOSTDELETE!  %s (%x) went down.\n", 
				   w->machine_name(), who );   
		hostPostmortem ( w );
			/* We may have to ask for some hosts at this time... */
		delete w;
		call_hostaddlogic();
	}
}

void 
MWDriver::handle_hostsuspend () 
{

	 
	int who;
    RMC->unpack ( &who, 1, 1 );

    MWWorkerID *w = lookupWorker ( who );
    if ( w != NULL ) {
        w->suspended();
        MWprintf ( 40, "Got HOSTSUSPEND!  \"%s\" is suspended.\n", 
				   w->machine_name() );   

        int task;
        if ( w->runningtask ) {
            task = w->runningtask->number;
            MWprintf ( 60, " --> It was running task %d.\n", task );

			switch ( suspensionPolicy ) {
			case REASSIGN: {
					// remove task from running list; push on todo.

					/* First see if it's *on* the running list.  If it's 
					   not, then it's either back on the todo list or
					   its already done.  If this is the case, ignore. */
				MWTask *rmTask = rmFromRunQ( task );
				if ( !rmTask ) {
					MWprintf ( 60, "Not Re-Assigning task %d.\n", task );
					break;
				}

				MWTask *rt = w->runningtask;
				if ( rt != rmTask ) {
					MWprintf ( 10, "REASSIGN: something big-time screwy.\n");
				}

					/* Now we put that task on the todo list and rematch
					   the tasks to workers, in case any are waiting around.*/
				pushTask( rt );
					// just in case...
				rematch_tasks_to_workers ( w );
				break;
			}
			case DEFAULT: {
					// we only take action if the todo list is empty.
				if ( todo == NULL ) {
						// here there are no more tasks left to do, we want
						// to re-allocate this job.  First, find idle worker:
					for ( MWWorkerID *o = workers ; o != NULL ; o = o->next){
						if ( o->isIdle() ) {
							MWprintf ( 60, "Reassigning task %d. to %s\n", 
									   task, o->machine_name() );
							w->runningtask->worker = o;
							o->runningtask = w->runningtask;
							o->gottask( o->runningtask->number );
							
							RMC->initsend();
							RMC->pack ( &(o->runningtask->number), 1, 1 );
							pack_driver_task_data();
							o->runningtask->pack_work();
							RMC->send ( o->get_id2(), DO_THIS_WORK );
							
							break;
						}
					}
				}
			}
			} // switch
		}
		else {
			MWprintf ( 60, " --> It was not running a task.\n" );
		}
    }
    else {
		MWprintf ( 40, "Got HOSTSUSPEND of worker %08x, but it isn't in "
				   "the worker list.\n", who );
    }
}

void 
MWDriver::handle_hostresume () {


/* Do nothing; applaud politely */

	int who;
    RMC->unpack ( &who, 1, 1 );

    MWWorkerID *w = lookupWorker ( who );
    if ( w != NULL ) {
        w->resumed();
        MWprintf ( 40, "Got HOSTRESUME.  \"%s\" has resumed.\n", 
				   w->machine_name() );
        if ( w->runningtask != NULL ) {
            MWprintf ( 60, " --> It's still running task %d.\n", 
					   w->runningtask->number );
        }
        else {
            MWprintf ( 60, " --> It's not running a task.\n" );
        }
	}
    else {
		MWprintf ( 40, "Got HOSTRESUME of worker %08x, but it isn't in "
				   "the worker list.\n", who );
    }
}

void 
MWDriver::handle_taskexit () {


/*   I have found that we usually get this message just before a HOSTDELETE, 
	 so I'm not going to do anything like remove that host...
*/

	int who;
    RMC->unpack ( &who, 1, 1 );

        // search workers for that id2.
    MWWorkerID *w = rmWorker ( who );
    if ( w == NULL ) {
		MWprintf ( 40, "Got TASKEXIT of worker %08x, but it isn't in "
				   "the worker list.\n", who );
        return;
    }

	MWprintf ( 40, "Got TASKEXIT from machine %s (%x).\n", 
			   w->machine_name(), w->get_id1());

	hostPostmortem( w );
	delete w;
		/* We may have to ask for some hosts at this time... */
	call_hostaddlogic();
}

void 
MWDriver::hostPostmortem ( MWWorkerID *w ) {

	MWTask *t;

	w->ended();
	t = w->runningtask;
	if ( t == NULL ) {
		MWprintf ( 60, " %s (%x) was not running a task.\n", 
				 w->machine_name(), w->get_id1() );
	}
	else {
		MWprintf ( 60, " %s (%x) was running task %d.\n", 
				 w->machine_name(), w->get_id1(), t->number );
			// remove that task from the running queue...iff that
			// task hasn't ALREADY been reassigned.  

		w->runningtask = NULL;
		MWWorkerID *o;

		if ( !(o = task_assigned( t )) ) {
			MWTask *check = rmFromRunQ( t->number );
			if ( t == check ) {
				t->worker = NULL;
				pushTask( t );
			}
		}		
		else {
			t->worker = o;
			MWprintf ( 60, " Task %d already has been reassigned\n", 
					   t->number );
		}
	}
	stats.gather( w );
}
		
MWReturn 
MWDriver::handle_worker_results( MWWorkerID *w ) 
{
	MWReturn ustat = OK;

    int tasknum;  
	double wall_time = 0;
	double cpu_time = 0;
    
    RMC->unpack( &tasknum, 1, 1 );
	RMC->unpack( &wall_time, 1, 1 );
	RMC->unpack( &cpu_time, 1, 1 );

    MWprintf ( 60, "Unpacked task #%d result from %s.\n", 
			   tasknum, w->machine_name());

	MWprintf ( 60, "It took %5.2f secs wall clock and %5.2f secs cpu time\n", 
			   wall_time, cpu_time );

	if ( w->isSusp() ) {
		MWprintf ( 60, "Got results from a suspended worker!\n" );
	}

    MWTask *t = rmFromRunQ( tasknum );

		/* The task *could* have been suspended, REASSIGNed, and put
		   on the todo list.  Let's just check the first entry on the
		   todo list and see if it's there.  If so, remove it and handle. */
	if ( suspensionPolicy == REASSIGN && 
		 todo && 
		 (todo->number == tasknum ) ) { // found!
		t = todo;
		if ( todoend == todo ) todoend = NULL;
		todo = todo->next;
		MWprintf ( 60, "Found task as 1st element in todo instead.\n" );
	}

    if( t == NULL ) {
        MWprintf ( 60, "Task not found in todo list; assumed done.\n" );

		w->completedtask( wall_time, cpu_time );
		if ( w->runningtask ) {
			w->runningtask = NULL;
		}
    }
    else {
        // tell t the results
        t->unpack_results();
			/* give these to the task class; now the user can use them. */
		t->working_time = wall_time;
		t->cpu_time = cpu_time;

        ustat = act_on_completed_task( t );
        
        num_completed_tasks++;

        if ( (checkpoint_frequency > 0) && 
             (num_completed_tasks % checkpoint_frequency) == 0 ) {
            checkpoint();
        } else {
			if ( checkpoint_time_freq > 0 ) {
				time_t now = time(0);
				MWprintf ( 80, "Trying ckpt-time, %d %d\n", 
						   next_ckpt_time, now );
				if ( next_ckpt_time <= now ) {
					next_ckpt_time = now + checkpoint_time_freq;
					MWprintf ( 80, "About to...next_ckpt_time = %d\n", 
							   next_ckpt_time );
					checkpoint();
				}
			}
		}

        t->next = NULL;
        if ( t->worker == NULL ) {
            MWprintf ( 40, "Task had no worker...huh?\n" );
        }

		if ( w != t->worker ) {
			MWprintf ( 90, "This task not done by 'primary' worker, "
					   "but that's ok.\n" );
		} else {
			MWprintf ( 90, "Task from 'primary' worker %s.\n", 
					   w->machine_name() );
		}
		
		w->runningtask = NULL;
		w->completedtask( wall_time, cpu_time );
		t->worker = NULL;

        if( ! task_assigned( t ) ) {
            MWprintf( 80, "Deleting task %d\n", t->number );
            delete t;
        }
    }

    return ustat;
}

void 
MWDriver::rematch_tasks_to_workers( MWWorkerID *nosend )
{

  // For each worker that is "waiting for work", you can safely(?)
  // now do a send_task_to_worker.  Don't send it to the id "nosend"
  // (He just reported in).  
   
    MWWorkerID *w = workers;
	int ns = nosend ? nosend->get_id2() : 0;
    
    while( w ) {
        if ( ( w->currentState() == IDLE ) && 
             ( w->get_id2() != ns ) )  {
            
            MWprintf ( 90, "In rematch_tasks_to_workers(), doing send...\n" );
            send_task_to_worker( w );
        }

        w = w->next;
    }
}

MWTask * 
MWDriver::getNextTask()  {
		// return next task to do.  NULL if none.
        
	if ( todo == NULL ) 
		return NULL;
	
	MWTask *t;
	switch( getmode ) {
		
	case GET_FROM_BEGIN: {
		
			// we believe the next task to lie at the head of the list.
		t = todo;    
		todo = todo->next;
		
		t->next = NULL;
		break;
	}
	case GET_FROM_KEY: {
		
		t = todo;
		
		if( task_key == NULL ) {
			MWprintf ( 10, " GET_FROM_KEY retrieval mode, "
					   "but no key function set.\n"); 
			MWprintf ( 10, " Returning task at head of list\n");
			todo = todo->next;
			t->next = NULL;
		}
		else {
			MWTask *mt = todo;
			MWTask *prevmt = NULL, *prevt;
			MWKey minval = (*task_key)( mt );
			while( t ) {
				MWKey val = (*task_key)( t );
				if( val < minval ) {
					minval = val;
					mt = t;
					prevmt = prevt;
				}
				prevt = t;
				t = t->next;
			}
			
			if( prevmt ) {
				prevmt->next = mt->next;
			}
			else {
				todo = todo->next;
			}
			
			t = mt;
			t->next = NULL;
		}
		
		break;
	}
	default: {
		MWprintf ( 10, "Unknown getmode %d\n", getmode );
		assert( 0 );
	}
	} // switch
    
	return t;
}

void 
MWDriver::pushTask( MWTask *t ) 
{

    // in this case, a task failed and needs to be done again.
    // we put it on the head of the list.
    
    if ( todo == NULL ) {
        todo = t;
        todo->next = NULL;
    }
    else {
        t->next = todo;
        todo = t;
    }
}

void 
MWDriver::addTasks( int n, MWTask **add_tasks )
{
    if ( n <= 0 ) {
        MWprintf ( 10, "Please add a positive number of tasks!\n" );
		RMC->exit(1);
    }
    
    MWTask *t;
    int na = 0;
    
    if( todo == NULL ) {
        t = add_tasks[0];
        t->next = NULL;
        todo = todoend = t;
        
        t->number = task_counter++;
        
        na = 1;      
    }
    
    switch ( addmode ) {
		
    case ADD_AT_BEGIN: {
			// Call the single add n times
		for( int i = na; i < n; i++ ) {
			addTask( add_tasks[i] );
		}
		break;
    }
	
    case ADD_AT_END: {
			// By default, things are added at the end. 
		
		while( na < n ) {
			
			t = add_tasks[na];
			t->next = NULL;
			todoend->next = t;
			todoend = t;
			
			t->number = task_counter++;
			
			na++;	  
		}
		break;
	}
    case ADD_BY_KEY: {
			// Call the single add n times
		for( int i = na; i < n; i++ )
			addTask( add_tasks[i] );
		break;
    }
    } // switch
}

void 
MWDriver::addTask( MWTask *add_task )  {

	add_task->number = task_counter++;

    if( todo == NULL ) {
        add_task->next = NULL;
        todo = todoend = add_task;
	}
    else {
		switch( addmode ) {
			
		case ADD_AT_BEGIN: {
			add_task->next = todo;
			todo = add_task;
			break;
		}
		
		case ADD_AT_END: {
				// By default, things are added at the end. 
			add_task->next = NULL;
			todoend->next = add_task;
			todoend = add_task;
			break;
		}
		
		case ADD_BY_KEY: {
				// This will insert at the first key WORSE than the 
				// given one
			if( task_key == NULL ) {
				MWprintf ( 10, "ERROR!  Adding by key, but no key "
						   "function defined!\n" );
				assert(0);
			}
			if( ! listsorted ) {
				MWprintf( 85, " Huh!  You are adding by key, but haven't "
						  "sorted the list\n" );
				MWprintf( 85, " You're the boss, but I question the "
						  "integrity of your algorithm\n");
			}
			MWTask *t = todo;
			MWTask *prevt = NULL;
			while( t ) {
				if( (*task_key)( add_task ) < (*task_key)( t ) ) {
					if( prevt == NULL ) {
						add_task->next = todo;
						todo = add_task;
					}
					else {
						prevt->next = add_task;
						add_task->next = t;
					}
					break;
				}
				prevt = t;
				t = t->next;
			}
			
			if( t == NULL ) {
					// This task is the worst!  Add it at the end.
				add_task->next = NULL;
				todoend->next = add_task;
				todoend = add_task;
			}
			
			break;
		}
		default: {
			MWprintf ( 10, "What the heck kinda addmode is %d?\n", addmode );
			assert( 0 );
		}
		} // switch        
    }
}

void 
MWDriver::set_task_key_function( MWKey (*t)( MWTask * ) ) {
	if( t != task_key )
		listsorted = false;
	
	task_key = t;
}

int 
MWDriver::set_task_add_mode( MWTaskAdditionMode mode )
{
	int errcode = 0;
	if( mode < ADD_AT_END || mode > ADD_BY_KEY ) {
		errcode = UNDEFINED;
		MWprintf( 10, " Bad Add Task Mode.  Switching to ADD_AT_END\n");
		addmode = ADD_AT_END;
    }
	else {
		addmode = mode;
    }
	
	return errcode;
}

int 
MWDriver::set_task_retrieve_mode( MWTaskRetrievalMode mode )
{

	int errcode = 0;
	if( mode < GET_FROM_BEGIN || mode > GET_FROM_KEY ) {
		errcode = UNDEFINED;
		MWprintf( 10, " Bad Retrive Task Mode.  "
				  "Switching to GET_FROM_BEGIN\n" );
		getmode = GET_FROM_BEGIN;
    }
	else {
		getmode = mode;
		if( ( mode == GET_FROM_KEY ) && ( task_key == NULL ) ) {
			MWprintf( 10, " Warning.  Call set_task_key() function before ");
			MWprintf( 10, " retrieving task\n");
		}
    }
	
	return errcode;
}

int 
MWDriver::set_machine_ordering_policy( MWMachineOrderingPolicy mode ) 
{
  int errcode = 0;
  if( mode < NO_ORDER || mode > BY_KFLOPS ) {
    errcode = UNDEFINED;
    MWprintf( 10, "Bad machine ordering policy %d.  Switching to NO_ORDER\n", 
	      (int) mode );
    machine_ordering_policy = NO_ORDER;
    worker_key = NULL;
  }
  else {
    MWprintf( 30, "Set machine ordering policy to %d\n", (int) mode );
    machine_ordering_policy = mode;
    if( mode == BY_USER_BENCHMARK ) {
      worker_key = &benchmark_result;
    }
    else {
      worker_key = &kflops;
    }

  }
  return errcode;
}

int 
MWDriver::print_task_keys() {

	int retcode = 0;
	
	if( task_key == NULL ) {
		retcode = UNDEFINED;
		MWprintf( 10, " No key function assigned to tasks -- Can't print\n");
	}
	else {
		MWprintf( 10, "Task Keys:\n");
		MWTask *t = todo;
		while( t ) {	  
				// One reason Jeff doen't like printf's -- can't overload
				// the output operator.  We assume that keys are doubles.
				// But it sure was easy to code MWprintf!  -Mike  :-)
			MWprintf( 10, "   %f\t(%d)\n", (*task_key)(t), t->number );
			t = t->next;
		}
	}
	
	return retcode;
}

int 
MWDriver::sort_task_list()  {

	if( task_key == NULL ) {
		MWprintf( 10, " Error.  No Key function defined.  Can't sort!\n");
		return UNDEFINED;	     
    }

		// I don't care about efficiency.  I'm doing section sort!  
	MWTask *tt, *it, *jt, *mt;

		// I need these to swap, since we don't have a doubly linked list
	MWTask *previt, *prevjt, *prevmt;
  
	it = todo;
	while( it ) {
		
		if( it == todo ) {
			previt = 0;   
		} 
		mt = it;
		MWKey mval = (*task_key)( mt );
		jt = it->next;
		prevjt = it;
		prevmt = prevjt;
		while( jt ) {
			MWKey jval = (*task_key)( jt );
			if( jval < mval ) {
				mt = jt;
				mval = jval;
				prevmt = prevjt;
			}	
				// Move along, lil' dogie
			prevjt = jt;
			jt = jt->next;
		}
		
		if( mt != it ) {
				// Swap 'em
			if( previt )
				previt->next = mt;
			else
				todo = mt;
			prevmt->next = it;
			
			tt = mt->next;
			mt->next = it->next;
			it->next = tt;
		}

			// Move along.  (We move along in m, since we swapped it with i)
		previt = mt;
		it = mt->next;
		
	}
	
	todoend = previt;
	listsorted = true;
	
	return 0;
}


int 
MWDriver::delete_tasks_worse_than( MWKey key ) {

	if( task_key == NULL ) {
		MWprintf( 10, " delete_task_worse_than:  no task key "
				  "function defined!\n");
		return UNDEFINED;
	}
	
	MWTask *t = todo;
	MWTask *prevt = NULL;
	
	if ( t == NULL )
		return 0;
	
	while( t ) {
		if( (*task_key)( t ) > key )  { // delete it
			if( prevt != NULL ) {
					// Not at beginning
				prevt->next = t->next;
				if( t == todoend )
					todoend = prevt;
				delete t;
				t = prevt->next;
			}
			else {
					// At beginning
				todo = t->next;
				if( t == todoend ) {
						// Check that something hasn't gone horribly wrong
					assert( todo == NULL );
					todoend = NULL;
				}
				delete t;
				t = todo;
			}	  
		}
		else {
			prevt = t;
			t = t->next;
		}
    }
	
	return 0;
}

// this is the same as the above fcn, without the add_task->number.
void 
MWDriver::ckpt_addTask( MWTask *add_task ) {
    if( todo == NULL ) {
        add_task->next = NULL;
        todo = todoend = add_task;
    }
    else {
        add_task->next = NULL;
        todoend->next = add_task;
        todoend = add_task;
    }
}

void 
MWDriver::putOnRunQ( MWTask *t ) 
{

    // Here's the deal:  running points to the head of the runQ, 
    // runningend points at the last element in the runQ.
    
    // we insert tasks at the end.
    
    t->next = NULL;
    
    if ( running == NULL ) {
        running = t;
        runningend = t;
    }
    else {
        runningend->next = t;
        runningend = t;
    }
}

MWTask * 
MWDriver::rmFromRunQ( int jobnum ) {

  // We search the run queue from the beginning.

    MWTask *t = running;

    if ( t == NULL )
        return NULL;

    if ( t->number == jobnum ) { // looked for job is first
        if ( t == runningend ) { // one element in list
            running = runningend = NULL;
            t->next = NULL;
            return t;
        }
        else { // > 1 element in list
            running = t->next;
            t->next = NULL;
            return t;
        }
    }

    MWTask *p = running;
    t = running->next;
    
    while ( t ) {
        if ( t->number == jobnum ) {
            if ( t == runningend ) {   // t is last elem
                runningend = p;
                p->next = NULL;
                return t;
            }
            else {  // t not last elem
                p->next = t->next;
                return t;
            }
        }
        else {
            p = t;
            t = t->next;
        }
    }

        // if you're here, you didn't find it.

    return NULL;
}

MWWorkerID * 
MWDriver::task_assigned( MWTask *t ) {
  
    MWWorkerID *w = workers;
    
    while( w ) {
        if( (w->runningtask) &&
            (w->runningtask->number == t->number) ) {

            return w;
        }      
        w = w->next;
    }

    
    return NULL;

}

bool 
MWDriver::task_in_todo_list( MWTask *t )
{
    // Before rescheduling a task on a TASKEXIT message, we need 
    // to see that the task is still in the todo list
    // Tasks NOT in the todo list have already been finished,
    // hence they should not be reassigned and this function should
    // return false.

    MWTask *tmp = todo;
    while( tmp ) {
		if( tmp == t ) {
			return true;
		}
		tmp = tmp->next;
    }
	
    return false;
}  

void 
MWDriver::printRunQ() {
    
    MWTask *t = running;
    MWprintf ( 60, "PrintRunQ start:\n" );
    while ( t ) {
        t->printself();
        t = t->next;
    }
    MWprintf ( 60, "PrintRunQ end.\n\n" );
}

void 
MWDriver::addWorker( MWWorkerID *w ) {

  // put a worker on the workers list.  Add to front.
  //  At this point, no benchmark information or machine information
  //  is known, so it is impossible to put the new machine in its proper
  //  place
  
    if ( workers == NULL ) {
        workers = w;
        w->next = NULL;
    }
    else {      
      w->next = workers;
      workers = w;
    }

}


// This could be more efficient, since we would need only insert items
//  in order to ensure that things are sorted.  But this gives us
//  more flexibility for later, and we will likely rewrite these classes anyway

void 
MWDriver::sort_worker_list()
{
  if( machine_ordering_policy == NO_ORDER || worker_key == NULL  )
    return;

  MWprintf( 70, "Before sorting worker list...\n" );
  printWorkers();


  // I don't care about efficiency.  I'm doing selection sort!  
  MWWorkerID *tt, *it, *jt, *mt;

  // I need these to swap, since we don't have a doubly linked list
  MWWorkerID *previt, *prevjt, *prevmt;
  
  it = workers;
  while( it ) {
		
    if( it == workers ) {
      previt = 0;   
    } 
    mt = it;

    MWKey mval = (*worker_key)( mt );

    jt = it->next;
    prevjt = it;
    prevmt = prevjt;
    while( jt ) {

      MWKey jval = (*worker_key)( jt );
      if( jval > mval ) {
	mt = jt;
	mval = jval;
	prevmt = prevjt;
      }	
				// Move along, lil' dogie
      prevjt = jt;
      jt = jt->next;
    }
    
    if( mt != it ) {
				// Swap 'em
      if( previt )
	previt->next = mt;
      else
	workers = mt;
      prevmt->next = it;
			
      tt = mt->next;
      mt->next = it->next;
      it->next = tt;
    }

    // Move along.  (We move along in m, since we swapped it with i)
    previt = mt;
    it = mt->next;
    
  }
		
  MWprintf( 70, "After sorting worker list...\n" );
  printWorkers();

}

MWWorkerID * 
MWDriver::lookupWorker ( int id ) {
    
        // look for a given id and return assoc. worker.
        // id can be either id1 or id2

		/* The following in place to test for high bit... */
	if ( id & 0x7fffffff ) {
		MWprintf ( 99, "Ding Ding Ding!  High bit set in lookupWorker!\n" );
		id &= 0x7fffffff;
	}

    MWWorkerID *w = workers;

    while ( w ) {
        if ( (w->get_id1() == id) || (w->get_id2() == id) ) {
            return w;
        }
        else {
            w = w->next;
        }
    }

        // didn't find it:
    return NULL;
}

void
MWDriver::call_hostaddlogic() {

		/* This is a wrapper around the lower level's hostaddlogic(), 
		   which will return 0 as a basic OK value, and a positive number
		   of hosts to delete if it thinks we should delete hosts.  This
		   can happen if the user changes the target number of workers 
		   to a lower value. */

	int *num_workers = new int[RMC->get_num_arches()];
	int j, numtodelete;
	MWWorkerID *w, *wx, *wn;
	for ( j=0 ; j < RMC->get_num_arches() ; j++ ) {
		num_workers[j] = numWorkers( j );
	}
   	numtodelete = RMC->hostaddlogic( num_workers );
	delete [] num_workers;
	
		/* Make sure we don't count already doomed machines: */
	if ( numtodelete > 0 ) {
		w = workers;
		while ( w ) {
			if ( w->is_doomed() ) numtodelete--;
			w = w->next;
		}
	}

	if ( numtodelete > 0 ) {

		MWprintf ( 40, "Deleting %d hosts.\n", numtodelete );
		MWprintf ( 40, "Ignore the HOSTDELETE or TASKEXIT messages.\n" );

			/* Walk thru list and kill idle workers. */
		w = workers;
		while ( w ) {
			if ( w->isIdle() ) {
				wx = rmWorker ( w->get_id1() );
				if ( wx != w ) {
					MWprintf ( 10, "Sanity failure in rmWorker!" );
					RMC->exit(1);
				}
				MWprintf ( 80, "Idle worker:\n" );
				w->printself ( 80 );
				wn = w->next;
				w->ended();
				stats.gather( w );
				RMC->removeWorker( w );
				w = wn;
				numtodelete--;
				if ( numtodelete == 0 ) return;
			} else {
				w = w->next;
			}
		}
				 
			/* Now walk thru workers and remove suspended machines */
		w = workers;
		while ( w ) {
			if ( w->isSusp() ) {
				wx = rmWorker ( w->get_id1() );
				if ( wx != w ) {
					MWprintf ( 10, "Sanity failure in rmWorker!" );
					RMC->exit(1);
				}
				MWprintf ( 80, "Idle worker:\n" );
				w->printself ( 80 );
				wn = w->next;
				hostPostmortem( w ); /* this really does do what we want! */
				RMC->removeWorker( w );
				w = wn;
				numtodelete--;
				if ( numtodelete == 0 ) return;
			} else {
				w = w->next;
			}
		}

			/* At this point, we could do something really cool like 
			   sort the workers based on some attributes.... */

			/* Next, walk thru list and mark working workers for removal */
		w = workers;
		while ( w ) {
			if ( !(w->is_doomed()) ) {
				w->mark_for_removal();
				MWprintf ( 40, "Host %s marked for removal.\n", 
						   w->machine_name() );
				numtodelete--;
				if ( numtodelete == 0 ) return;
			}
			w = w->next;
		}
	}
}

int 
MWDriver::numWorkers() {
	int count = 0;
	MWWorkerID *w = workers;
	while ( w ) {
		count++;
		w = w->next;
	}
	return count;
}

int 
MWDriver::numWorkers( int arch ) {
	int count = 0;
	MWWorkerID *w = workers;
	while ( w ) {
		if ( w->get_arch() == arch ) {
			count++;
		}
		w = w->next;
	}
	return count;
}
	

void 
MWDriver::printWorkers() 
{
	int n = 0;

    MWprintf ( 70, "---- A list of Workers follows: ----------------\n" );

    MWWorkerID *w = workers;

    while ( w ) {
        w->printself( 70 );
		n++;
        w = w->next;
    }

    MWprintf ( 70, "---- End worker list --- %d workers ------------\n", n );
    
}

MWKey 
MWDriver::return_best_todo_keyval()
{

  MWKey retval = DBL_MAX;

  if( task_key == NULL )
    {
      MWprintf( 10, " return_bst_keyval:  no task key "
		"function defined!\n");      
    }
  else
    {
      if( todo )
	{
	  if( listsorted )
	    {
	      return (*task_key)( todo );
	    }
	  else
	    {
	      retval = -DBL_MAX;
	      MWTask *it = todo;
	      while( it )
		{
		  MWKey tmp = (*task_key)( it );
		  if( tmp < retval )
		    retval = tmp;
		  it = it->next;
		}
	      
	    }
	}
    }
  return retval;
}

MWKey MWDriver::return_best_running_keyval()
{

  MWKey retval = DBL_MAX;

  if( task_key == NULL )
    {
      MWprintf( 10, " return_bst_keyval:  no task key "
		"function defined!\n");      
    }
  else
    {

      if( running )
	{
	  MWTask *it = running;
	  while( it )
	    {
	      MWKey tmp = (*task_key)( it );

	      if( tmp < retval )
		retval = tmp;
	      it = it->next;
	    }
	  
	}
    }

  return retval;
}



MWWorkerID * 
MWDriver::rmWorker ( int id ) {
    
    // search for a worker with the given id and remove it.
	// This can refer to either the primary or secondary id.
    
		/* There is an unfortunate bit of pvm dependence here.  Often, 
		   the highest bit will be set when addressing pvm host machines.
		   This pvmd tid will be packed in a message, and there's no
		   way for the MWRMComm layer to remove that annoying high
		   bit.  So...we have to clear it here.  The limitation is
		   that no other lower layer can use negative numbers as 
		   id1 or id2. */
	if ( id & 0x7fffffff ) {
		MWprintf ( 99, "Ding Ding Ding!  High bit set in rmWorker!\n" );
		id &= 0x7fffffff;
	}

    MWWorkerID *w, *p;
    w = workers;
    
    if ( w == NULL )
        return NULL;
    
    if ( (w->get_id1() == id) || (w->get_id2() == id) ) {
        workers = w->next;
        return w;
    }
    
    p = w;
    w = w->next;
    
    while ( w ) {
        if ( (w->get_id1() == id) || (w->get_id2() == id) ) {
            p->next = w->next;
            return w;
        }
        else {
            p = w;
            w = w->next;
        }
    }
    
    // if we get here, we didn't find it
    return NULL;
}

int
MWDriver::set_checkpoint_frequency( int freq ) {
	if ( checkpoint_time_freq != 0 ) {
		MWprintf ( 10, "Cannot set_checkpoint_frequency while time-based "
				   "frequency is not zero!\n" );
		return checkpoint_frequency;
	}
    int old = checkpoint_frequency;
    checkpoint_frequency = freq;
    return old;
}

int 
MWDriver::set_checkpoint_time ( int secs ) {
	if ( checkpoint_frequency != 0 ) {
		MWprintf ( 10, "Cannot set_checkpoint_time while task-based "
				   "frequency is not zero!\n" );
		return checkpoint_time_freq;
	}
	int old = checkpoint_time_freq;
	checkpoint_time_freq = secs;
	next_ckpt_time = time(0) + secs;
	return old;
}

void 
MWDriver::set_suspension_policy( MWSuspensionPolicy policy ) {
	if ( (policy!=DEFAULT) && (policy!=REASSIGN) ) {
		MWprintf ( 10, "Bad suspension policy %d.  Using DEFAULT.\n" );
		suspensionPolicy = DEFAULT;
		return;
	}
	suspensionPolicy = policy;
}

void
MWDriver::checkpoint() {
    
    FILE *cfp;  // Checkpoint File Pointer
		/* We're going to write the checkpoint to a temporary file, 
		   then move the file to "ckpt_filename" */

		/* We're not going to use tempnam(), because it sometimes 
		   gives us a filename in /var/tmp, which is bad for rename() */

	char *tempName = "mw_tmp_ckpt_file";

    if ( ( cfp=fopen( tempName, "w" ) ) == NULL ) {
        MWprintf ( 10, "Failed to open %s for writing! errno %d\n", 
				   tempName, errno );
        return;
    }

    MWprintf ( 50, "Beginning checkpoint()\n" );

        // header
    fprintf ( cfp, "MWDriver Checkpoint File.  %ld\n", time(0) );
    
        // some internal MWDriver state:
    fprintf ( cfp, "%d %d %d %d\n%d %d %d %d\n", task_counter, 
			  checkpoint_frequency, checkpoint_time_freq, 
			  num_completed_tasks,
			  (int)addmode, (int)getmode, (int)suspensionPolicy,
	      (int) machine_ordering_policy );

	if ( bench_task ) {
		fprintf ( cfp, "1 " );
		bench_task->write_ckpt_info ( cfp );
	} else {
		fprintf ( cfp, "0\n" );
	}

	MWprintf ( 80, "Wrote internal MWDriver state.\n" );

	RMC->write_checkpoint( cfp );

	MWprintf ( 80, "Wrote RMC state.\n" );

		/* Yes, I really am passing it the list of workers... */
	stats.write_checkpoint ( cfp, workers );
	
	MWprintf ( 80, "Wrote the Worker stats.\n" );

		// Write the user master's state:
    write_master_state( cfp );

	MWprintf ( 80, "Wrote the user master's state.\n" );

    int num_tasks = 0;
    MWTask *t = running;
    while ( t ) {
        num_tasks++;
        t = t->next;
    }
    
    t = todo;
    while ( t ) {
        num_tasks++;
        t = t->next;
    }

        // Tasks separator:
    fprintf ( cfp, "Tasks: %d \n", num_tasks );

    t = running;
    while ( t ) {
        fprintf ( cfp, "%d ", t->number );
        t->write_ckpt_info( cfp );
        t = t->next;
    }

    t = todo;
    while ( t ) {
        fprintf ( cfp, "%d ", t->number );
        t->write_ckpt_info( cfp );
        t = t->next;
    }

	MWprintf ( 80, "Wrote task list.\n" );

    fclose ( cfp );

		/* Now we rename our temp file to the real thing. Atomicity! */
	if ( rename( tempName, ckpt_filename ) == -1 ) {
		MWprintf ( 10, "rename( %s, %s ) failed! errno %d.\n", 
				   tempName, ckpt_filename, errno );
	}

	MWprintf ( 50, "Done checkpointing.\n" );
}

void 
MWDriver::restart_from_ckpt() {

	int i;

    FILE *cfp;
    if ( ( cfp=fopen( ckpt_filename, "r" ) ) == NULL ) {
        MWprintf ( 10, "Failed to open %s for reading! errno %d\n", 
				   ckpt_filename, errno );
        return;
    }

    char buf[128];
    time_t then;
    fscanf( cfp, "%s %s %s %ld", buf, buf, buf, &then );
    
    MWprintf ( 10, "This checkpoint made on %s\n", ctime( &then ) );

    fscanf( cfp, "%d %d %d %d", &task_counter, 
			&checkpoint_frequency, &checkpoint_time_freq,
            &num_completed_tasks );
	fscanf( cfp, "%d %d %d %d", 
			(int*) &addmode, (int*) &getmode, (int*) &suspensionPolicy,
		(int*) &machine_ordering_policy );
		
	int hasbench = 0;
	fscanf( cfp, "%d ", &hasbench );
	if ( hasbench ) {
		bench_task = gimme_a_task();
		bench_task->read_ckpt_info( cfp );
		MWprintf ( 10, "Got benchmark task:\n" );
		bench_task->printself( 10 );
	} 

	MWprintf ( 10, "Read internal MW info.\n" );

	RMC->read_checkpoint( cfp );

	MWprintf ( 10, "Read RMC state.\n" );

		// read old stats 
	stats.read_checkpoint( cfp );
	
	MWprintf ( 10, "Read defunct workers' stats.\n" );

    read_master_state( cfp );

	MWprintf ( 10, "Read the user master's state.\n" );

    int num_tasks;
    fscanf ( cfp, "%s %d", buf, &num_tasks );

    if ( strcmp( buf, "Tasks:" ) ) {
        MWprintf ( 10, "Problem in reading Tasks separator. buf = %s\n", buf );
	fclose ( cfp );
        return;
    }

    MWTask *t;
    for ( i=0 ; i<num_tasks ; i++ ) {
        t = gimme_a_task();   // return a derived task class...
        fscanf( cfp, "%d ", &(t->number) );
        t->read_ckpt_info( cfp );
        t->printself( 80 );
        ckpt_addTask( t );
    }

	MWprintf ( 10, "Read the task list.\n" );
    
    fclose( cfp );

    MWprintf ( 10, "We are done restarting.\n" );
}

int 
MWDriver::get_number_tasks() {
	MWTask *t = todo;
	int n=0;
	while ( t ) {
		n++;
		t = t->next;
	}
	return n;
}


#ifdef INDEPENDENT

extern MWWorker *gimme_a_worker ();
void
MWDriver::ControlPanel ( )
{
    MWReturn ustat = OK;
    RMC->hostadd ();
    master_mainloop_ind ( 0, 2 );
    MWWorker *worker = gimme_a_worker();
    worker->go( 1, NULL );
    master_mainloop_ind ( 0, 2 );
    worker->ind_benchmark ( );
    while ( ( (todo != NULL) || (running != NULL) ) && ( ustat == OK ) )
    {
    	ustat = master_mainloop_ind ( 0, 2 );
	ustat = worker->worker_mainloop_ind ( );
    }
    return;
}
#endif


MWKey
kflops( MWWorkerID *w )
{
  return (MWKey) w->KFlops;
}

MWKey
benchmark_result( MWWorkerID *w )
{
  return (MWKey) w->get_bench_result();
}
