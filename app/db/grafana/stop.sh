#!/bin/bash

if ! [ -v GSW_HOME ]
then
    echo environment not set, must run '. setenv' first
    exit
fi

# Stop advertising over mDNS
$GSW_HOME/proc/tool/mdns_publish/mdns_clean.sh grafana_pids

# Stop the Grafana server service
sudo service grafana-server stop
