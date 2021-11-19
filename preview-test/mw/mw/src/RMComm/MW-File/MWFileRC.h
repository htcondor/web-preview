
#ifndef MWFILERC_H
#define MWFILERC_H

#include "../MWRMComm.h"
#include "MWFileRCEvents.h"
#include "MWFileWorker.h"
#include "list.h"
#include <stdio.h>

/** 
	A Resource Management and Communication class that uses Condor 
	for underlying support of resource managament. Some crude inter-process 
	communication is provided using the userlog feature of Condor.
	resource management.

*/

class MWFileRC : public MWRMComm
{

  public:

	///  Constructor 
	MWFileRC( bool val, int id )
	{
		isMaster = val;
		FileRCID = id;
		current_num_workers = 0;

		workerEvents = NULL;
		sendList = NULL;
		recvList = NULL;
	};

	///  Destructor 
	~MWFileRC() {};

 /** @name A. Resource Management Routines

	 Here we implement the pure virtual functions found in 
	 ur parent class, MWRMComm.
 */
 //@{

	/**  Initialises. Depending on whether it is master or worker instance
	     it initializes all the internal variables. */
	int setup ( int argc, char *argv[], int *mytid, int *mastertid );

	/**  Shutdown. Kills all the workers if it is master */
	void exit ( int exitval );

	/**  Initialize workers if already some have started up */
	int init_beginning_workers ( int *nworkers, MWWorkerID ***workers );

	/**  This function is actually a misonomer. It DOES NOT spawn a new
	     worker. Rather it just inits the structure that is passed on to 
	     it */
	int start_worker ( MWWorkerID *w );

	/**  This function removes a existing worker */
	int removeWorker ( MWWorkerID *w );

	/**  Figure out whether or not to generate a new worker depending on
	     whether new requests have been made */
	int hostaddlogic ( int *w );

	/**  A dummy function. For the sake of conformity with the Pvm */
	int config ( int *, int *, MWWorkerID *** );

	/** A routine for reading in the MW-File state at the time 
	 *  of checkpointing.
	 */
	int read_RMstate ( FILE *fp = NULL );

	/** A routine for writing in the MW-File state at the time 
	 *  of checkpointing.
	 */
	int write_RMstate ( FILE *fp = NULL );

	void processExec ( char *execut, char *require );

	void worker_removed ( MWWorkerID *w );

		//@}

	/** @name B. Communication Routines

		Unlike MWPvmRC, the communication routines are non-trivial 
		because Condor provides no inter-process comminucation.
		Thus we use files for communication. So a send is essentially
		a file write operation and a recv is a file-read operation.
		We maintain 2 lists:- The sendList and recvList for taking 
		care of what is to be written/read to/from the files. As in
		pvm a user beings by calling initsend which creates a new list.
		Calls to pack insert into the list what is being packed. And
		finally a send writes the entire thing into a file 
		identified by the destination. Corresponding things happpen
		in recv.
	*/
	//@{

	/**  Initialize the send buffer */
	int initsend ( int useless = 0 );

	/**  Send function */
	int send ( int toWhom, int msgtag );

	/**  Recv function */
	int  recv ( int fromWhom, int msgtag );

	/**  Get some info about the recv buffer */
	int bufinfo ( int buf_id, int *len, int *tag, int *sending_host );

	/// pack some bytes
	int pack ( char *bytes, int nitem, int stride = 1 );

	/// float
	int pack ( float *f, int nitem, int stride = 1 );

	/// double
	int pack ( double *d, int nitem, int stride = 1 );

	/// int
	int pack ( int *i, int nitem, int stride = 1 );

	/// unsigned int
	int pack ( unsigned int *ui, int nitem, int stride = 1 );

	/// short
	int pack ( short *sh, int nitem, int stride = 1 );

	/// unsigned short
	int pack ( unsigned short *ush, int nitem, int stride = 1 );

	/// long
	int pack ( long *l, int nitem, int stride = 1 );

	/// unsigned long
	int pack ( unsigned long *ul, int nitem, int stride = 1 );

	/// string
	int pack ( char  *str );

	/// Unpack some bytes
	int unpack ( char *bytes, int nitem, int stride = 1 );

	/// float
	int unpack ( float *f, int nitem, int stride = 1 );

	///double
	int unpack ( double *d, int nitem, int stride = 1 );

	/// int
	int unpack ( int *i, int nitem, int stride = 1 );

	/// unsigned int
	int unpack ( unsigned int *ui, int nitem, int stride = 1 );

	/// short
	int unpack ( short *sh, int nitem, int stride = 1 );

	/// unsigned short
	int unpack ( unsigned short *ush, int nitem, int stride = 1 );

	/// long
	int unpack ( long *l, int nitem, int stride = 1 );

	/// unsigned long
	int unpack ( unsigned long *ul, int nitem, int stride = 1 );

	/// string
	int unpack ( char  *str );

		//@}
  private:

	/**	Some private functions */
	/**	Handle a message from the worker */
	int handle_finished_worker ( int i );

	/**	Is Called when a host is resumed */
	int handle_resumed_worker ( int i );

	/**	Is Called when a host is suspended */
	int handle_suspended_worker ( int i );

	/**	Is Called when a task is dead */
	int handle_killed_worker ( int i );

	/** 	Called when the worker starts executing first */
	int handle_executing_worker( int i );

	/**	Called in no_checkpoint mode */
	int handle_transited_worker( int i );

	/**	Is called when the worker detects that the master has 
	 * come up
	 */
	int handle_master_executing ();

	/**	Is Called when a message is received by the worker. */
	int handle_work ( int msgtag );

	/**	Polls the log files to see some changes */
	/**	Always get the last result */
	void CheckLogFilesRunning ( );

	/**	Is Called when we are recovering from a crash */
	void CheckLogFilesResuscicate ( );

	/**	The main resuscicate function */
	void resuscicate ( );

	/** 	The helping routine for resuscicate */
	int GetWorkerCounter ( char *file );

	/**	Another helping routine */
	int GetCounter ( char *file1, char *file2 );

	/**	Another helping routine */
	int GetMasterExpectedNumber ( char *file );

	/**	Find the condor_ID of the worker with this log file */
	void GetCondorId ( char *lgfile, int *cId, int *pId );

	/**	Create a worker */
	int do_spawn ( int numworkers, int arch );

	/**	Kill a worker */
	void killWorker ( int i );

	/**	Notify various events */
	int setup_notifies ( int worker_id );

	/**	Master receive */
	int master_recv ( int fromWhom, int msgtag );

	/**	Worker receive */
	int worker_recv ( int fromWhom, int msgtag );

	/**	Determine whether worker i has sent a message */
	bool IsComplete ( int i );

	/**	Get a architecture number */
	int ChoseArchNum ();

	/**  Information by the upper layer */
	void inform_target_num_workers ( );

	/**  A function that inits some internal structures */
	void InitStructures ( );

	void WaitOnExistFile( );


	/**	The bool indicates the mode of the RC instance 
	 *  a true value means that it is a master and a false means that 
	 * it is a worker.
	 *  We need to make it a tristate as somtimes an RC can be both
	 *  a master and a worker.
	 */
	bool isMaster;

	/**	The value is the id that the worker gets. For master it is
	 * of no use.
	 */
	int FileRCID;

	/**	The expected number of the next message */
	int expected_number;

	/**	The expected_number of the next message that the master is
	 * expecting 
	 */
	int master_expected_number;

	/**	The actual task that is being executed */
	int actual_task;

	/**	The directory where all the workers have to send their output */
	char output_directory[256];

	/**	The directory where the master has to send the work */
	char input_directory[256];

	/**	The control directory the master reads for all resource 
	 *   management functions.
	 */
	char control_directory[256];

	/**	The file in the control directory that will contain the momentary
	 *  number of workers. 
	 */
	char moment_worker_file[256];

	/**	The init file */
	char init_file[256];

	/**  	The number of workers that were present. This is an internal
	 *   variable that will be used for copying the entire thing when 
	 *   the target number changes. This is the number that will be maintained close to 
	 *   target_num_workers.
	 */
	int current_num_workers;

	/**  The number of submitted workers */
	int submitted_num_workers;

	/**  An integer corresponding to the number of fileWorkers struct
	 *   that we had/have as the maximum;
	 */
	int max_num_workers;

	/**  An array of message tags to be sent to the upper layers
	 *   as to what tag has to be sent with the event associated with this 
	 *   worker
	 */
	FileRCEvent *workerEvents;

	/**  An array of the number of workers */
	struct FileWorker *fileWorkers;

	/**  A list of all the items that are sent in a send after a series of 
	 *   packs.
	 */
	List *sendList;

	/**  A list of all the items that are received */
	List *recvList;

	/**  This is a variable that keeps the cycle in effect. The receive
	 * of ours goes in cycles to ensure fairness to the messages from all
	 * the slaves.
	 */
	int cyclePosition;

	/**  This determines with what frequency should we check the log files
	 */
	int CHECKLOG_FREQ;

	/**  This keeps the track of how many cycles were made.
	 */
	int turnNo;

	/**  The tag of the message that just came in	*/
	int msgTag;

	/**  The message came from whom 	*/
	int whomRecv;

	int num_sync;

	int SIMUL_SEND;

	/**	Internal variable that is init to what the worker has to 
	 * send to the upper layer in case of a master wakeup
	 */
	int MasterUp;

	/**  A variable array helping in choosing the next arch to chose from */
	int *workerArch;

	/**  A variable to keep track of submit files */
	int subId;

	/**  A variable array keeping track of how many have been requested */
	int *hostadd_reqs;

	/**  A variable to keep track of which of each have been requested */
	int **hostaddind_reqs;

	/**  The checksum calculated */
	long long checksum;

	/**  The worker timeout in minutes */
	long worker_timeout;

};
#endif MWFILERC_H

