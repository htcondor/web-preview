#ifndef MWNWSTASK_H
#define MWNWSTASK_H

#include <stdio.h>
#include "MWTask.h"

/** The task class specific to NWS weather monitoring
*/

class MWNWSTask : public MWTask {

 public:
    
    /// Default Constructor
    MWNWSTask();

    /** 
    */
    MWNWSTask( double, double );

    MWNWSTask( const MWNWSTask& );

    /// Default Destructor
    ~MWNWSTask();

    MWNWSTask& operator = ( const MWNWSTask& );

    /**@name Implemented methods
       
       These are the task methods that must be implemented 
       in order to create an application.
    */
    //@{
    /// Pack the work for this task into the PVM buffer
    void pack_work( void );
    
    /// Unpack the work for this task from the PVM buffer
    void unpack_work( void );
    
    /// Pack the results from this task into the PVM buffer
    void pack_results( void );
    
    /// Unpack the results from this task into the PVM buffer
    void unpack_results( void );
    //@}

        /// dump self to screen:
    void printself( int level = 60 );

    /**@name Checkpointing Implementation 
       These members used when checkpointing. */
    //@{
        /// Write state
    void write_ckpt_info( FILE *fp );
        /// Read state
    void read_ckpt_info( FILE *fp );
    //@}

    /**@name Input Parameters.
       These parameters are sent to the other side */
    //@{
    	/// the length of measurement.
    double timeLength;
	/// the interval between measurements
    double timeInterval;
    //@}

    /**@name Output Parameters.
       These parameters are returned by the workers */
    //@{
    	/// the maximum b/w observed.
    double max;
    	/// the minimum b/w observed.
    double min ;
    	/// the median b/w observed.
    double median ;
    //@}
};

#endif
