#!/bin/bash
pico2wave -w="${GSW_HOME}/data/temp.wav" "$1"
aplay ${GSW_HOME}/data/temp.wav
rm ${GSW_HOME}/data/temp.wav
