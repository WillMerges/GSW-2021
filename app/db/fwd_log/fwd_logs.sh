#!/bin/bash

if ! [ -v GSW_HOME ]
then
    echo environment not set, must run '. setenv' first
    exit
fi

${GSW_HOME}/app/log_view/log_view | python3 send_udp_logs.py
