/* MWprintf.C - The place for the MWPrintf stuff. */

#include "MW.h"
#include <stdarg.h>
#include <time.h>
#include <errno.h>

int MWprintf_level = 50;
static int fopen_count = 0;

FILE* Open ( char *filename, char *mode )
{
    FILE *fp;

    fp = fopen ( filename, mode );
    //MWprintf ( 10, "Opened a file %s  %x \n", filename, fp);
    if ( fp ) fopen_count++;
    return fp;
}

void Close ( FILE *fp )
{
    int retval;
	fflush(fp);
    retval = fclose ( fp );
    if ( retval != 0 )
    	MWprintf ( 10, "Could not close the file %x as errno is %d\n", fp, errno );
    else
    	fopen_count--;
}

int set_MWprintf_level ( int level ) {
	int foo = MWprintf_level;
	if ( (level<0) || (level>99) ) {
		MWprintf( 10, "Bad arg \"%d\" in set_MWprintf_level().\n", level );
	} else {
		MWprintf_level = level;
	}
	return foo;
}

void MWprintf ( int level, char *fmt, ... ) {

	static int printTime = TRUE;

	if ( level > MWprintf_level ) {
		return;
	}

	if ( printTime ) {
		char *t;
		time_t ct = time(0);
		t = ctime( &ct );
		t[19] = '\0';
		printf( "%s ", &t[11] );
	}

	va_list ap;
	va_start( ap, fmt );
	vprintf( fmt, ap );
	va_end( ap );
	fflush( stdout );

	if ( fmt[strlen(fmt)-1] == '\n' ) 
		printTime = TRUE;
	else
		printTime = FALSE;
}
