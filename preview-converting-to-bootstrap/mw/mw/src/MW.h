#ifndef MW_H
#define MW_H

#include "MWprintf.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>

#ifdef LINUX  /* have to have this to compile on our linux machines. */
#include <std/typeinfo.h>
#endif

/// FALSE is defined as 0
#ifndef FALSE
#define FALSE 0
#endif

/// TRUE is defined as 1
#ifndef TRUE
#define TRUE 1
#endif

/// UNDEFINED is defined as -1
#ifndef UNDEFINED
#define UNDEFINED -1
#endif

/* If you've got a dumb compiler that doesn't understand bool, this
   line is for you:
*/
#if defined( DUMB_COMPILER )
/// If defined ( DUMB_COMPILER ), we typedef bool to int.
typedef int bool;
#endif

/** An upper limit on the number of workers -- so we can allocated arrays
    to keep statistics */
const int MW_MAX_WORKERS = 8192;



/**@name Introduction.

   MW is a class library that can be a useful tool for building
   opportunistic, fault-tolerant applications for high throughput
   computing.  

   In order to build an application, there are three classes
   that the user {\rm must} rederive.

   \begin{itemize}
   \item \Ref{MWDriver}
   \item \Ref{MWWorker}
   \item \Ref{MWTask}
   \end{itemize}

   The documentation of these classes includes a description 
   of the pure virtual methods that must be implemented for 
   a user's particular application.

   Using the MW library allows the user to focus on the application
   specific implementation at hand.  All details related to
   being fault tolerant and opportunistic are implemented in the
   MW library.

   Also included is a small, naive, example of how to create
   an application with the MW class library.  The three classes

   \begin{itemize}
   \item \Ref{Driver_Fib}
   \item \Ref{Worker_Fib}
   \item \Ref{Task_Fib}
   \end{itemize}
   
   are concrete classes derived from MW's abstract classes.  
   Using these classes, a simple program that makes a lot of 
   fibonacci sequences is created.

 */

/**
   A list of the message tags that the Master-Worker application
   will send and receive.  See the switch statement in master_mainloop 
   for more information.
 */

enum MWmessages{
  HOSTADD = 33,
  HOSTDELETE,   
  HOSTSUSPEND, 
  HOSTRESUME, 
  TASKEXIT,   
  DO_THIS_WORK, 
  RESULTS,      
  INIT,         
  INIT_REPLY,   
  BENCH_RESULTS,
  KILL_YOURSELF,
  CHECKSUM_ERROR,
  RE_INIT
};

/** Possible return values from many virtual functions */
enum MWReturn {
		/// Normal return
	OK, 
		/// We want to exit, not an error.
	QUIT, 
		/// We want to exit immediately; a bad error ocurred
	ABORT
};



#endif

