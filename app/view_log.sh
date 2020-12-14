#!/bin/bash

if ! [ -v GSW_HOME ]
then
    echo environment not set, must run '. setenv' first
fi


### this attaches to the DLP process and looks for the write syscall
#PIDLIST=${GSW_HOME}/startup/current/pidlist

#if ! [ -f $PIDLIST ]
#then
#    echo "GSW is not running, no pidlist file found"
#    exit
#fi

#DLP_PID=`tail -n 1 $PIDLIST`

#sudo strace -f -p${DLP_PID} -s9999 -e write
### we'll just read the file instead ###

### PROBLEM ###
# this won't stop reading from system.log, and won't read system.log1....
# potential solution: have dlp write to a FIFO called system.fifo and have tail read that
tail -f ${GSW_HOME}/log/system.fifo
