
#ifndef MWFILESEND_H
#define MWFILESEND_H

#include "MWFileTypes.h"
#include <stdlib.h>

class FileSendBuffer
{
  public:
	FileType type;
	void *data;
	int size;

	FileSendBuffer ( void *dt, FileType tp, int sz )
	{
	    int siz;
	    if ( tp == STRING ) 
		siz = sizeof(char) * sz;
	    else
		siz = sz;

	    data = (void *) malloc ( siz );
	    memcpy ( data, dt, siz );
	    type = tp;
	    size = sz;
	}

	~FileSendBuffer ( )
	{
	    if ( data != NULL ) free(data);
	}
};
#endif MWFILESEND_H

