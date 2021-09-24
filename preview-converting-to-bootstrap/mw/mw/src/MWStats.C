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


MWStatistics::MWStatistics() {

	uptimes = new double[MW_MAX_WORKERS];
	workingtimes = new double[MW_MAX_WORKERS];
	susptimes = new double[MW_MAX_WORKERS];
	cputimes = new double[MW_MAX_WORKERS];

	workingtimesnorm  = new double[MW_MAX_WORKERS];

	sumbenchmark = new double[MW_MAX_WORKERS];
	numbenchmark = new int[MW_MAX_WORKERS];

	max_bench_result = 0.0;
	min_bench_result = DBL_MAX;

	for ( int i=0 ; i<MW_MAX_WORKERS ; i++ ) {
		uptimes[i] = workingtimes[i] = susptimes[i] = cputimes[i] = 0.0;
		workingtimesnorm[i] = sumbenchmark[i] = 0.0;
		numbenchmark[i] = 0;
	}

    duration = 0;
		// start in the constructor...
	struct timeval t;
	gettimeofday ( &t, NULL );
	starttime = (double) t.tv_sec + ( (double) t.tv_usec * 1e-6 );
	previoustime = 0.0;
}


MWStatistics::~MWStatistics() {
    
	delete [] uptimes;
	delete [] workingtimes;
	delete [] susptimes;
	delete [] cputimes;

	delete [] sumbenchmark;
	delete [] numbenchmark;
	delete [] workingtimesnorm;



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
  
  MWprintf ( 10, "In checkpointing -- sn: %d, run:%d, max:%d\n", sn_workers, run_workers, max );

  double *u  = new double[max];
  double *w  = new double[max];
  double *s  = new double[max];
  double *c  = new double[max];
  double *wn  = new double[max];
  double *sb = new double[max];	
  int    *nb = new int[max];

  int i;
  for ( i=0 ; i<max ; i++ ) {
    u[i] = uptimes[i];
    w[i] = workingtimes[i];
    s[i] = susptimes[i];
    c[i] = cputimes[i];
    wn[i] = workingtimesnorm[i];
    sb[i] = sumbenchmark[i];
    nb[i] = numbenchmark[i];
  }

  double up, wo, su, cp,  tn, sumb;
  int numb;
  
  wkr = wlist; 
  while ( wkr ) {
    wkr->ckpt_stats( &up, &wo, &su, &cp,  &tn, &sumb, &numb );
    
    u[wkr->get_vid()] += up;
    w[wkr->get_vid()] += wo;
    s[wkr->get_vid()] += su;
    c[wkr->get_vid()] += cp;
    wn[wkr->get_vid()] += tn;
    sb[wkr->get_vid()] += sumb;
    nb[wkr->get_vid()] += numb;
    
    wkr = wkr->next;
  }

  struct timeval t;
  gettimeofday ( &t, NULL );
  double now = (double) t.tv_sec + ( (double) t.tv_usec * (double) 1e-6 );
  
  fprintf ( cfp, "%d %15.5f\n", max, ((now-starttime)+previoustime) );
  for ( i=0 ; i<max ; i++ ) {
    //		MWprintf ( 10, "%15.5f %15.5f %15.5f %15.5f\n", 
    //				   u[i], w[i], s[i], c[i] );
    fprintf ( cfp, "%15.5f %15.5f %15.5f %15.5f %15.5f %15.5f %d\n", 
	      u[i], w[i], s[i], c[i] , wn[i], sb[i], nb[i]);
  }

  fprintf ( cfp, "%15.5f %15.5f\n", max_bench_result, min_bench_result);

  delete [] u;
  delete [] w;
  delete [] s;
  delete [] c;	
  delete [] wn;
  delete [] sb;
  delete [] nb;;


}


//This is the good one
void MWStatistics::read_checkpoint( FILE *cfp ) {
	
	int n=0;
	int i;
	fscanf ( cfp, "%d %lf", &n, &previoustime );
	MWprintf ( 10, "num stats to read: %d prev: %f\n", n, previoustime );
	for ( i=0 ; i<n ; i++ ) {

		fscanf ( cfp, "%lf %lf %lf %lf %lf %lf %d", 
			 &uptimes[i],
			 &workingtimes[i], 
			 &susptimes[i],
			 &cputimes[i], 
			 &workingtimesnorm[i],
			 &sumbenchmark[i],
			 &numbenchmark[i]);

		MWprintf ( 10, "%d %15.5f %15.5f %15.5f %15.5f %15.5f %15.5f %d\n", 
			   i,
			   uptimes[i], 
			   workingtimes[i], 
			   susptimes[i], 
			   cputimes[i],
			   workingtimesnorm[i],
			   sumbenchmark[i],
			   numbenchmark[i]);
	}

	fscanf ( cfp, "%lf %lf", &max_bench_result, &min_bench_result);
}




void MWStatistics::gather( MWWorkerID *w ) {

    if ( w == NULL ) {
        MWprintf ( 10, "Tried to gather NULL worker!\n" );
        return;
    }

	MWprintf ( 80, "Gathering: worker vid %d\n", w->get_vid() );

	//JTL 8/25/00
	// We need to update the total time to include this time,
	// because now we can call gather() without calling ended()

	//  This is starting top get a bit kludgy, but the main
	//  sticking point is that we don't want the "kill workers"
	//  time in our efficiency numbers.

	struct timeval t;
	gettimeofday ( &t, NULL );
	double now = (double) t.tv_sec + ( (double) t.tv_usec * (double) 1e-6 );
	
	if( w->start_time > 1.0 ) {
	  w->total_time = now - w->start_time;
	}
	
	uptimes[w->get_vid()] += w->total_time;
	workingtimes[w->get_vid()] += w->total_working;
	susptimes[w->get_vid()] += w->total_suspended;	
	cputimes[w->get_vid()] += w->cpu_while_working;

	workingtimesnorm[w->get_vid()] += (w->normalized_cpu_working_time);

	sumbenchmark[w->get_vid()] += (w->sum_benchmark);
	numbenchmark[w->get_vid()] += (w->num_benchmark);


	// Jeff resets all these to 0 -- so we can call gather 
	//  more than once without affecting the stats validity

	w->total_time = 0.0;
	w->total_working = 0.0;
	w->total_suspended = 0.0;
	w->cpu_while_working = 0.0;
	w->normalized_cpu_working_time = 0.0;
	w->sum_benchmark = 0.0;
	w->num_benchmark = 0;
	
	
		/* we don't delete w here.  That's done in RMC->removeWorker() */
}

void MWStatistics::makestats() {

	int i;
	int numWorkers = get_max_vid();
	double sumuptimes = 0.0;
	double sumworking = 0.0;
	double sumcpu = 0.0;
	double sumsusp = 0.0;

	double sumworknorm = 0.0;

	double sum_sum_benchmark = 0.0;
	int    sum_num_benchmark = 0;

    MWprintf ( 20, "**** Statistics ****\n" );

	MWprintf ( 20, "Dumping raw stats:\n" );
	MWprintf ( 20, "Vid    Uptimes     Working     CpuUsage   Susptime\n" );
	for ( i=0 ; i<numWorkers ; i++ ) {
		if ( uptimes[i] > 34560000 ) { /* 400 days! */
			MWprintf ( 20, "Found odd uptime[%d] = %12.4f\n", i, uptimes[i] );
			uptimes[i] = 0;
		}
		
		MWprintf ( 20, "%3d  %10.4f  %10.4f  %10.4f %10.4f %10.4f %10.4f %d \n",
			   i, 
			   uptimes[i], 
			   workingtimes[i], 
			   cputimes[i], 
			   susptimes[i], 
			   workingtimesnorm[i], 
			   sumbenchmark[i], 
			   numbenchmark[i] );


		sumuptimes += uptimes[i];
		sumworking += workingtimes[i];
		sumcpu += cputimes[i];
		sumsusp += susptimes[i];
		sumworknorm  += workingtimesnorm[i];
		sum_sum_benchmark += sumbenchmark[i];
		sum_num_benchmark += numbenchmark[i];

	}
	
        // find duration of this run (+= in case of checkpoint)
	struct timeval t;
	gettimeofday ( &t, NULL );
	double now = (double) t.tv_sec + ( (double) t.tv_usec * (double) 1e-6 );
    duration = now - starttime;
	duration += previoustime;  // so it's sum over all checkpointed work

	MWprintf ( 20, "\n" );
    MWprintf ( 20, "Number of (different) workers:            %d\n", 
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

	double average_bench = ( sum_sum_benchmark / sum_num_benchmark );
	double equivalent_bench = ( sumworknorm / sumcpu );


	MWprintf (20, "Averaged      benchmark   factor    :      %10.4f\n", average_bench);
	MWprintf (20, "Equivalent    benchmark   factor    :      %10.4f\n", equivalent_bench);
	MWprintf (20, "Minimum       benchmark   factor    :      %10.4f\n", min_bench_result);
	MWprintf (20, "Maximum       benchmark   factor    :      %10.4f\n\n", max_bench_result);

	double av_present_workers = ( sumuptimes / duration ) ;
	double av_nonsusp_workers = ( ( sumuptimes - sumsusp) / duration ) ;
	double av_active_workers  = ( ( sumcpu ) / duration ) ;

	MWprintf (20, "Average Number Present Workers      :      %10.4f\n", av_present_workers);
	MWprintf (20, "Average Number NonSuspended Workers :      %10.4f\n", av_nonsusp_workers);
	MWprintf (20, "Average Number Active Workers       :      %10.4f\n", av_active_workers);

	double equi_pool_performance = ( equivalent_bench * av_active_workers );
	double equi_run_time = ( sumworknorm );
	double parallel_performance  = (( sumworking ) / ( sumuptimes - sumsusp)) ;

	MWprintf (20, "Equivalent Pool Performance         :      %10.4f\n", equi_pool_performance);
	MWprintf (20, "Equivalent Run Time                 :      %10.4f\n\n", equi_run_time);

	MWprintf (20, "Overall Parallel Performance        :      %10.4f\n", parallel_performance);

	MWprintf (20, "Total Number of benchmark tasks     :      %10d\n\n",  sum_num_benchmark);

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


void MWStatistics::get_stats(double *average_bench,
			     double *equivalent_bench,
			     double *min_bench,
			     double *max_bench,
			     double *av_present_workers,
			     double *av_nonsusp_workers,
			     double *av_active_workers,
			     double *equi_pool_performance,
			     double *equi_run_time,
			     double *parallel_performance,
			     double *wall_time,
			     MWWorkerID *w
			     )
{

  int i;
  int numWorkers = get_max_vid();
  double sumuptimes = 0.0;
  double sumworking = 0.0;
  double sumcpu = 0.0;
  double sumsusp = 0.0;
  double sumworknorm = 0.0;
  
  double sum_sum_benchmark = 0.0;
  int    sum_num_benchmark = 0;

  for ( i=0 ; i<numWorkers ; i++ ) {
    if ( uptimes[i] > 34560000 ) { /* 400 days! */
      MWprintf ( 20, "Found odd uptime[%d] = %12.4f\n", i, uptimes[i] );
      uptimes[i] = 0;
    }

    sumuptimes += uptimes[i];
    sumworking += workingtimes[i];
    sumsusp += susptimes[i];
    sumcpu += cputimes[i];
    sumworknorm  += workingtimesnorm[i];
    sum_sum_benchmark += sumbenchmark[i];
    sum_num_benchmark += numbenchmark[i];

  }

  // find duration of this run (+= in case of checkpoint)
  struct timeval t;
  gettimeofday ( &t, NULL );
  double now = (double) t.tv_sec + ( (double) t.tv_usec * (double) 1e-6 );
  duration = now - starttime;
  duration += previoustime;  // so it's sum over all checkpointed work

  MWWorkerID *tempw = w;
  while( tempw ) {
    if( tempw->start_time > 0.1 )
      sumuptimes += now - tempw->start_time;

    sumworking += tempw->total_working;
    sumsusp += tempw->total_suspended;	
    sumcpu += tempw->cpu_while_working;

    sumworknorm += (tempw->normalized_cpu_working_time);

    sum_sum_benchmark += (tempw->sum_benchmark);
    sum_num_benchmark += (tempw->num_benchmark);


    MWprintf(10, "Stats, id: %d\t%lf, %lf, %lf, %lf, %lf, %lf, %d\n",
	     tempw->id1, sumuptimes, sumworking, sumsusp, sumcpu, sumworknorm, 
	     sum_sum_benchmark, sum_num_benchmark );

    tempw = tempw->next;

  }
	
  *wall_time = duration;

  *average_bench = ( sum_sum_benchmark / sum_num_benchmark );
  *equivalent_bench = ( sumworknorm / sumcpu );
  *min_bench = min_bench_result;
  *max_bench = max_bench_result;
  
  *av_present_workers = ( sumuptimes / duration ) ;
  *av_nonsusp_workers = ( ( sumuptimes - sumsusp ) / duration ) ;
  *av_active_workers  = ( ( sumcpu ) / duration ) ;

  *equi_pool_performance = ( *equivalent_bench * *av_active_workers );
  *equi_run_time = ( sumworknorm );
  *parallel_performance  = (( sumworking ) / ( sumuptimes - sumsusp)) ;



    double uptime_mean = mean( uptimes, numWorkers );
    double uptime_var  = var ( uptimes, numWorkers, uptime_mean );
    double wktime_mean = mean( workingtimes, numWorkers );
    double wktime_var  = var ( workingtimes, numWorkers, wktime_mean );
	double cputime_mean= mean( cputimes, numWorkers );
	double cputime_var = var ( cputimes, numWorkers, cputime_mean );
    double sptime_mean = mean( susptimes, numWorkers );
    double sptime_var  = var ( susptimes, numWorkers, sptime_mean );


}

void MWStatistics::update_best_bench_results ( double bres ) {

  MWprintf(10,"bench = %lf\n", bres);

  if (bres > max_bench_result){
    max_bench_result = bres;
  }

  if (bres < min_bench_result){
    min_bench_result = bres;
  }

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
  for ( int i=0 ; i<MW_MAX_WORKERS ; i++ ) {
    if ( uptimes[i] > 0.0 ) {
      n = i;
    }
  }
  n++;
  return n;
}
