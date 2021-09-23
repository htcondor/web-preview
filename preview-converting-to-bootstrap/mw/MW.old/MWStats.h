/* stats.h

   Header file for statistics class.
*/

#ifndef STATISTICS_H
#define STATISTICS_H

#include "MWWorkerID.h"
#include "MW.h"

/** This class serves as a holder for worker classes before they go to die.
    It holds the workers until a makestats() call is made, at which
    point it generates some statistics, given the workers.  It also
    has the ability to make a cool image of that run....
*/
class MWStatistics {

 public:
    /// Default constructor
    MWStatistics();
    /// Destructor...
    ~MWStatistics();

		/** Literally, take the stats from this worker and store 
			them in the stats class.  This is done when the worker
			dies and during a checkpoint.
		 */
    void gather ( MWWorkerID *w );

    /** This method is called at the end of a run.  It prints out some
        statistical information on that run, plus it can write out data
        to a file that can be turned into a gif file representing that run.
    */
    void makestats();

		/** We find that we should save stats across checkpoints.  Here
			we're given the ckpt FILE* and the list of running workers.
			Basically, we write out the sum of the old stats we've got
			plus the stats of the running workers. */
	void write_checkpoint( FILE *cfp, MWWorkerID *wlist );

		/** Read in stats information across the checkpoint */
	void read_checkpoint( FILE *cfp );

 private:

    double duration;
	double starttime;
	double previoustime; // the total run time of the checkpoints before...

	double *uptimes;
	double *workingtimes;
	double *cputimes;
	double *susptimes;

    double mean ( double *array, int length );
    double var ( double *array, int length, int mean );
	int get_max_vid();

	static const int MAX_WORKERS;
};

#endif


