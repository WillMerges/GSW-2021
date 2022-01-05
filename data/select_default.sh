#!/bin/bash

if [ -v $1 ]
then
    echo "must be run as 'select_default <vehicle directory>'"
    exit
fi

ln -sf $1 default
