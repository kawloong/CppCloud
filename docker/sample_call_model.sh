#!/bin/bash

cmd=`basename $0`
echo "../client_c/$cmd" ${1-localhost:4800} ${@:2:999}
exec ../client_c/$cmd ${1-localhost:4800} ${@:2:999}

