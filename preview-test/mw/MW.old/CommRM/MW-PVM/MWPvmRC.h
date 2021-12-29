#ifndef MWPVMRC_H
#define MWPVMRC_H

#include "../MWRMComm.h"
extern "C" {
#include "pvm3.h"
}

/// A mask for getting rid of that annoying high set bit.
#define PVM_MASK 0x7fffffff

/** 
	A Resource Management and Communication class that uses PVM 
	for underlying support of inter-process communication and 
	resource management.

	We *could* do cool things here...like keep track of amount of
	data sent, to whom, etc.  We could also MWprintf ( 99, "..." )
	everything that gets sent for heaps and heaps of debugging.

*/

class MWPvmRC : public MWRMComm {
	
 public:
		/// Constructor...
	MWPvmRC() { 
		hostadd_reqs = NULL;
		is_master = FALSE;   // assume the worst...
	};
		/// Destructor...
	~MWPvmRC() {
		if ( hostadd_reqs ) delete [] hostadd_reqs;
	};

		/** @name A. Resource Management Routines

			Here we implement the pure virtual functions found in 
			ur parent class, MWRMComm.
		*/
		//@{

		/** Shut down.  Calls pvm_exit(), then ::exit(exitval). */
	void exit( int exitval );

		/** Initialization.  Does pvm_catchout(), pvm_parent(), 
			and pvm_notifies(). */
	int setup( int argc, char *argv[], int *my_id, int *master_id );

		/** Does a pvm_config, and stuffs the returned hostinfo
		    struct information into a lot of MWWorkerID's.  See
		    the parent class for return details.  */
	int config( int *nhosts, int *narches, MWWorkerID ***workers );

		/** Basically do a pvm_spawn.  We also put some information
			into w upon return.
			@return The tid spawned.  Negative number on error.
		*/
	int start_worker ( MWWorkerID *w );
	
		/** Start up the workers that are given to us at the beginning. 
			See base class comments for more details.
			@param nworkers The number of workers at start
			@param workers A pointer to an array of pointers to 
			       MWWorkerID classes.  This call will new() memory 
				   for the MWWorkerIDs.  Also, if (*w)[n]->id2 is
				   -1, that means that the spawn failed for 
				   worker number n.
		*/
	int init_beginning_workers ( int *nworkers, MWWorkerID ***workers );

		/** Remove a worker.  Basically, call pvm_delhosts(). */
	int removeWorker( MWWorkerID *w );

		/** Read some state. A null function */
	int read_RMstate ( FILE *fp );

		/** Write some state. A null function */
	int write_RMstate ( FILE *fp );

		//@}

 private:

		/** Figure out wether or not to ask for hosts, and how many... 
			@param num_workers An array of size num_arches that contains
			the number of workers in each arch class.
		 */
	int hostaddlogic( int *num_workers );

		/** Do the pvm_spawn and associated stuff */
	int do_spawn( MWWorkerID *w );

		/** Set up the proper notifies for a task & host tid */
	int setup_notifies ( int task_tid );

		/** This function says to pvm: "I would like another machine, 
			please".
			@param howmany The number of machines to request
			@param archnum The arch class number to ask for. */
	int ask_for_host( int howmany, int archnum );

		/** Shows the pvm virtual machine according to pvm.  Used
			for debugging purposes; not normally called. */
	void conf();
	
		/** Helper for hostaddlogic().  Returns the index of the min
			element in array, which is of length len */
	int min ( int *array, int len );

		/** The number of outstanding requests for a particular 
			arch. */
	int *hostadd_reqs;

		//@}

		/** Am I the master process or not? */
	int is_master;

 public:


		/** @name C. Communication Routines
			
			These are essentially thin wrappers of PVM calls.
		*/
		//@{

		///
	int initsend ( int encoding = PvmDataDefault) { 
		return pvm_initsend ( encoding ); 
	}
		///
	int send ( int to_whom, int msgtag ) {
		return pvm_send ( to_whom, msgtag );
	}
		///
	int recv ( int from_whom, int msgtag ) {
		return pvm_recv ( from_whom, msgtag );
	}

		/** Provide info on the message just received */
	int bufinfo ( int buf_id, int *len, int *tag, int *from ) {
			/* We and the last arg with PVM_MASK to clear out that 
			   annoying high bit! */
		*from &= PVM_MASK;
		return pvm_bufinfo ( buf_id, len, tag, from );
	}

		/// Pack some bytes
	int pack ( char *bytes,         int nitem, int stride = 1 ) {
		return pvm_pkbyte ( bytes, nitem, stride );
	}

		/// float
	int pack ( float *f,            int nitem, int stride = 1 ) {
		return pvm_pkfloat ( f, nitem, stride );
	}

		/// double
	int pack ( double *d,           int nitem, int stride = 1 ) {
		return pvm_pkdouble ( d, nitem, stride );
	}

		/// int
	int pack ( int *i,              int nitem, int stride = 1 ) {
		return pvm_pkint ( i, nitem, stride );
	}

		/// unsigned int
	int pack ( unsigned int *ui,    int nitem, int stride = 1 ) {
		return pvm_pkuint ( ui, nitem, stride );
	}

		/// short
	int pack ( short *sh,           int nitem, int stride = 1 ) {
		return pvm_pkshort ( sh, nitem, stride );
	}

		/// unsigned short
	int pack ( unsigned short *ush, int nitem, int stride = 1 ) {
		return pvm_pkushort ( ush, nitem, stride );
	}

		/// long
	int pack ( long *l,             int nitem, int stride = 1 ) {
		return pvm_pklong ( l, nitem, stride );
	}

		/// unsigned long
	int pack ( unsigned long *ul,   int nitem, int stride = 1 ) {
		return pvm_pkulong ( ul, nitem, stride );
	}

		/// Pack a NULL-terminated string
	int pack ( char *str ) {
		return pvm_pkstr ( str );
	}
	
	
		/// Unpack some bytes
	int unpack ( char *bytes,         int nitem, int stride = 1 ) {
		return pvm_upkbyte ( bytes, nitem, stride );
	}

		/// float
	int unpack ( float *f,            int nitem, int stride = 1 ) {
		return pvm_upkfloat ( f, nitem, stride );
	}

		/// double
	int unpack ( double *d,           int nitem, int stride = 1 ) {
		return pvm_upkdouble ( d, nitem, stride );
	}

		/// int
	int unpack ( int *i,              int nitem, int stride = 1 ) {
		return pvm_upkint ( i, nitem, stride );
	}

		/// unsigned int
	int unpack ( unsigned int *ui,    int nitem, int stride = 1 ) {
		return pvm_upkuint ( ui, nitem, stride );
	}

		/// short
	int unpack ( short *sh,           int nitem, int stride = 1 ) {
		return pvm_upkshort ( sh, nitem, stride );
	}

		/// unsigned short
	int unpack ( unsigned short *ush, int nitem, int stride = 1 ) {
		return pvm_upkushort ( ush, nitem, stride );
	}

		/// long
	int unpack ( long *l,             int nitem, int stride = 1 ) {
		return pvm_upklong ( l, nitem, stride );
	}

		/// unsigned long
	int unpack ( unsigned long *ul,   int nitem, int stride = 1 ) {
		return pvm_upkulong ( ul, nitem, stride );
	}

		/// Unpack a NULL-terminated string
	int unpack ( char *str ) {
		return pvm_upkstr ( str );
	}

		//@}

};

#endif
