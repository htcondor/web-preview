/*
ibp_probe by Douglas Thain

This program fetches out status information from
an IBP depot and displays it as a ClassAd suitable
for consumption by catalog_update.

This program is placed in the public domain.
*/

#include "ibp_client.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static char *server_name = "localhost";
static int server_port = 6714;
static int timeout = 60;

static void show_version( char *cmd )
{
	fprintf(stderr,"%s %s %s\n",cmd,__DATE__,__TIME__);
}

static void show_use( char *cmd )
{
	fprintf(stderr,"Use: %s [options]\n",cmd);
	fprintf(stderr,"Where options are:\n");
	fprintf(stderr,"   -s <host>  Server to query (default is %s)\n",server_name);
	fprintf(stderr,"   -p <port>  Server's port number (default is %d)\n",server_port);
	fprintf(stderr,"   -t <secs>  Timeout, in seconds (default is %d)\n",timeout);
	fprintf(stderr,"   -v         Show version string\n");
	fprintf(stderr,"   -h         Show this help screen\n");
}

int main( int argc, char *argv[] )
{
	IBP_DptInfo info;
	struct ibp_timer timer;
	struct ibp_depot depot;
	char ch;

	while((ch=getopt(argc,argv,"s:p:t:vh"))!=(char)-1) {
		switch(ch) {
			case 's':
				server_name = optarg;
				break;
			case 'p':
				server_port = atoi(optarg);
				break;
			case 't':
				timeout = atoi(optarg);
				break;
			case 'v':
				show_version(argv[0]);
				return 1;
				break;
			default:
			case 'h':
				show_use(argv[0]);
				return 1;
				break;
		}
	}

	strcpy(depot.host,server_name);
	depot.port = server_port;

	timer.ClientTimeout = timeout;
	timer.ServerSync = 1;

	info = IBP_status(&depot,IBP_ST_INQ,&timer,0,0,0,0);
	if(!info) {
		fprintf(stderr,"Unable to fetch status from host %s port %d\n",server_name,server_port);
		return 1;
	}

	printf("[\nName = \"%s\";\nType = \"IBP\"; Port = %d;\nStableStorage = %lu;\nStableStorageUsed = %lu;\nVolatileStorage = %lu;\nVolatileStorageUsed = %lu;\nDuration = %d;\n]\n",
		depot.host,
		depot.port,
		info->StableStor,
		info->StableStorUsed,
		info->VolStor,
		info->VolStorUsed,
		info->Duration);

	return 0;
}


