#!/bin/bash

set -x

TEMP_FILE=`mktemp` && {
    echo "----------------------------------------------------------------------" > $TEMP_FILE

    if [ -x /usr/bin/fortune ]; then
        /usr/bin/fortune condor-tips >> $TEMP_FILE
        echo "" >> $TEMP_FILE
    fi

    echo "The full Condor manual may be viewed at:" >> $TEMP_FILE
    echo "http://www.cs.wisc.edu/condor/manual" >> $TEMP_FILE
    echo "----------------------------------------------------------------------" >> $TEMP_FILE

	chmod 644 $TEMP_FILE
    mv $TEMP_FILE /etc/motd
}

