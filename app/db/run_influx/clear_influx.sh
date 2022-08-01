#!/bin/bash
#
# File:     clear_influx.sh
#
# Purpose:  clears all data out of the 'gsw' database in the currently running
#           InfluxDB server
#
# Usage:    ./clear_influx.sh
#

#if [ -v $1 ]
#then
#    echo "must be run as 'clear_influx.sh <device name>'"
#    exit
#fi


#influx -execute "drop measurement $1" -database=gsw

# clears everything
influx -host influx.local -execute "DROP SERIES FROM /.*/" -database=gsw
