#!/bin/bash

echo "arr is $# $*"

case $1 in 

esac

newArr=("${@:2:999}")
echo "newArr is " ${newArr[*]}
