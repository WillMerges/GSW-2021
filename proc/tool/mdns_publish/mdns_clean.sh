#!/bin/bash

# Kill any running mDNS publishing processes
# Reads PIDs out of first argument and kills them

if ! [ $1 ]
then
    echo "usage: ./mdns_clean [PID file]"
    return
fi

FILE=`pwd`/$1

if ! [ -f $FILE ]
then
    echo "no such PID file"
else
    while read l; do
        echo "killing PID $l"
        sudo kill $l
        sleep 0.5
    done <$FILE
    rm $FILE
fi

