#!/bin/bash

if ! [ -v $1 ]
then
    rm -f current
    ln -s $1 current
else
    echo "run as: select_current.sh [script directory]"
fi
