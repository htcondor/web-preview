#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <iostream.h>
#include <fstream.h>
#include <strstream.h>
#include <assert.h>
#include <stdarg.h>
#include <errno.h>

#include "MW.h"
#include "MWDriver.h"
#include "MWWorkerID.h"
#include "MWWorker.h"
#include "MWTask.h"
#include "MWNWSTask.h"

/* 8/28/00
   Qun Chen wrote signal handling code that Michael likes.
   We add it here */

#include <setjmp.h>
#include <signal.h>

jmp_buf env; 
static void signal_handler(int sig){
  
  switch ( sig ){
  case SIGINT:       /* process for interrupt */
    longjmp(env,sig);
    /* break never reached */
  case SIGCONT:     /* process for alarm */
    longjmp(env,sig);
    /* break never reached */
  default:        exit(sig);
  }
  
  return;
}
/* end Qun */

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
    
  worker_timeout = false;
  worker_timeout_limit = 0.0;
  worker_timeout_check_frequency = 0;
  next_worker_timeout_check = 0;

  defaultTimeInterval = 10;
  defaultTimeLength = 50;

#if defined( XML_OUTPUT )
  
  xml_menus_filename = "menus";
  xml_jobinfo_filename = "job_info";
  xml_pbdescrib_filename = "pbdescrib";
  xml_filename = "/u/m/e/metaneos/public/html/iMW/status.xml";

  CondorLoadAvg = NO_VAL;
  LoadAvg = NO_VAL;
  Memory = NO_VAL;
  Cpus = NO_VAL;
  VirtualMemory = NO_VAL;
  Disk = NO_VAL;
  KFlops = NO_VAL;
  Mips = NO_VAL;

  memset(OpSys, 0, sizeof(OpSys)); 
  memset(Arch, 0 , sizeof(Arch)); 
  memset(mach_name, 0 , sizeof(mach_name)); 
  
  get_machine_info();
#endif    


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

  if ( ustat != OK ) {
    RMC->exit(1);
    return; 
  }
    
  // the main processing loop:
  ustat = master_mainloop ();
	
  if( ustat == OK || ustat == QUIT ) {
    // We're done - now print the results.
    printresults();

    //JTL  Since kill_workers can take a long time --we like
    //   to collect the statistics beforehand.  Therefore, 
    //   we gather all the stats from the currently workign workers,
    //   then call makestats here (before kill_workers).

    MWWorkerID *tempw = workers;
    while( tempw ) {
      stats.gather( tempw );
      tempw = tempw->next;
    }
    // tell the stats class to do its magic
    stats.makestats();
  }

  kill_workers();

  // remove checkpoint file.
  struct stat statbuf;
  if ( stat( ckpt_filename, &statbuf ) == 0 ) {
    if ( unlink( ckpt_filename ) != 0 ) {
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

#if 0
      // Yet another Hack for Michael
      get_userinfo( argc, argv );
#endif


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
    MWWorkerID *w = NULL;
    MWReturn ustat = OK;
    int len, tag, info;

#else
    int buf_id;
    int info, len, tag;
    int sending_host;
    MWWorkerID *w = NULL;
	
    MWReturn ustat = OK;

        // while there is at least one job in the todo or running queue:
    while ( ( (todo != NULL) || (running != NULL) ) && ( ustat == OK ) ) 
    {

#endif
      if( todo ) {
	MWprintf( 99, " CONTINUE -- todo list has at least task "
		  "number: %d\n", todo->number ); 
      }
      else if( running ) {
	MWprintf( 60, " CONTINUE -- running list has at least task "
		  "number: %d\n", running->number ); 
      }

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

	/* 8/28/00
	   Qun Chen's signal handling code for Michael Ferris
	*/

	int returned_from_longjump, processing = 1;
	
	if ((returned_from_longjump = setjmp(env)) != 0)
	  switch (returned_from_longjump)     {
	  case SIGINT:
	    MWprintf( 10, "!!!! Get user interruption.\n" );
	    checkpoint();
	    //printresults();
	    return QUIT;
	  case SIGCONT: //Our program normally won't have this signal
	    MWprintf( 10, "FATCOP was signaled to checkpoint...\n" );
	    checkpoint();
	    MWprintf( 10, "finish checkpointing ...\n" );
	    break; 
	  }
	(void) signal(SIGINT, signal_handler);
	(void) signal(SIGCONT, signal_handler);
	// alarm( 10 );
	


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
	case CHECKSUM_ERROR: {
	    handle_checksum();
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


		// If there is a worker_timeout_limit check all workers if there are
		// timed-out workers in working state

		if (worker_timeout){

		  time_t now = time(0);

		  if (next_worker_timeout_check <= now){

		    next_worker_timeout_check = now + worker_timeout_check_frequency; 
		    MWprintf ( 80, "About to check timedout workers"); 	       					 
		    reassign_tasks_timedout_workers();

		  }
		  		     
		  
		}
		


#ifndef INDEPENDENT

	}		

	if( ustat != OK ) {
		MWprintf( 10, "The user signalled %d to stop execution.\n", ustat );
	}
	MWprintf ( 10, "The MWDriver is done.\n" );
	
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

    // 9/5/00 Jeff changes this at Jim Basney's behest.  
    //          Now, MWPvmRC will return the proper hostname, so we
    //          check to see if the name has already been set before
    //          setting it here

    if( w->machine_name()[0] == '\0' ) {
      w->set_machine_name( workerhostname );
    }
    else {
      MWprintf( 30, "Current hostname: %s, unpacked name %s\n", w->machine_name(), 
		workerhostname );
    }

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

	MWprintf( 10, "Received bench result %lf from %s ( id: %d )\n", 
		  bres, w->machine_name(), w->get_id1() );

	stats.update_best_bench_results( bres );

	w->benchmarkOver();

	//JTL -- Now you need to order the workerlist
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
		//XXX RMC is to delete the worker -- why?!?!?!?!
		//delete w;  // comment this if you uncomment above line!
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

    MWTask *t = NULL;
    if( w->currentState() != IDLE ) {
		return;
    }
		/* End of Jeff's kludgy fix */

    double lastMeasured;
    double nowTime;
    struct timeval timeVal;
    w->getNetworkConnectivity ( lastMeasured );
    gettimeofday ( &timeVal, NULL );
    nowTime = timeval_to_double ( timeVal );
    if ( nowTime > lastMeasured + 1000000 * 60 * 10 )
    {
    	t = new MWNWSTask ( defaultTimeInterval, defaultTimeLength );
    }
    else
    	t = getNextTask();

    if ( t != NULL ) {

        RMC->initsend();
	RMC->pack ( &(int)(t->taskType), 1, 1 );
        RMC->pack( &(t->number), 1, 1 );
        
		pack_driver_task_data();

        t->pack_work();
        
        RMC->send( w->get_id2(), DO_THIS_WORK );
        
        w->gottask( t->number );
        w->runningtask = t;
        t->worker = w;
        putOnRunQ( t );
        
        MWprintf ( 10, "Sent task %d to %s  ( id: %d ).\n", 
		   t->number, w->machine_name(),
		   w->get_id1() );
        
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

	// Here unset the high bit -- This should go in MWPvmRC.C?!?!?
	if( who & 0x80000000 ) {
	  who &= 0x7fffffff;
	}

	MWWorkerID *w = rmWorker( who );
	if ( w == NULL ) {
		MWprintf ( 40, "Got HOSTDELETE of worker %d, but it isn't in "
				   "the worker list.\n", who );
	} else {
		
		MWprintf ( 40, "Got HOSTDELETE!  \" %s \" (%d) went down.\n", 
				   w->machine_name(), who );   
		hostPostmortem ( w );
			/* We may have to ask for some hosts at this time... */
		delete w;
		call_hostaddlogic();
	}
}

void
MWDriver::handle_checksum ( )
{
    // Some worker returned us a checksum error message
    // Actually there may be some bootstrapping problem here
    int who;
    RMC->unpack ( &who, 1, 1 );
	
    MWReturn ustat = OK;
    MWWorkerID *w = lookupWorker ( who );

    MWprintf( 10, "Worker \" %s \" ( %d ) returned a checksum error!\n", 
	      w->machine_name(), who );

    if ( w == NULL )
    {
    	MWprintf ( 10, "Got CHECKSUM_ERROR from worker %08d, but it isn't in "
					"the worker list.\n", who );
	return;
    }
    else
    {
    	switch ( w->currentState ( ) ) 
	{
		case BENCHMARKING:
		{
				   int stat = RMC->initsend();
				   if( stat < 0 ) {
				       MWprintf( 10, "Error in initializing send in pack_worker_init_data\n");
				   }
				  
				   if ( (ustat = pack_worker_init_data()) != OK ) {
				       	MWprintf ( 10, "pack_worker_init_data returned !OK.\n" );
				       	w->printself(10);
				       	w->started();
					w->ended();
					return;
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
				    return;
				    break;
		}
		case WORKING:
		{
				    if ( w->runningtask )
				    {
				    	// It was running a task
					// See if this task is there in the done list
					pushTask ( w->runningtask );
					send_task_to_worker ( w );
					return;
				    }
				    else
				    {
				    	// How is this possible that a worker is not working on something
					MWprintf ( 10, "Worker was not working on any task but still gave a CHECKSUM_ERROR\n");
					return;
				    }
				    break;
		}
	}

    }
}

void 
MWDriver::handle_hostsuspend () 
{

	 
	int who;
    RMC->unpack ( &who, 1, 1 );


        // Here unset the high bit -- This should go in MWPvmRC.C?!?!?
	        if( who & 0x80000000 ) {
		          who &= 0x7fffffff;
			          }
				  
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
					   the 
					   tasks to workers, in case any are waiting around.*/


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
		MWprintf ( 40, "Got HOSTSUSPEND of worker %d, but it isn't in "
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
		MWprintf ( 40, "Got TASKEXIT of worker %d, but it isn't in "
				   "the worker list.\n", who );
        return;
    }

	MWprintf ( 40, "Got TASKEXIT from machine %s (%d).\n", 
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
		MWprintf ( 60, " %s (%d) was not running a task.\n", 
				 w->machine_name(), w->get_id1() );
	}
	else {
		MWprintf ( 60, " %s (%d) was running task %d.\n", 
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

	// We once received CPU times that were NAN from a worker,
	// and this messes EVERYTHING up

	if( cpu_time != cpu_time ) {
	  MWprintf( 10, "Error.  cpu_time of %lf, assigning to wall_time of %lf\n",
		    cpu_time, wall_time );
	}
	
	// Jeff's CPU time sanity check!!!!
	if( cpu_time > 1.1 * wall_time || cpu_time < 0.01 * wall_time ) {

	  MWprintf( 10, "Warning.  Cpu time =%lf,  wall time = %lf seems weird.\n", 
		    cpu_time, wall_time );
	  MWprintf( 10, "Assigning cpu time to be %lf\n", wall_time );
	  cpu_time = wall_time;
	}
	//	total_time = now - start_time;	


    MWprintf ( 60, "Unpacked task %d result from %s ( id %d ).\n", 
			   tasknum, w->machine_name(), 
	       w->get_id1() );

	MWprintf ( 60, "It took %5.2f secs wall clock and %5.2f secs cpu time\n", 
			   wall_time, cpu_time );

	int mwtasknum = w->runningtask ? w->runningtask->number : -2;
	if( tasknum != mwtasknum ) {
	  MWprintf( 10, "What the $%!^$!^!!!.  MW thought that %s (id %d) was running %d, not %d\n", w->machine_name(), w->get_id1(), mwtasknum, tasknum );
	}

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
        MWprintf ( 60, "Task not found in running list; assumed done.\n" );

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

	if ( t->taskType == MWNORMAL )
	{
		ustat = act_on_completed_task( t );
        
		num_completed_tasks++;

		if ( (checkpoint_frequency > 0) && 
			(num_completed_tasks % checkpoint_frequency) == 0 ) 
		{
				checkpoint();
		} 
		else 
		{
			if ( checkpoint_time_freq > 0 ) 
			{
				time_t now = time(0);
				MWprintf ( 80, "Trying ckpt-time, %d %d\n", 
		   			next_ckpt_time, now );
				if ( next_ckpt_time <= now ) 
				{
					next_ckpt_time = now + checkpoint_time_freq;
					MWprintf ( 80, "About to...next_ckpt_time = %d\n", 
						   next_ckpt_time );
					checkpoint();
				}
			}
		}
	}
	else
	{
		if ( t->taskType == MWNWS )
		{
			if ( w ) 
				w->setNetworkConnectivity ( t );
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
	int ns = nosend ? nosend->get_id2() : -1;
    
    while( w ) {
        if ( ( w->currentState() == IDLE ) && 
             ( w->get_id2() != ns ) )  {
            
	  MWprintf ( 92, "In rematch_tasks_to_workers(), trying send...\n" );
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

		// JP fixed this to update todoend
		if (todo == NULL){
		  todoend = NULL;
		}
		
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
				
				// JP fixed this in case mt = todoend

				if (mt == todoend){
				  todoend = prevmt;
				}
				
			}
			else {
				todo = todo->next;
				// JP fixed this in case todoend has to change
				if (todo == NULL){
				  todoend = NULL;
				}
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
MWDriver::pushTask( MWTask *push_task ) 
{

  // in this case, a task failed and needs to be done again.
  // we put it on the head of the list.
  //  9/6/00 -- Jeff says no.  Some people want to keep
  //   things by key, so you should add it where they say to.
  //   This is just like addTask, excecpt we don't want to 
  //   increment the number
    
    if ( todo == NULL ) {
        todo = push_task;
        push_task->next = NULL;
    }
    else {

      switch( addmode ) {
			
      case ADD_AT_BEGIN: {
	push_task->next = todo;
	todo = push_task;
	break;
      }
		
      case ADD_AT_END: {

	push_task->next = NULL;
	todoend->next = push_task;
	todoend = push_task;
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
	MWKey ptval = (*task_key)( push_task );
	while( t ) {
	  if( ptval < (*task_key)( t ) ) {
	    if( prevt == NULL ) {
	      push_task->next = todo;
	      todo = push_task;
	    }
	    else {
	      prevt->next = push_task;
	      push_task->next = t;
	    }
	    break;
	  }
	  prevt = t;
	  t = t->next;
	}
	
	if( t == NULL ) {
	  // This task is the worst!  Add it at the end.
	  push_task->next = NULL;
	  todoend->next = push_task;
	  todoend = push_task;
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
MWDriver::ckpt_addTask( MWTask *add_task )  {

  //	add_task->number = task_counter++;

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

#if 0
    // Michael debugging code
    if( (*task_key)( todo ) < 0 ) {
      MWprintf( 10, "WOAH!!! -- You are totally hosed\n" );
    }
#endif
}

void 
MWDriver::set_task_key_function( MWKey (*t)( MWTask * ) ) 
{
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


void 
MWDriver::set_worker_timeout_limit( double timeout_limit, int timeout_freq ) {

  if (timeout_limit > 0){

    worker_timeout = true;
    worker_timeout_limit = timeout_limit;

    worker_timeout_check_frequency = timeout_freq ;
    next_worker_timeout_check = time(0) + timeout_freq ;

    MWprintf( 30, "Set worker timeout limit to %lf\n", timeout_limit);
    MWprintf( 30, "Set worker timeout frequency to %d\n", timeout_freq);    

  }
  else {
    MWprintf(10, "Urgh, Timeout limit has to be > 0\n");
  }

}

void MWDriver::reassign_tasks_timedout_workers(){

  MWprintf ( 10, "Enter : reassign_tasks_timedout_workers()\n");

  if (worker_timeout_limit > 0) {

    MWWorkerID *tempw = workers;
    struct timeval t;
    gettimeofday ( &t, NULL );
    double now = timeval_to_double ( t );

    MWprintf( 60, "Now: %lf\n", now );

    while( tempw ) {		

      if (tempw->currentState() == WORKING) {

	MWprintf( 50, "Last event from %s ( id: %d ) : %lf\n", tempw->machine_name(), 
		  tempw->get_id1(), tempw->get_last_event() );

	if ( (now - tempw->get_last_event() ) > worker_timeout_limit ){
	  
	  // this worker is timed out
	  
	  if (tempw->runningtask){

	    int task = tempw->runningtask->number;

	    MWTask *rmTask = rmFromRunQ( task );

	    if ( rmTask ) {
	    MWprintf ( 10, "Worker %d is timed-out\n",tempw->get_id1());
	    MWprintf ( 10, "Task %d rescheduled \n",rmTask->number);
	    }
	    else {
	      MWprintf ( 60, "Task not found in running list; assumed done or in todo list.\n" );
	    }

	    if ( !rmTask ) {
	      MWprintf ( 60, "Not Re-Assigning task %d.\n", task );
	    }
	    else {

	      MWTask *rt = tempw->runningtask;
	      
	      if ( rt != rmTask ) {
		MWprintf ( 10, "REASSIGN: something big-time screwy.\n");
	      }

	      // We don't need to do that.
	      // tempw->runningtask = NULL;

	      /* Now we put that task on the todo list and rematch
		 the tasks to workers, in case any are waiting around.*/
	      pushTask( rt );
	      // just in case...
	      rematch_tasks_to_workers ( tempw );
	    }

	  }
	}

      }

      tempw = tempw->next;

    }

    /* This is Jeff's code.  We go through the *running* list 
       too to see if there is a task there that needs to be reassigned.

       For example -- if there is a task in the running list
       not assigned to anyone.  This can happen.  For example...

       
    */

    MWTask *tempr = running;
    while( tempr ) {

      // Find the worker running this task.
      bool foundworker = false;
      MWWorkerID *tempw = workers;
      while( tempw ) {
	if( tempw->runningtask ) {
	  if( tempw->runningtask->number == tempr->number ) {
	    foundworker = true;
	    break;
	  }
	}
	tempw = tempw->next;
      }

      if( foundworker ) {
	assert( tempw != NULL );
	MWprintf( 60, "Worker id %d  has task %d\n", tempw->get_id1(), 
		  tempr->number );
      }
      else {
	MWprintf( 10, "Woah!!!  Task number %d lost his worker.  Rescheduling\n", 
		  tempr->number );

	// First remove him from the run queue
	MWTask *check = rmFromRunQ( tempr->number );
	if( check != tempr ) {
	  MWprintf( 10, "There is a HUGE logic error in the code.  Aborting\n" );
	  assert( 0 );
	}

	pushTask( tempr );
	break;
      }
      
      tempr = tempr->next;
    }
    
    

  }
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

#if 0
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
#endif

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
		

}

MWWorkerID * 
MWDriver::lookupWorker ( int id ) {
    
        // look for a given id and return assoc. worker.  

		/* The following in place to test for high bit that (stupid) 
		   PVM sets! ... */

	if ( id & 0x80000000 ) {
	  MWprintf ( 10, "Ding Ding Ding!  High bit set in lookupWorker!\n" );
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


int 
MWDriver::numWorkersInState( int ThisState ) {

  // We handle the case WORKING separately such that
  // we don't count the workers with no runningtask

  int count = 0;   
  
  if (ThisState == WORKING){
    MWWorkerID *w = workers;
    while ( w ) {
      if ( (w->currentState() == ThisState) && (w->runningtask != NULL) ) {
	count++;
      }
      w = w->next;
    }
    return count;   
  }
  else{
    MWWorkerID *w = workers;
    while ( w ) {
      if ( w->currentState() == ThisState ) {
	count++;
      }
      w = w->next;
    }
    return count;
  }
  
}

	

void 
MWDriver::printWorkers() 
{
	int n = 0;

    MWprintf ( 1, "---- A list of Workers follows: ----------------\n" );

    MWWorkerID *w = workers;

    while ( w ) {
        w->printself( 1 );
	MWprintf( 1, "\n" );
		n++;
        w = w->next;
    }

    MWprintf ( 1, "---- End worker list --- %d workers ------------\n", n );
    
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
	      // A debugging statement for Michael
	      MWprintf( 30, "List sorted -- returning first item\n" );
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
		   This pvmd tid will be packed in the message telling
		   you which host to remove.

		   Unless you can change the PVM code, or *always* want to
		   unset the first bit of every integer than you unpack
		   you have to do it here.  :-(

		   
		   that no other lower layer can use negative numbers as 
		   id1 or id2. */
	if ( id & 0x80000000 ) {
	  MWprintf ( 10, "High bit set in rmWorker!   Unsetting!\n" );
	  id &= 0x7fffffff;
	}

    MWWorkerID *w, *p;
    w = workers;
    
    if ( w == NULL ) {
      MWprintf( 10, "Trying to rmWorker %d, but worker list is NULL -- What's up with that!?\n" , id );
      return NULL;
    }
    
    if ( (w->get_id1() == id) || (w->get_id2() == id) ) {
        workers = w->next;
        return w;
    }
    
    p = w;
    w = w->next;
    
    while ( w ) {
        if ( (w->get_id1() == id) || (w->get_id2() == id) ) {
            p->next = w->next;

	    MWprintf( 10, "Here is the worker you removed:\n" );
	    w->printself( 10 );
	    
            return w;
        }
        else {
            p = w;
            w = w->next;
        }
    }

    // These are also for Jeff's debugging purposes
    MWprintf( 10, "Trying to remove non-existent worker %d!?!?!\n", id );
    printWorkers();
      
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

#if 0
	// hacked for Michael's benefit
	fscanf( cfp, "%d %d %d", 
			(int*) &addmode, (int*) &getmode, (int*) &suspensionPolicy );
#endif

	
		
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

    // 9/2/00 Jeff added in a hurry for Michael.  I think it is
    //   best to add the checkpoint items by key...

    if( task_key !=NULL ) {
      set_task_add_mode( ADD_BY_KEY );
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

    // This is debugging code for Michael

#if 1
    if( task_key != NULL ) {
      MWprintf( 10, "Printing keys of user's task list the user's task list...\n" );
      //sort_task_list();
      print_task_keys();
    }
#endif
    
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

int MWDriver::get_number_running_tasks()
{
  MWTask* t;
  int counter = 0;
  
  if (running == NULL) {
    return 0;
  }

  t = running;

  while (t){
    t = t->next;
    counter++;
  }

  return counter;


}





/* 8/28/00

   Qun added Jeff's idea of adding a SORTED list of tasks.
   This can greatly improve the efficiency in many applications.
   
   XXX I haven't debugged this, and probably there needs to be more
   checking that the list is actually sorted.  
 */

/* added by Qun: for insert a list of sorted tasks */

void 
MWDriver::addSortedTasks( int n, MWTask **add_tasks )
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

#if 0
      // JEFF DEBUG -- Check that they are in order.
	MWprintf( 10, "Adding tasks with keys...\n" );
	for( int i = 0; i < n; i++ ) {
	  MWprintf( 10, "  %lf\n", (*task_key)( add_tasks[i] ) );
	}
#endif

      

      MWTask *hand = todo;
      MWTask *handprev = NULL;
    

      // Call the single add n times
      // since it is a sorted list, we only need to strat from hand to search for
      // an insertion position every time. To make the program easy, we record
      // handprev whose next pointer points to hand

	for( int i = na; i < n; i++ ){
/*
	cout << "###################### " << i << endl;
	print_task_keys();
	if (NULL == handprev){
	  cout << " handprev is NULL " << endl;
	}
	else{
	  cout << "handprev: " << (*task_key)( handprev ) <<endl;
	}
	if( NULL == hand){
	  cout << " hand is NULL " << endl;
	}
	else{
	  cout << "hand: " << (*task_key)( hand ) <<endl;
	}
	cout << "add_tasks: " << (*task_key)( add_tasks[i] ) <<endl;
*/
	addTaskByKey( add_tasks[i], hand, handprev );
      }
      
      break;
    }
    
    } // switch

    // JEFF DEBUG
    // print_task_keys();
    
}


void 
MWDriver::addTaskByKey( MWTask *add_task , MWTask *& hand, MWTask *& handprev )  {
  
  add_task->number = task_counter++;
  
  if( todo == NULL ) {
    add_task->next = NULL;
    todo = todoend = add_task;
    hand = todo;  //set up the next hand value
    handprev = NULL;
  }
  else { //only assume ADD_BY_KEY
    
    // This will insert at the first key WORSE than the given one
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
    MWTask *t = hand;
    MWTask *prevt = handprev;
    
    while( t ) {
      if( (*task_key)( add_task ) < (*task_key)( t ) ) {
	if( prevt == NULL ) {
	  add_task->next = todo;
	  todo = add_task;
	  hand = todo->next;
	  handprev = todo;
	}
	else {
	  prevt->next = add_task;
	  add_task->next = t;
	  hand = t;
	  handprev = prevt->next;
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
      handprev = todoend; //set handprev
      todoend = add_task;
      hand = todoend;    //set hand   
    }
  }
  return ;
  
}




//end the changes done  by Qun



double MWDriver::timeval_to_double( struct timeval t )
{
  return (double) t.tv_sec + ( (double) t.tv_usec * (double) 1e-6 );
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

// These are functions that might be useful, but I am not sure if they should
//  be included in the final release.

double 
MWDriver::get_instant_pool_perf( ) {

        double total_perf = 0;
	MWWorkerID *w = workers;

	while ( w ) {
		if ( (w->currentState() == WORKING) && (w->runningtask != NULL) ) {
			total_perf += w->get_bench_result();
		}
		w = w->next;
	}

	return total_perf;
}


#if defined( XML_OUTPUT )

void MWDriver::write_XML_status(){

  ofstream xmlfile("/u/m/e/metaneos/public/html/iMW/status.xml",
		   ios::trunc|ios::out);


  if( ! xmlfile ) {
    cerr << "Cannot open 'status.xml data file' file!\n";
  }

  //system("/bin/rm -f ~/public/html/iMW/status.xml");
  //system("/bin/mv -f ./status.xml ~/public/html/iMW/");   

  // system("/bin/more status.xml");

  char *temp_ptr ;
  temp_ptr = get_XML_status();

  xmlfile << temp_ptr;
  delete temp_ptr;

  //  xmlfile << ends;

  xmlfile.close();

  //system("/bin/rm -f ~/public/html/iMW/status.xml");
  //system("/bin/cp -f status.xml /u/m/e/metaneos/public/html/iMW/");
  // system("/bin/cp -f status.xml ~metaneos/public/html/iMW/");
}

#include <strstream.h>

char* MWDriver::get_XML_status(){

  ifstream menus(xml_menus_filename);

  ostrstream xmlstr;

  xmlstr << "<?xml version=\"1.0\"?>" << endl;
  xmlstr << "<?xml:stylesheet type=\"text/xsl\" href=\"menus-QAP.xsl\"?>" << endl;
  xmlstr << "<INTERFACE TYPE=\"iMW-QAP\">" << endl;

  // xmlstr << menus;

  char ch;
  while (menus.get(ch)) xmlstr.put(ch);

  // menus >> xmlstr ;

  xmlstr << "<MWOutput TYPE=\"QAP\">" << endl;

  char *temp_ptr ;

  temp_ptr = get_XML_job_information();

  xmlstr << temp_ptr;
  delete temp_ptr;

  temp_ptr = get_XML_problem_description();

  xmlstr << temp_ptr;
  delete temp_ptr;

  temp_ptr = get_XML_interface_remote_files();

  xmlstr << temp_ptr;
  delete temp_ptr;

  temp_ptr = get_XML_resources_status();

  xmlstr << temp_ptr;
  delete temp_ptr;

  temp_ptr = get_XML_results_status();

  xmlstr << temp_ptr;
  delete temp_ptr;

  xmlstr << "</MWOutput>" << endl;
  xmlstr << "</INTERFACE>" << endl;

 xmlstr << ends;



  menus.close();


  return xmlstr.str();

}

char* MWDriver::get_XML_job_information(){

  ifstream jobinfo(xml_jobinfo_filename);

  ostrstream xmlstr;

  char ch;
  while (jobinfo.get(ch)) xmlstr.put(ch);

  xmlstr << ends;

  jobinfo.close();
 

  return xmlstr.str();

};

char* MWDriver::get_XML_problem_description(){

  ifstream pbdescrib(xml_pbdescrib_filename);

  ostrstream xmlstr;

  char ch;
  while (pbdescrib.get(ch)) xmlstr.put(ch);

  xmlstr << ends;

  pbdescrib.close();


  return xmlstr.str();

};

char* MWDriver::get_XML_interface_remote_files(){

  ostrstream xmlstr;

  xmlstr << "<InterfaceRemoteFiles>" << endl;

  // dump here content of file

  xmlstr << "</InterfaceRemoteFiles>" << endl;

  xmlstr << ends;

  return xmlstr.str();

}


char* MWDriver::get_XML_resources_status(){

  int i;
  ostrstream xmlstr;

  // Begin XML string

  xmlstr << "<ResourcesStatus>" << endl;

  double average_bench;
  double equivalent_bench;
  double min_bench;
  double max_bench;
  double av_present_workers;
  double av_nonsusp_workers;
  double av_active_workers;
  double equi_pool_performance;
  double equi_run_time;
  double parallel_performance;
  double wall_time;

  stats.get_stats(&average_bench,
		  &equivalent_bench,
		  &min_bench,
		  &max_bench,
		  &av_present_workers,
		  &av_nonsusp_workers,
		  &av_active_workers,
		  &equi_pool_performance,
		  &equi_run_time,
		  &parallel_performance,
		  &wall_time,
		  workers
		  );

  // MWStats Information

  xmlstr << "<MWStats>" << endl;

  xmlstr << "<WallClockTime>" << wall_time << "</WallClockTime>" << endl;
  xmlstr << "<NormalizedTotalCPU>" << equi_run_time << "</NormalizedTotalCPU>" << endl;
  xmlstr << "<ParallelEff>" <<  parallel_performance << "</ParallelEff>" << endl;

  xmlstr << "<BenchInfo>" << endl;
     xmlstr << "<InstantPoolPerf>" << get_instant_pool_perf() << "</InstantPoolPerf>" << endl;
     xmlstr << "<EquivalentPoolPerf>" << equi_pool_performance << "</EquivalentPoolPerf>" << endl;
     xmlstr << "<AverageBench>" << average_bench << "</AverageBench>" << endl;
     xmlstr << "<EquivalentBench>" << equivalent_bench << "</EquivalentBench>" << endl;
     xmlstr << "<MinBench>" << min_bench << "</MinBench>" << endl;
     xmlstr << "<MaxBench>" << max_bench << "</MaxBench>" << endl;
  xmlstr << "</BenchInfo>" << endl;

  xmlstr << "<AverageWorkersStats>" << endl;
     xmlstr << "<AveragePresentWorkers>" << av_present_workers << "</AveragePresentWorkers>" << endl;
     xmlstr << "<AverageNonSuspWorkers>" << av_nonsusp_workers << "</AverageNonSuspWorkers>" << endl;
     xmlstr << "<AverageActiveWorkers>"  << av_nonsusp_workers << "</AverageActiveWorkers>" << endl;
  xmlstr << "</AverageWorkersStats>" << endl;


  xmlstr << "</MWStats>" << endl;

     // Master Information

     xmlstr << "<Master>" << endl;

     xmlstr << "<MasterPhysicalProperties>" << endl;
     xmlstr << "<Name>" << mach_name << "</Name>" << endl;
     //     xmlstr << "<IPAddress>" << get_IPAddress() << "</IPAddress>" << endl;
     xmlstr << "<OpSys>" << get_OpSys() << "</OpSys>" << endl;
     xmlstr << "<Arch>" << get_Arch() <<"</Arch>" << endl;
     xmlstr << "<Memory>" << get_Memory() << "</Memory>" << endl;
     xmlstr << "<VirtualMemory>" << get_VirtualMemory() << "</VirtualMemory>" <<  endl; 
     xmlstr << "<DiskSpace>" << get_Disk() << "</DiskSpace>" << endl;
     xmlstr << "<KFlops>" << get_KFlops() << "</KFlops>" << endl;
     xmlstr << "<Mips>" << get_Mips() << "</Mips>" << endl;
     xmlstr << "<CPUs>" << get_Cpus() << "</CPUs>" << endl;
     xmlstr << "<NWSinfos/>" << endl;
     xmlstr << "</MasterPhysicalProperties>" << endl;
                       


     xmlstr << "<MasterUsageProperties>" << endl;
     xmlstr << "<StartTime>July 1st 1999, 18:21:12s GMT</StartTime>"  << endl;
     xmlstr << "</MasterUsageProperties>" << endl;

     xmlstr << "</Master>" << endl;

     // Worker Information

     xmlstr << "<Workers>" << endl;

     int numworkers = numWorkersInState( INITIALIZING ) +  numWorkersInState( BENCHMARKING ) + numWorkersInState( IDLE ) + numWorkersInState( WORKING ) ;

     xmlstr << "<WorkersNumber>" << numworkers << "</WorkersNumber>" << endl;

     xmlstr << "<WorkersStats>" << endl;

     xmlstr << "<WorkersInitializing>" << numWorkersInState( INITIALIZING ) << "</WorkersInitializing>" << endl;
     xmlstr << "<WorkersBenchMarking>" << numWorkersInState( BENCHMARKING ) << "</WorkersBenchMarking>" << endl;
     xmlstr << "<WorkersWaiting>" << numWorkersInState( IDLE ) << "</WorkersWaiting>" << endl;
     xmlstr << "<WorkersWorking>" << numWorkersInState( WORKING ) << "</WorkersWorking>" << endl;
     xmlstr << "<WorkersSuspended>" << numWorkersInState( SUSPENDED ) << "</WorkersSuspended>" << endl;
     xmlstr << "<WorkersDone>" << numWorkersInState( EXITED ) << "</WorkersDone>" << endl;

     xmlstr << "</WorkersStats>" << endl;

     xmlstr << "<WorkersList>" << endl;

     MWWorkerID *w = workers;

     while ( w ) {

       if ((w->currentState() != SUSPENDED) || (w->currentState() != EXITED)){
		         
       xmlstr << "<Worker>" << endl;

       xmlstr << "<WorkerPhysicalProperties>" << endl;

          xmlstr << "<Name>" << w->machine_name() << "</Name>" << endl;
	  //          xmlstr << "<IPAddress>" << w->get_IPAddress() << "</IPAddress>" << endl; 
          xmlstr << "<Status>" << MWworker_statenames[w->currentState()] << "</Status>" << endl; 
	  xmlstr << "<OpSys>" << w->OpSys << "</OpSys>" << endl; 
	  xmlstr << "<Arch>" <<  w->Arch << "</Arch>" << endl; 
	  xmlstr << "<Bandwidth>" << "N/A" << "</Bandwidth>" << endl; 
	  xmlstr << "<Latency>" << "N/A" << "</Latency>" << endl; 
	  xmlstr << "<Memory>" << w->Memory << "</Memory>" <<  endl; 
	  xmlstr << "<VirtualMemory>" << w->VirtualMemory << "</VirtualMemory>" <<  endl; 
	  xmlstr << "<DiskSpace>" << w->Disk << "</DiskSpace>" << endl; 
	  xmlstr << "<BenchResult>" << w->get_bench_result() <<"</BenchResult>" << endl;
	  xmlstr << "<KFlops>" << w->KFlops <<"</KFlops>" << endl;
	  xmlstr << "<Mips>" << w->Mips <<"</Mips>" << endl;   
	  xmlstr << "<CPUs>" << w->Cpus <<"</CPUs>" << endl;
	  xmlstr << "<NWSinfos>" << "N/A" << "</NWSinfos>" << endl; 

       xmlstr << "</WorkerPhysicalProperties>" << endl;

       xmlstr << "<WorkerUsageProperties>" << endl;

          xmlstr << "<TotalTime>" << "N/A" << "</TotalTime>" << endl;
          xmlstr << "<TotalWorking>" << w->get_total_working() << "</TotalWorking>" << endl;
          xmlstr << "<TotalSuspended>" << w->get_total_suspended() << "</TotalSuspended>" << endl;

       xmlstr << "</WorkerUsageProperties>" << endl;

       xmlstr << "</Worker>" << endl;
       }

       w = w->next;  

     }

     xmlstr << "</WorkersList>" << endl;

     xmlstr << "</Workers>" << endl;

     // Global Worker Statistics

      xmlstr << "<GlobalStats>" << endl;

      

      xmlstr << "</GlobalStats>" << endl;

     // Task Pool Information


     xmlstr << "<TaskPoolInfos>" << endl;

     int nrt = get_number_running_tasks();
     int tt = get_number_tasks();
     int tdt = tt - nrt;

     xmlstr << "<TotalTasks>" << tt << "</TotalTasks>" << endl;
     xmlstr << "<TodoTasks>" << tdt << "</TodoTasks>" << endl;
     xmlstr << "<RunningTasks>" << nrt << "</RunningTasks>" << endl;
     xmlstr << "<NumberCompletedTasks>" << num_completed_tasks << "</NumberCompletedTasks>" << endl;
     //xmlstr << "<MaxNumberTasks>" << max_number_tasks << "</MaxNumberTasks>" << endl;
     xmlstr << "</TaskPoolInfos>" << endl;

     // Memory Info

     // Secondary storage info


  // End  XML string

  xmlstr << "</ResourcesStatus>" << endl;
  xmlstr << ends ;

  return xmlstr.str();

}

char* MWDriver::get_XML_results_status(){

  ostrstream xmlstr;

  xmlstr << "<ResultsStatus>" << endl;
  xmlstr << "</ResultsStatus>" << endl;

  xmlstr << ends;

  return xmlstr.str();

}

/* The following (until the end of the file) was written by one of 
   Jean-Pierre's students.  It isn't exactly efficient, dumping
   a condor_status output to a file and reading from that.  I understand
   that it does work, however.  -MEY (8-18-99) */

int MWDriver::check_for_int_val(char* name, char* key, char* value) {
	if (strcmp(name, key) == 0)
		return atoi(value);
	else return NO_VAL;
}

double MWDriver::check_for_float_val(char* name, char* key, char* value) {
	if (strcmp(name, key) == 0)
		return atof(value);
	else return NO_VAL;  
}

int  MWDriver::check_for_string_val(char* name, char* key, char* value) {
	if (strcmp(name, key) == 0)
		return 0;
	else return NO_VAL;  
}


void MWDriver::get_machine_info()
{
  FILE* inputfile;
  char filename[50];
  char key[200];
  char value[1300];
  char raw_line[1500];
  char* equal_pos;
  int found;
  char temp_str[256];

  char zero_string[2];

  memset(zero_string, 0 , sizeof(zero_string));

  memset(filename, '\0', sizeof(filename));
  memset(key, '\0', sizeof(key));
  memset(value, '\0', sizeof(value));

  strcpy(filename, "/tmp/metaneos_file2");

  memset(temp_str, '\0', sizeof(temp_str));
  sprintf(temp_str, "/unsup/condor/bin/condor_status -l %s > %s", mach_name, filename);

  if (system(temp_str) < 0)
    {

      MWprintf( 10, "Error occurred during attempt to get condor_status for %s.\n",  mach_name );
      return;
    }

  if ((inputfile = fopen(filename, "r")) == 0)
    {
      MWprintf( 10, "Failed to open condor_status file!\n");
      return;
    }
  else
    {
      MWprintf( 90, "Successfully opened condor_status file.\n");      
    }

  while (fgets(raw_line, 1500, inputfile) != 0)
    {
      found = 0;
      equal_pos = strchr(raw_line, '=');

      if (equal_pos != NULL)
	{
	  strncpy(key, raw_line, equal_pos - (raw_line+1));
	  strcpy(value, equal_pos+2);

	  if (CondorLoadAvg == NO_VAL && !found)
	    {
	      CondorLoadAvg = check_for_float_val("CondorLoadAvg", key, value);
	      if (CondorLoadAvg != NO_VAL)
		found = 1;
	    }

	  if (LoadAvg == NO_VAL && !found)
	    {
	      LoadAvg = check_for_float_val("LoadAvg", key, value);
	      if (LoadAvg != NO_VAL)
		found = 1;
	    }

	  if (Memory == NO_VAL && !found)
	    {
	      Memory = check_for_int_val("Memory", key, value);
	      if (Memory != NO_VAL)
		found = 1;
	    }

	  if (Cpus == NO_VAL && !found)
	    {
	      Cpus = check_for_int_val("Cpus", key, value);
	      if (Cpus != NO_VAL)
		found = 1;
	    }

	  if (VirtualMemory == NO_VAL && !found)
	    {
	      VirtualMemory = check_for_int_val("VirtualMemory", key, value);
	      if (VirtualMemory != NO_VAL)
		found = 1;
	    }

	  if (Disk == NO_VAL && !found)
	    {
	      Disk = check_for_int_val("Disk", key, value);
	      if (Disk != NO_VAL)
		found = 1;
	    }

	  if (KFlops == NO_VAL && !found)
	    {
	      KFlops = check_for_int_val("KFlops", key, value);
	      if (KFlops != NO_VAL)
		found = 1;
	    }

	  if (Mips == NO_VAL && !found)
	    {
	      Mips = check_for_int_val("Mips", key, value);
	      if (Mips != NO_VAL)
		found = 1;
	    }

	  
	  if ( (strncmp(Arch, zero_string, 1) == 0) && !found)
	    {	     
	      if (check_for_string_val("Arch", key, value) == 0){
	       strncpy( Arch, value, sizeof(Arch) );	       
	      }
	     
	     if (strncmp(Arch, zero_string, 1) != 0)
	       found = 1;
	    }

	  if ( (strncmp(OpSys, zero_string, 1) == 0) && !found)
	    {	     
	      if (check_for_string_val("OpSys", key, value) == 0){
	       strncpy( OpSys, value, sizeof(OpSys) );	       
	      }
	     
	     if (strncmp(OpSys, zero_string, 1) != 0)
	       found = 1;
	    }

	  if ( (strncmp(IPAddress, zero_string, 1) == 0) && !found)
	    {	     
	      if (check_for_string_val("StartdIpAddr", key, value) == 0){
	       strncpy(IPAddress , value, sizeof(IPAddress) );	       
	      }
	     
	     if (strncmp(IPAddress, zero_string, 1) != 0)
	       found = 1;
	    }


	  memset(key, '\0', sizeof(key));
	  memset(value, '\0', sizeof(value));

	}

    }

  MWprintf(90,"CURRENT MACHINE  : %s \n", mach_name);
  MWprintf(90,"Architecture : %s \n", Arch);
  MWprintf(90,"Operating System : %s \n", OpSys);
  MWprintf(90,"IP address : %s \n", IPAddress);

  MWprintf(90,"CondorLoadAvg : %f\n", CondorLoadAvg);
  MWprintf(90,"LoadAvg : %f\n", LoadAvg);
  MWprintf(90,"Memory : %d\n", Memory);
  MWprintf(90,"Cpus : %d\n", Cpus);
  MWprintf(90,"VirtualMemory : %d\n", VirtualMemory);
  MWprintf(90,"Disk : %d\n", Disk);
  MWprintf(90,"KFlops : %d\n", KFlops);
  MWprintf(90,"Mips : %d\n", Mips);

  fclose( inputfile );

  if (remove(filename) != 0)
    {
       MWprintf(10,"Condor status file NOT removed!\n");
     }
   else
     {
       MWprintf(90,"Condor status file removed.\n");
     }


}
#endif

