#include "MW.h"
#include "MWWorker.h"
#include "RMComm/MWRMComm.h"
#include <unistd.h>
#include <sys/resource.h>  /* for getrusage */
#include <sys/time.h>      /* for gettimeofday */

MWWorker::MWWorker() 
{
	master = UNDEFINED;

#ifdef INDEPENDENT
	RMC = GlobalRMComm;
#endif
}

MWWorker::~MWWorker() 
{
}

void MWWorker::go( int argc, char *argv[] ) 
{
	int myid;   //actually never used...
#ifdef INDEPENDENT
	RMC = GlobalRMComm;
#else
	MWReturn ustat = OK;
#endif

	MWprintf ( 10, "About to call setup\n");
	RMC->setup( argc, argv, &myid, &master );
	MWprintf ( 10, "Worker %x started.\n", myid );
	
#ifndef INDEPENDENT
	if ( (ustat = worker_setup()) != OK ) {
/* We don't do this exit ourself: the master will kill us. */
/*		RMC->exit(1); */
	}
	worker_mainloop();
}

MWReturn MWWorker::worker_setup() {
#endif

	int len, tag, tid;
	gethostname ( mach_name, 64 );

	MWprintf ( 10, "Worker started on machine %s.\n", mach_name );
	
		/* Pack and send to the master all these information
		   concerning the host and the worker specificities  */
	RMC->initsend();
	RMC->pack( mach_name );
	pack_worker_initinfo();
	
	int status = RMC->send( master, INIT );
	MWprintf ( 10, "Sent the master %x an INIT message.\n", master );
	
	if ( status < 0 ) {
		MWprintf ( 10, "Had a problem sending my name to master.  Exiting.\n");
		RMC->exit(1);
	}
	
#ifdef INDEPENDENT
	return;
}

MWReturn MWWorker::worker_setup() 
{
    return OK;
}

MWReturn MWWorker::ind_benchmark ( )
{
	
#endif
		// wait for the setup info from the master 
	
	int buf_id = RMC->recv( master, -1 );   
	if( buf_id < 0 ) {
		MWprintf ( 10, "Had a problem receiving INIT_REPLY.  Exiting.\n" );
		RMC->exit( INIT_REPLY_FAILURE );
	}
	status = RMC->bufinfo ( buf_id, &len, &tag, &tid );
	
	MWReturn ustat = OK;
		// unpack initial data to set up the worker state

	switch ( tag )
	{
		case INIT_REPLY:
		{

			if ( (ustat = unpack_init_data()) != OK ) {
				int err = -1;
				MWprintf ( 10, "Error unpacking initial data.\n" );
				RMC->initsend();
				RMC->pack( &err, 1 );
				RMC->send( master, BENCH_RESULTS );
				return ustat;
			}

			int bench_tf = FALSE;
			RMC->unpack( &bench_tf, 1 );

			if ( bench_tf ) {
				MWprintf ( 40, "Recvd INIT_REPLY, now benchmarking.\n" );
				workingTask->unpack_work();
				double bench_result = benchmark( workingTask );
				MWprintf ( 40, "Benchmark completed....%f\n", bench_result );
				int zero = 0;
				RMC->initsend();
				RMC->pack( &zero, 1 );  // zero means that unpack_init_data is OK.
				RMC->pack( &bench_result, 1 );
			} else {
				MWprintf ( 40, "Recvd INIT_REPLY, no benchmark.\n" );
				double z = 0.0;
				int zero = 0;
				RMC->initsend();
				RMC->pack( &zero, 1 );  // zero means that unpack_init_data is OK.
				RMC->pack( &z, 1 );
			}
			RMC->send( master, BENCH_RESULTS );

			return ustat;
			break;
		}

		case CHECKSUM_ERROR:
		{
			MWprintf ( 10, "Got a checksum error\n");
			RMC->exit( CHECKSUM_ERROR_EXIT );
		}
	}
}

#ifndef INDEPENDENT
void MWWorker::worker_mainloop() {
	
		/* sit in a very simple little state machine.  
		   Wait for work; get work; do work; return results.  
		   Repeat until asked to kill self. */
	
	int status = 0, len = 0, tag = 0, tid = 0;
	
	for (;;) {
		
#else
MWReturn MWWorker::worker_mainloop_ind () 
{
	int status = 0, len = 0, tag = 0, tid = 0;

#endif
		// wait here for any message from master
		int buf_id = RMC->recv ( master, -1 );

		if( buf_id < 0 ) {
		  MWprintf( 10, "Could not receive message from master.  Exiting\n" );
		  RMC->exit( buf_id );
		}

		status = -2; len = -2; tag = -2; tid = -2;		
		status = RMC->bufinfo ( buf_id, &len, &tag, &tid );

		switch ( tag ) {
			
		case RE_INIT: {   /* This can happen:  the lower level can tell us 
							 that the master has gone down and come back 
							 up, and we hve to re-initialize ourself. */
			worker_setup();
			break;
		}

		case DO_THIS_WORK: {
			
			int num = UNDEFINED;
			MWTaskType thisTaskType;
			double wall_time = 0.0;
			double cpu_time = 0.0;
			struct rusage r;
			struct timeval t;
			int tstat;
			
			tstat = RMC->unpack ( &(int)(thisTaskType), 1, 1);
			if ( tstat != 0 )
			{
				MWprintf ( 10, "Error: The receive buffer not unpacked on %d\n",
					mach_name );
				fflush ( stdout );
				RMC->exit ( UNPACK_FAILURE );
			}

			workingTask->taskType = thisTaskType;

			tstat = RMC->unpack( &num, 1, 1 );
			if( tstat != 0 ) {
			  MWprintf( 10, "Error.  The receive buffer not unpacked on %s\n", 
				    mach_name );
			  fflush( stdout );
			  RMC->exit( UNPACK_FAILURE );
			}

			workingTask->number = num;

			
			MWprintf( 40, " Worker %s got task number %d\n", mach_name, num );
			fflush( stdout );
			
			unpack_driver_task_data();

			workingTask->unpack_work();
			
				/* Set our stopwatch.  :-) */
			gettimeofday ( &t, NULL );
			wall_time -= timeval_to_double( t );
			getrusage ( RUSAGE_SELF, &r );
			cpu_time -= timeval_to_double ( r.ru_utime );
			cpu_time -= timeval_to_double ( r.ru_stime );

				/* do it! */
			switch ( thisTaskType )
			{
				case MWNORMAL:
					execute_task( workingTask );
					break;
				case MWNWS:
					execute_nws ( workingTask );
					break;
			}
				/* Record times for it... */
			gettimeofday ( &t, NULL );
			wall_time += timeval_to_double ( t );
			getrusage ( RUSAGE_SELF, &r );
			cpu_time += timeval_to_double ( r.ru_utime );
			cpu_time += timeval_to_double ( r.ru_stime );

				/* Now send... */
			RMC->initsend();
			RMC->pack( &num, 1, 1 );
			RMC->pack( &wall_time, 1, 1 );
			RMC->pack( &cpu_time, 1, 1 );
			workingTask->pack_results();
			status = RMC->send(master, RESULTS);

			if ( status < 0 ){
			  MWprintf ( 10, "Bummer!  Could not send results of task %d\n", num ); 
			  MWprintf( 10, "Exiting worker!" ); 
			  RMC->exit( FAIL_MASTER_SEND );
			}
                        
		
			MWprintf ( 40, "%s sent results of job %d.\n", 
					   mach_name, workingTask->number );
			break;
			
		}
		case KILL_YOURSELF: {
			suicide();
		}
		case CHECKSUM_ERROR:
		{
			MWprintf ( 10, "Got a checksum error\n");
			RMC->exit( CHECKSUM_ERROR_EXIT );
		}
		default: {
			MWprintf ( 10, "Received strange command %d.\n", tag );
			RMC->exit( UNKNOWN_COMMAND );
		}
		} // switch
#ifndef INDEPENDENT
	}
#else
	return OK;
#endif

}

/* We've received orders to kill ourself; we're not needed anymore.
   Fall on own sword. */

void MWWorker::suicide () 
{   
	MWprintf ( 10, "\"Goodbye, cruel world...\" says %s\n", mach_name );
	
	RMC->exit(0);
}

void
MWWorker::execute_nws ( MWTask *task )
{
}
