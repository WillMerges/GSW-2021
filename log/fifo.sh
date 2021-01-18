#!/bin/bash

if ! [ -v GSW_HOME ]
then
    echo environment not set, must run '. setenv' first
fi

if [ -v $1 ]
then
    echo "must run as fifo.sh [ -make | -delete ]"
fi


if [[ $1 == "-make" ]]
then
    mkfifo ${GSW_HOME}/log/system.fifo
fi

if [[ $1 == "-delete" ]]
then
    rm -f ${GSW_HOME}/log/system.fifo
fi
