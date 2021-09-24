/* Driver-fib.C

These methods specific to the Fib. application

*/

#include "MW.h"
#include "Driver-fib.h"
#include "Worker-fib.h"
#include "Task-fib.h"

Driver_Fib::Driver_Fib() {
    max_seed_num = 0;
    num_tasks = 0;
    seqlen = 0;
    completedTasks = NULL;
}

Driver_Fib::~Driver_Fib() {

    if ( completedTasks ) {
        for ( int i=0 ; i < num_tasks ; i++ )
            if ( completedTasks[i] ) 
                delete completedTasks[i];
        delete [] completedTasks;
    }
}

MWReturn Driver_Fib::get_userinfo( int argc, char *argv[] ) {

    /* File format of the input file (which is stdin)
	     2   # arches
		 worker_executable 
		 worker_executable

         maximum seed number
         sequence length
		 checkpoint frequency/time.
    */

	int i;

    for ( i=0 ; i<argc ; i++ ) {
        MWprintf ( 70, "arg %d: %s\n", i, argv[i] );
    }

	int ckpt_frq = 0;
	int numarches = 0;
	char exec[256];

	scanf ( "%d", &numarches );
	MWprintf ( 30, "%d worker executable%s", numarches, 
			   numarches==1 ? ":" : "s:\n" );
	RMC->set_num_arches( numarches );
	MWprintf ( 10, "Set the arches to %d\n", numarches );
	for ( i=0 ; i<numarches ; i++ ) {
		scanf ( "%s", exec );
		MWprintf ( 30, " %s\n", exec );
		if ( i == 1 )
			RMC->set_worker_attributes( i, exec, 
				"((Arch==\"INTEL\") && (Opsys==\"SOLARIS26\"))" );
		else
			RMC->set_worker_attributes( i, exec, 
				"((Arch==\"INTEL\") && (Opsys==\"LINUX\"))" );
//		RMC->set_worker_attributes( i, exec, "" );
				
	} 
	
    scanf ( "%d", &max_seed_num );
    scanf ( "%d", &seqlen );
	scanf ( "%d", &ckpt_frq );

    num_tasks = (max_seed_num * (max_seed_num+1)) / 2;
    MWprintf(10, "The numer of tasks is %d\n", num_tasks );
	set_checkpoint_frequency ( 1000000 );

    MWprintf ( 30, "Max seed number = %d, num_tasks = %d\n", 
			   max_seed_num,num_tasks);
    MWprintf ( 30, "Sequence length = %d\n", seqlen );
	MWprintf ( 30, "Checkpoint frequency = %d\n", ckpt_frq );

	target = 40; 
	set_target_num_workers( target );

	register_benchmark_task ( new Task_Fib ( 8,8,8 ) );

		/* Testing the REASSIGN policy... */
	return OK;
}

MWReturn Driver_Fib::setup_initial_tasks(int *n_init , MWTask ***init_tasks) {

/* Here we set up an array of tasks that will be present in the todo
   list when the application starts.  We will pass this array back 
   through the init_tasks pointer.  It's length will be passed back
   through the n_init pointer. */

/* We first make an array of pointers to MWTasks... */

    *n_init = num_tasks;
    *init_tasks = new MWTask *[num_tasks];

/* completedTasks is a list of tasks which have been completed.  It is 
   private to the Driver_Fib class.  Here, we simply new some space for
   the array which will hold the tasks when they're done.  Note that 
   your application may not need to hold old tasks like this one does.
   Many applications simply throw tasks away when they're completed. */

    completedTasks = new Task_Fib*[num_tasks];
    for ( int i=0 ; i < num_tasks ; i++ ) {
        completedTasks[i] = NULL;
    }

/* And now we make the Task_Fib instances.  This for() loop
   creates pairs of numbers that follow the pattern {1,1},{1,2},{1,3}, ...
   {1,max_seed_num},{2,2},{2,3},...{2,max_seed_num},{3,3}....etc */

    int first  = 1;
    int second = 1;
	for ( int taskindex = 0 ; taskindex < num_tasks ; taskindex++ ) {
		(*init_tasks)[taskindex] = new Task_Fib( first, second, seqlen );

		second++;
        if ( second > max_seed_num ) {
            first++;
            second = first;
        }
    }
	return OK;
}



MWReturn Driver_Fib::act_on_completed_task( MWTask *t ) {

/* Our job here is simple:  Take the task and add it to the list of 
   completed tasks. */

#ifdef NO_DYN_CAST
	Task_Fib *tf = (Task_Fib *) t;
#else
    Task_Fib *tf = dynamic_cast<Task_Fib *> ( t );
#endif

    completedTasks[tf->number] = new Task_Fib( *tf );
/*
	if ( tf->number % 100 == 0 ) {
		target -= 7;
		set_target_num_workers( target );
	}
*/
	return OK;
}

MWReturn Driver_Fib::pack_worker_init_data( void ) {
    // The following gets unpacked in Worker_Fib::unpack_init_data( void ).
    // Here, one would normally pack up a matrix or some other relevant 
    // starting data... We don't need that for this application, so we'll
    // just send along a silly string:
    RMC->pack( "Get to work!" );
	return OK;
}

void Driver_Fib::printresults() {

/* print out everything in the completedtasks list. */

    for ( int i = 0 ; i < num_tasks ; i++ ) {
        if ( completedTasks[i] != NULL ) {
            for ( int j = 0 ; j < seqlen ; j++ ) {
                MWprintf ( 20, "%d\t", completedTasks[i]->results[j] );
            }
        }
        else {
            MWprintf ( 10, "NULL!" );
        }
        MWprintf ( 20, "\n" );
    }
}

void
Driver_Fib::write_master_state( FILE *fp ) {

/* Used for checkpointing. */

    fprintf ( fp, "%d %d %d %d\n", max_seed_num, num_tasks, seqlen, target );

    for ( int i=0 ; i<num_tasks ; i++ ) {
        if ( completedTasks[i] ) {
            fprintf ( fp, "%d ", i );
            completedTasks[i]->write_ckpt_info( fp );
        } else {
            fprintf ( fp, "-1\n" );
        }
    }

}

void 
Driver_Fib::read_master_state( FILE *fp ) {

/* Also used for checkpointing. */

    if ( completedTasks ) {
        MWprintf ( 10, "You can't call read_master_state() now!\n" );
        return;
    }
    MWprintf ( 10, "In read_master_state" );

    fscanf ( fp, "%d %d %d %d", &max_seed_num, &num_tasks, &seqlen, &target );

    int num;

    MWprintf ( 10, "In read_master_state" );
    completedTasks = new Task_Fib*[num_tasks];
    MWprintf ( 10, "In read_master_state" );
    for ( int i=0 ; i<num_tasks ; i++ ) {
        fscanf ( fp, "%d", &num );
        if ( num != -1 ) {
                // sanity check
            if ( num != i ) {
                MWprintf ( 10, "Huh?, num = %d, i = %d\n", num, i );
            }
            completedTasks[i] = new Task_Fib;
            completedTasks[i]->number = num;
            completedTasks[i]->read_ckpt_info( fp );
            //completedTasks[i]->printself( 80 );
        }
        else {
            completedTasks[i] = NULL;
            //MWprintf ( 10, "%d: no task\n", i );
        }
    }
}

MWTask*
Driver_Fib::gimme_a_task() {
	/* The MWDriver needs this.  Just return a Task_Fib instance. */
    return new Task_Fib;
}

MWDriver*
gimme_the_master () 
{
    return new Driver_Fib;
}


