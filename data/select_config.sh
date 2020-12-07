#!/bin/bash

if [ -v $1 ]
then
    echo "must be run as 'select <vehicle directory>'"
    exit
fi

ln -sf $1/config config
