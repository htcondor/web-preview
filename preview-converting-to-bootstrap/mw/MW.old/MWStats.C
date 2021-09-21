/* stats.C

   Implementation of Statistics class
*/

#include "MWStats.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

const int MWStatistics::MAX_WORKERS = 1000;

MWStatistics::MWStatistics() {

	uptimes = new double[MAX_WORKERS];
	workingtimes = new double[MAX_WORKERS];
	susptimes = new double[MAX_WORKERS];
	cputimes = new double[MAX_WORKERS];

	for ( int i=0 ; i<MAX_WORKERS ; i++ ) {
		uptimes[i] = workingtimes[i] = susptimes[i] = cputimes[i] = 0.0;
	}

    duration = 0;
		// start in the constructor...
	struct timeval t;
	gettimeofday ( &t, NULL );
	starttime = (double) t.tv_sec + ( (double) t.tv_usec * (double) 1e-6 );
	previoustime = 0.0;
}


MWStatistics::~MWStatistics() {
    
	delete [] uptimes;
	delete [] workingtimes;
	delete [] susptimes;
	delete [] cputimes;
}

void MWStatistics::write_checkpoint( FILE *cfp, MWWorkerID *wlist ) {
		/* write out stats for each worker Vid.  This is a pain in 
		   my ass.  We have to find the maximum Vid in the system;
		   in both the old stats and the running workers.  Then, for
		   each Vid, we have to sum the old stats + the new running
		   worker stats, and write them out one at a time.... */
	
	MWWorkerID *wkr;  // My eternal temporary workerID...
	int sn_workers = get_max_vid();  // stats now says max is...
	wkr = wlist;
	int run_workers=0;
	while ( wkr ) {
		if ( wkr->get_vid() > run_workers ) {
			run_workers = wkr->get_vid();
		}
		wkr = wkr->next;
	}
	run_workers++;   // there are this many running

	int max = 0;  // get the max of the two...
	if ( run_workers > sn_workers ) {
		max = run_workers;
	} else {
		max = sn_workers;
	}

//	MWprintf ( 10, "sn: %d, run:%d, max:%d\n", sn_workers, run_workers, max );

	double *u = new double[max];
	double *w = new double[max];
	double *s = new double[max];
	double *c = new double[max];

	int i;
	for ( i=0 ; i<max ; i++ ) {
		u[i] = uptimes[i];
		w[i] = workingtimes[i];
		s[i] = susptimes[i];
		c[i] = cputimes[i];
	}

	double up, wo, su, cp;
	wkr = wlist; 
	while ( wkr ) {
		wkr->ckpt_stats( &up, &wo, &su, &cp );
		u[wkr->get_vid()] += up;
		w[wkr->get_vid()] += wo;
		s[wkr->get_vid()] += su;
		c[wkr->get_vid()] += cp;
		wkr = wkr->next;
	}

	struct timeval t;
	gettimeofday ( &t, NULL );
	double now = (double) t.tv_sec + ( (double) t.tv_usec * (double) 1e-6 );
	
	fprintf ( cfp, "%d %15.5f\n", max, ((now-starttime)+previoustime) );
	for ( i=0 ; i<max ; i++ ) {
//		MWprintf ( 10, "%15.5f %15.5f %15.5f %15.5f\n", 
//				   u[i], w[i], s[i], c[i] );
		fprintf ( cfp, "%15.5f %15.5f %15.5f %15.5f\n", 
				  u[i], w[i], s[i], c[i] );
	}

	delete [] u;
	delete [] w;
	delete [] s;
	delete [] c;	
}

void MWStatistics::read_checkpoint( FILE *cfp ) {
	
	int n=0;
	int i;
	fscanf ( cfp, "%d %lf", &n, &previoustime );
	MWprintf ( 10, "num stats to read: %d prev: %f\n", n, previoustime );
	for ( i=0 ; i<n ; i++ ) {
		fscanf ( cfp, "%lf %lf %lf %lf", &uptimes[i], &workingtimes[i], 
				 &susptimes[i], &cputimes[i] );
	//	MWprintf ( 10, "%15.5f %15.5f %15.5f %15.5f\n", uptimes[i], 
	//			   workingtimes[i], susptimes[i], cputimes[i] );
	}
}

void MWStatistics::gather( MWWorkerID *w ) {

    if ( w == NULL ) {
        MWprintf ( 10, "Tried to gather NULL worker!\n" );
        return;
    }

	MWprintf ( 80, "Gathering: worker vid %d\n", w->get_vid() );

	uptimes[w->get_vid()] += w->total_time;
	workingtimes[w->get_vid()] += w->total_working;
	susptimes[w->get_vid()] += w->total_suspended;	
	cputimes[w->get_vid()] += w->cpu_while_working;
		/* we don't delete w here.  That's done in RMC->removeWorker() */
}

void MWStatistics::makestats() {

	int i;
	int numWorkers = get_max_vid();
	double sumuptimes = 0.0;
	double sumworking = 0.0;
	double sumcpu = 0.0;
	double sumsusp = 0.0;

    MWprintf ( 20, "**** Statistics ****\n" );

	MWprintf ( 70, "Dumping raw stats:\n" );
	MWprintf ( 70, "Vid    Uptimes     Working     CpuUsage   Susptime\n" );
	for ( i=0 ; i<numWorkers ; i++ ) {
		if ( uptimes[i] > 34560000 ) { /* 400 days! */
			MWprintf ( 20, "Found odd uptime[%d] = %12.4f\n", i, uptimes[i] );
			uptimes[i] = 0;
		}
		MWprintf ( 70, "%3d  %10.4f  %10.4f  %10.4f %10.4f\n", i, 
				   uptimes[i], workingtimes[i], cputimes[i], susptimes[i] );
		sumuptimes += uptimes[i];
		sumworking += workingtimes[i];
		sumcpu += cputimes[i];
		sumsusp += susptimes[i];
	}
	
        // find duration of this run (+= in case of checkpoint)
	struct timeval t;
	gettimeofday ( &t, NULL );
	double now = (double) t.tv_sec + ( (double) t.tv_usec * (double) 1e-6 );
    duration = now - starttime;
	duration += previoustime;  // so it's sum over all checkpointed work

	MWprintf ( 20, "\n" );
    MWprintf ( 20, "Number of workers:                          %d\n", 
			   numWorkers);
    
    MWprintf ( 20, "Wall clock time for this job:             %10.4f\n", 
			   duration );

	MWprintf ( 20, "Total time workers were alive (up):       %10.4f\n", 
			   sumuptimes );
	MWprintf ( 20, "Total wall clock time of workers:         %10.4f\n", 
			   sumworking );
	MWprintf ( 20, "Total cpu time used by all workers:       %10.4f\n", 
			   sumcpu );
	MWprintf ( 20, "Total time workers were suspended:        %10.4f\n", 
			   sumsusp );

    double uptime_mean = mean( uptimes, numWorkers );
    double uptime_var  = var ( uptimes, numWorkers, uptime_mean );
    double wktime_mean = mean( workingtimes, numWorkers );
    double wktime_var  = var ( workingtimes, numWorkers, wktime_mean );
	double cputime_mean= mean( cputimes, numWorkers );
	double cputime_var = var ( cputimes, numWorkers, cputime_mean );
    double sptime_mean = mean( susptimes, numWorkers );
    double sptime_var  = var ( susptimes, numWorkers, sptime_mean );

    MWprintf ( 20,"Mean & Var. uptime for the workers:       %10.4f\t%10.4f\n",
             uptime_mean, uptime_var );
    MWprintf ( 20,"Mean & Var. working time for the worker:  %10.4f\t%10.4f\n",
             wktime_mean, wktime_var );
    MWprintf ( 20,"Mean & Var. cpu time for the workers:     %10.4f\t%10.4f\n",
             cputime_mean, cputime_var );
    MWprintf ( 20,"Mean & Var. susp. time for the workers:   %10.4f\t%10.4f\n",
             sptime_mean, sptime_var );

}

double MWStatistics::mean( double *array, int length ) {
    double sum=0;
    for ( int i=0 ; i<length ; i++ ) {
        sum += array[i];
    }
    sum /= length;
    return sum;
}

double MWStatistics::var( double *array, int length, int mean ) {
	double ret=0;
	double diff=0;
	for ( int i=0 ; i<length ; i++ ) {
		diff = array[i] - mean;
		ret += ( diff * diff );
	}
	if( length > 1 )
		ret /= length-1;
	return ret;
}

int MWStatistics::get_max_vid() {
	int n = -1;
	for ( int i=0 ; i<MAX_WORKERS ; i++ ) {
		if ( uptimes[i] > 0.0 ) {
			n = i;
		}
	}
	n++;
	return n;
}
