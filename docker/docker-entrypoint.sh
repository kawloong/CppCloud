#!/bin/bash

set -e # o pipefail


case $1 in 
    sample_conf|sample_prvd|sample_tcp_invk|sample_http_invk|agent_prvd)
        param1=${2-localhost:4800}
        echo "run command: $1" $param1
        exec "$1" $param1
    ;;

    cppcloud_serv|server|serv)
        params="${@:2:999}"
        echo "Start to run ./cppcloud_serv $params"
        cppcloud_serv "$params"
    ;;

    sh|bash)
        echo 'run bash at boot'
        /bin/bash
    ;;

    log)
        params="${@:2:999}"
        tail /cppcloud/docker/logs/cppcloud_serv.log $params
    ;;

    *)
        if [ "${1:0:1}" = '-' ]; then
            echo "Start to run ./cppcloud_serv $@"
            cppcloud_serv "$@"
        else
            cat help.txt
        fi
    ;;
esac