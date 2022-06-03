#!/bin/bash

./print_meas $1 -g | feedgnuplot --domain --stream trigger --xlen 10000 --lines --points --terminal 'dumb 200,55' --xlabel milliseconds --exit --ymin $2 --ymax $3
