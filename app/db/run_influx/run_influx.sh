#!/bin/bash

if ! [ -v GSW_HOME ]
then
    echo environment not set, must run '. setenv' first
    exit
fi

cd ${GSW_HOME}/app/db/run_influx

# Advertise ourself as 'influx.local' over mDNS
echo "advertising mdns"
${GSW_HOME}/proc/tool/mdns_publish/mdns_publish influx.local influx_pids

# catch signals
trap "echo "trap"; pkill -9 $INFLUX_PID; rm influx_pids" SIGINT  # catch ctrl + c
trap "echo "trap"; pkill -9 $INFLUX_PID; rm influx_pids" SIGTERM # catch default 'kill' signal

# if we error out early, want to stop publishing
# trap "echo "trap"; $GSW_HOME/proc/tool/mdns_publish/mdns_clean.sh influx_pids" EXIT

sudo influxd -config influxdb.conf
INFLUX_PID=$!
