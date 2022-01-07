#!/bin/bash

set +e

if [ -v $1 ]
then
    echo "must be run as 'select_default <vehicle directory>'"
    exit
fi

rm default > /dev/null 2> /dev/null
ln -sf $1 default
