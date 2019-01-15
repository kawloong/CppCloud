#!/bin/bash

cmd=`basename $0`
echo "../cpp_sdk/$cmd" ${1-localhost:4800} ${@:2:999}
exec ../cpp_sdk/$cmd ${1-localhost:4800} ${@:2:999}

