#!/bin/bash

# Run InfluxDB server on remote machine, first argument is hostname
# Second argument is username, or defaults to 'launch'

if ! [ $1 ]
then
    echo "Usage: ./remote_run.sh [hostname] <username>"
    exit
fi

if ! [ $2 ]
then
    USER="launch"
else
    USER=$2
fi

ssh -t $USER@$1 "bash -ci \"gsw; cd ${GSW_HOME}/app/db/run_influx; ./run_influx.sh\""
