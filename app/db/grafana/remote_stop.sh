#!/bin/bash

# Stop Grafana server at hostname 'grafana.local' remotely 
# First argument is username, or defaults to 'launch'

if ! [ $1 ]
then
    USER="launch"
else
    USER=$1
fi

ssh $USER@grafana.local "gsw; app/db/grafana/stop.sh"
