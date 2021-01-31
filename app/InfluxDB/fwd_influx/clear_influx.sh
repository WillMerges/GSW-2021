#!/bin/bash

if [ -v $1 ]
then
    echo "must be run as 'clear_influx.sh <device name>'"
    exit
fi


influx -execute "drop measurement $1" -database=gsw
