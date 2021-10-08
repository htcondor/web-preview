#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/param.h>
#include <grp.h>


#define NOBODY 99
#define MAGIC_PASSWORD_FILE "/scratch/zmiller/ftwrap/magic_pass"
#define IRODS_PLUGIN_SCRIPT "/scratch/zmiller/ftwrap/irods_plugin.sh"
#define ENV_NAME "IRODS_PASS"


int
main(int argc, char** argv) {

    char debug[256];
    sprintf(debug, "/tmp/zkm-d.%i", getpid());
    FILE *DEBUG = fopen(debug, "a");
    if (DEBUG == NULL) {
        exit(97);
    }

    // hack for now... everyone (root/user/nobody) needs to write to this file
    fchmod(fileno(DEBUG), 0666);

    fprintf(DEBUG, "\nINVOCATION at %i\n", time(0));

    if(strcmp(argv[1], "-classad") == 0) {

        // pass through... no permission magic needed here
        fprintf(DEBUG, "invoking %s -classad\n", IRODS_PLUGIN_SCRIPT);

    } else {

	// get a permission status report
        uid_t ruid = -1;
        uid_t euid = -1;
        uid_t suid = -1;
	getresuid(&ruid,&euid,&suid);
        fprintf(DEBUG, "ruid,euid,suid was %i,%i,%i\n", ruid,euid,suid);
	uid_t save_uid = euid;

        // become root and prove it
	seteuid(0);
	getresuid(&ruid,&euid,&suid);
        fprintf(DEBUG, "ruid,euid,suid is  %i,%i,%i\n", ruid,euid,suid);

        // read magic password (should be in a file readable only by root)
        char magic_password[256];
        FILE *passfile = fopen(MAGIC_PASSWORD_FILE, "r");
        if (passfile) {
            fgets(magic_password, 256, passfile);
            fclose(passfile);
            fprintf(DEBUG, "pass is %s", magic_password);
        } else {
            fprintf(DEBUG, "unable to open %s\n", MAGIC_PASSWORD_FILE);
            fclose(DEBUG);
            exit(98);
        }

	// put the password in the environment for the process we're about to exec.
        setenv(ENV_NAME, magic_password, 1);

	// drop all privs and supplemental groups
	gid_t egid = getegid();
	seteuid( 0 );
	setgroups( 1, &egid );
	setgid( egid );
	setreuid( NOBODY,save_uid );

	// get a final permission status report
	getresuid(&ruid,&euid,&suid);
        fprintf(DEBUG, "ruid,euid,suid now %i,%i,%i\n", ruid,euid,suid);
        fprintf(DEBUG, "invoking %s %s %s\n", IRODS_PLUGIN_SCRIPT, argv[1], argv[2]);
    }

    // if we don't close, debug output is probably lost
    fclose(DEBUG);

    // run the actual plugin, as euid of the user and ruid nobody
    execv(IRODS_PLUGIN_SCRIPT, argv);

    // FAILED!  reopen debug log and report.
    DEBUG = fopen(debug, "a");
    fprintf(DEBUG, "exec failed! errno %n\n", errno);
    fclose(DEBUG);
 
    exit(99);
}

