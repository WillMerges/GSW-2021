#!/bin/bash

if ! [ -v GSW_HOME ]
then
    echo environment not set, must run '. setenv' first
    exit
fi

# Advertise ourself as 'grafana.local' over mDNS
$GSW_HOME/proc/tool/mdns_publish/mdns_publish grafana.local grafana_pids

# Start the Grafana server service
sudo service grafana-server start
