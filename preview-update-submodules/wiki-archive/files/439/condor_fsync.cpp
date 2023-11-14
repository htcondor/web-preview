#include "condor_common.h"
#include "condor_fsync.h"

bool fsync_on = true;

int condor_fsync(int fd, const char* path)
{
	if(!fsync_on)
		return 0;

	return fsync(fd);
}