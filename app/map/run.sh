#!/bin/bash

./CreateLink.py track.kml
./print_gps | ./GPSTrack.py
