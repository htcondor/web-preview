#ifndef MWFILERCEVENTS_H
#define MWFILERCEVENTS_H

enum FileRCEvents 
{
	FileHostDelete,		/* Not yet implemented */
	FileHostAdd,		/* Host gets added */
	FileTaskExit,		/* Task has exited */
	FileTaskSuspend,	/* Task has been suspended */
	FileTaskResume,		/* Task resumed */
	FileChecksumError,	/* Checksum error */
	FileNumEvents		/* Number of events */
};

class FileRCEvent
{
  public:
	/* What message has to be sent on the 
	 * receipt of the event
	 */
	int 		tag[FileNumEvents];
};

#endif MWFILERCEVENTS_H
