#!/bin/bash

set -x

FORTUNE_DATA_DIR=/usr/share/games/fortune

TEMP_DIR=`mktemp -d` && {
    cd $TEMP_DIR
    /usr/local/bin/retrieve_condor_fortune_txt.py -o condor-tips
    strfile condor-tips condor-tips.dat
    cmp -s condor-tips.dat $FORTUNE_DATA_DIR/condor-tips.dat
    if [ "$?" -ne "0" ]; then
		chmod 644 condor-tips condor-tips.dat
        mv condor-tips condor-tips.dat $FORTUNE_DATA_DIR
    fi
    cd /tmp
    rm -rf $TEMP_DIR
}
