#ifndef MWRMCOMM_H
#define MWRMCOMM_H
#include <MWWorkerID.h>

/** This class is an abstract layer over a Resource Management (RM) and
	Communication (Comm) layer.  Hence the RMComm name.  It is designed
	to abstract away the differences between various systems.  

	The member functions in this class fall into two categories:  
	Resource Management and Message Passing (Communication).  The "master"
	in a master-worker application will use both; the worker will only
	use the communication facilities.
*/

enum MWRMCommErrors {
  WORKER_TIMEOUT = -50,
  UNABLE_TO_WAKE = -49,
  CANNOT_OPEN_INPUT_FILE = -48,
  SCANF_ERROR_ON_INPUT_FILE = -47,
  MESSAGE_SEQUENCE_ERROR = -46,
  UNKNOWN_DATA_TYPE = -45,
  WAITFILE_PROTOCOL_ERROR = -44,
  FAIL_MASTER_SEND = -43,
  CHECKSUM_ERROR_EXIT = -42,
  UNKNOWN_COMMAND = -41,
  INIT_REPLY_FAILURE = -40,
  UNPACK_FAILURE = -39
};

class MWRMComm {

	friend class MWDriver;

 public:

  /** Constructor.  Sets data to -1's. */
  MWRMComm();

  /// Destructor...
  virtual ~MWRMComm();

		/** @name A. Resource Management Functions

			Here are all the methods you could want to have for managing 
			a set of machines.  See each method for details...
		*/
		//@{

		/** System shutdown.  Does not return.  
		    @param exitval The value to call ::exit() with 
		*/
	virtual void exit( int exitval );

		/** Initialization of the master process.  This will be called
			one time only when the master starts up.  
			@return 0 on success, -1 on failure
		 */
	virtual int setup( int argc, char* argv[], int *my_id, int *master_id ) = 0;

		/** This returns the state of the "virtual machine" - aka the 
			set of workers under the master's control.  
			@param nhosts The number of hosts
			@param narches The number of architechture classes
			@param w A pointer to a pointer to an array of pointers.  
			  This should be NULL when called; config() will allocate
			  the memory using new and return it to you.  Don't forget
			  to delete not only the elements in that array, but 
			  also the array itself.  The array will have nhosts
			  elements, and they will each represent a worker 
			  machine.
			 @return 0 on success, -1 on error.
		*/
	virtual int config( int *nhosts, int *narches, MWWorkerID ***w ) { return 0; };
	
		/** Start a worker on a machine that has been given to you.
			This is really only important if the process of starting
			a worker is two-stage.  For instance, in pvm, you
			first get a machine.  THEN you have to spawn an 
			executable on that machine.  For something like MW-files, 
			once you get a worker it already has the executable 
			started on it.  In that case this will basically be
			a no-op.  
			@param w A pointer to a MWWorkerID.  This must point
			 to allocated memory.  It is filled with info like the
			 machine name, id2, and the arch.
			@return id2 on success, -1 on failure
		*/
	virtual int start_worker( MWWorkerID *w ) = 0;

		/** This routine is used to start up multiple workers at 
			the beginning of a run.  It should only be called one
			time.  It basically does a config() to find out what
			machines are available, and then starts a worker on 
			each of them.  You may want to check the implementations
			for details...
			@param nworkers The number of workers at the start (returned!)
			@param workers A pointer to a pointer to an array of
			 MWWorkerID pointers.  The memory management for this 
			 is the same as it is for the config() call - it should
			 be null when called and will point to allocated memory
			 when it comes back.
			@return 0 on success, -1 on failure
		*/
	virtual int init_beginning_workers( int *nworkers, MWWorkerID ***workers ) = 0;

		/** Remove a worker from the virtual machine.  This call 
			will delete w, so don't reference that memory again!
			@param w The MWWorkerID of the machine to remove.
			@return 0 on success, a negative number on failure
		 */
	virtual int removeWorker( MWWorkerID *w ) = 0;
		//@}

		/** @name B. Checkpointing Functions */
		//@{
		/** Write out internal state to a FILE *
		 */
	virtual int write_checkpoint( FILE * fp );

		/** Read in restart information from a FILE * 
		 */
	virtual int read_checkpoint( FILE * fp );

		/** Some Low level specific read/write functions */
	virtual int read_RMstate( FILE *fp ); 
  ///
	virtual int write_RMstate ( FILE *fp );
		//@}

		/** @name C. Other public functions */
		//@{
		/** Set the name of the binary and the requirements string 
			for an arch class.  Technically, the requirements string 
			is not needed for MWPvmRC - it is defined in the submit
			file.  It *is* needed in the MWFileRC, however, for job
			submission.
			@param arch_class This specifies which arch class the above
			    requirements will apply to.
			@param exec_name The name of the worker executable for 
			    this arch class.
			@param requirements A string containing the "requirements" 
			    attribute of a given arch class of workers.  This will 
				be something that can be used in a submit file, like 
				"(arch == \"INTEL\" && opsys == \"SOLARIS26\")"
			 */
	void set_worker_attributes ( int arch_class, char *exec_name, 
				     char *requirements ); 

  /** If the RM software needs to "process" the executable name in
      some way, this is done here.
  */
	virtual void processExec ( char*, char* );

		/** Return the number of arch classes */
	int get_num_arches();

		/** Set the number of arch classes */
	void set_num_arches( int n );

  /** Set whether or not you would like worker checkpointing 
      (if the CommRM implementation has the capability)
  */

  virtual void set_worker_checkpointing( bool wc );


		//@}
 protected: 
	
		/** @name D. Host Management Members */
		//@{
		/** This will figure out if we need to ask for more hosts
			or remove hosts.  It is called whenever a host is added
			or removed from the system, or set_target_num_workers()
			is called.
			@param num_workers A pointer to an array of length
			 num_arches that contains the number of workers for 
			 each arch class.
			@return If we have more workers than we need, we return a
			 positive number as the "excess" that can be deleted.  
		*/
	virtual int hostaddlogic( int *num_workers ) = 0;

		/** Set a "target" number of workers across all arches.  This 
			is useful if you don't care how many you get of each arch... 
			@param num_workers The target number of workers 
		*/
	void set_target_num_workers( int num_workers ) {
		set_target_num_workers( num_workers, -1 );
	}

	void set_target_num_workers( int num_workers, int arch_class ) {
	        /* XXX implement arch_class!!!! */
	        target_num_workers = num_workers;
	}



		//@}

		/** @name E. Some Protected Data */
		//@{
		/** An array containing the {\bf full} pathnames of the executables.
			Element 0 is for arch "0", element 1 is for arch "1", etc.
			Usually read in get get_userinfo().  The array is fixed at 
			16 arches - that ought to be enough!
		*/
	char worker_executables[16][_POSIX_PATH_MAX];

		/** An array containing the requirements string for each arch 
			class.  This is needed in MW-files. */
	char worker_requirements[16][1024];

		/** The desired number of workers */
	int target_num_workers;

		/** The number of different arch classes. */
	int num_arches;

  /// Would you like the workers to be checkpointed
  bool worker_checkpointing;
		//@}
  
 public:

		/** @name The Communication Routines

			These message passing routines are very closely modeled on
			PVM's message passing facility.  They are, however, pretty
			generic in that any message passing interface should be 
			able to implement them.
		*/
	
		//@{

		/** Initialize a buffer for sending some data.
			@param encoding Defined by each application.  0 = default */
	virtual int initsend ( int encoding = 0 ) = 0;
		/** Send the data that has been packed. 
			@param to_whom An identifier for the recipient
			@param msgtag A 'tag' to identify that type of message */
	virtual int send ( int to_whom, int msgtag ) = 0;

		/** Receive some data that has been packed.  Should make this 
			more PVM-independent; will do this sometime.
			@param from_whom From a specific source; -1 is from all
			@param msgtag With a certain tag; -1 is all. */
	virtual int recv ( int from_whom, int msgtag ) = 0;

		/** Provide info on the message just received */
	virtual int bufinfo ( int buf_id, int *len, int *tag, int *from ) = 0;

		/** Needed only for MW-Independent */
	virtual void hostadd ( ) { };

		/** @name Pack Functions
			
			In the following pack() functions, there are some common themes.
			First, each stuffs some data into a buffer to be sent.  The
			nitem parameter is just a count of the number of items.  The 
			stride parameter specifies *which* items to pack.  1 implies
			all, 2 would be every 2nd item, 3 is every 3rd item, etc. 

			The return value is user defined.  It should be standardized, 
			but I'll do that later.
		*/
		//@{

		/// Pack some bytes
	virtual int pack ( char *bytes,         int nitem, int stride = 1 ) = 0;
		/// float
	virtual int pack ( float *f,            int nitem, int stride = 1 ) = 0;
		/// double
	virtual int pack ( double *d,           int nitem, int stride = 1 ) = 0;
		/// int
	virtual int pack ( int *i,              int nitem, int stride = 1 ) = 0;
		/// unsigned int
	virtual int pack ( unsigned int *ui,    int nitem, int stride = 1 ) = 0;
		/// short
	virtual int pack ( short *sh,           int nitem, int stride = 1 ) = 0;
		/// unsigned short
	virtual int pack ( unsigned short *ush, int nitem, int stride = 1 ) = 0;
		/// long
	virtual int pack ( long *l,             int nitem, int stride = 1 ) = 0;
		/// unsigned long
	virtual int pack ( unsigned long *ul,   int nitem, int stride = 1 ) = 0;
		/// Pack a NULL-terminated string
	virtual int pack ( char *string ) = 0;

		//@}

		/** @name Unpack Functions
			
			These unpack functions unpack data packed with the pack() 
			functions.  See the pack() functions for more details.

		*/
		//@{

		/// Unpack some bytes
	virtual int unpack ( char *bytes,         int nitem, int stride = 1 ) = 0;
		/// float
	virtual int unpack ( float *f,            int nitem, int stride = 1 ) = 0;
		/// double
	virtual int unpack ( double *d,           int nitem, int stride = 1 ) = 0;
		/// int
	virtual int unpack ( int *i,              int nitem, int stride = 1 ) = 0;
		/// unsigned int
	virtual int unpack ( unsigned int *ui,    int nitem, int stride = 1 ) = 0;
		/// short
	virtual int unpack ( short *sh,           int nitem, int stride = 1 ) = 0;
		/// unsigned short
	virtual int unpack ( unsigned short *ush, int nitem, int stride = 1 ) = 0;
		/// long
	virtual int unpack ( long *l,             int nitem, int stride = 1 ) = 0;
		/// unsigned long
	virtual int unpack ( unsigned long *ul,   int nitem, int stride = 1 ) = 0;
		/// Unpack a NULL-terminated string
	virtual int unpack ( char *string ) = 0;

		//@}
		//@}

};

#endif
