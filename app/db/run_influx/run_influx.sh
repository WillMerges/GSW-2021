#!/bin/bash

if ! [ -v GSW_HOME ]
then
    echo environment not set, must run '. setenv' first
fi

# Advertise ourself as 'influx.local' over mDNS
$GSW_HOME/proc/tool/mdns_publish/mdns_publish influx.local influx_pids

# don't need to trap signals since parent process of mdns_publish killed on ctrl+c
#trap "$GSW_HOME/proc/tool/mdns_publish/mdns_clean.sh influx_pids" SIGINT
trap "rm influx_pids" SIGINT

sudo influxd -config influxdb.conf
