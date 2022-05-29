#!/bin/bash

# Creates KML network link and pipes GPS/altitude measurements to a KML track generator

if ! [ -v GSW_HOME ]
then
    echo environment not set, must run '. setenv' first
    exit
fi

./CreateLink.py track.kml
${GSW_HOME}/app/print_meas/print_meas GPS_LAT GPS_LONG ALT | ./GPSTrack.py
