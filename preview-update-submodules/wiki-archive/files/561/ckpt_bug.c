#include <stdio.h>
#include <errno.h>

void ckpt_and_exit( void );

int main( int argc, char **argv )
{
	printf( "Starting up, my pid is %d\n",getpid() );

	if ( argc > 1 && argv[1][0] == 'Y' ) {
		printf("Doing getpwnam()\n");
		getpwnam("foobar");
	} else {
		printf("Skipping getpwnam()\n");
	}

	printf( "Checkpoint and exit...\n" );
	ckpt_and_exit();

	printf( "Returned from checkpoint and exit\n");

	printf( "Exiting\n" );
	return 0;
}
