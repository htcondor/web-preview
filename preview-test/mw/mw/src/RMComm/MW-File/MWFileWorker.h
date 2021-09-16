#ifndef MWFILEWORKER_H
#define MWFILEWORKER_H

enum FileWorkerState
{ 
    FILE_FREE,
    FILE_SUBMITTED,
    FILE_EXECUTE,	// Started executing for the first time.
    FILE_RUNNING, 
    FILE_KILLED, 
    FILE_SUSPENDED,
    FILE_RESUMED,
    FILE_SHOT,
    FILE_TRANSIT,
};


struct FileWorker
{
    int id;

    // What is the message number that I have to look next for.
    int counter;

    // What is the message that the worker is looking for.
    int worker_counter;

    // What is the condor_id of the worker.
    int condorID;

    // What is the proc id.
    int condorprocID;

    FileWorkerState state;

    int arch;

    int event_no;

    /** What was last served */
    int served;

    int num_starts;
};

#endif MWFILEWORKER_H
