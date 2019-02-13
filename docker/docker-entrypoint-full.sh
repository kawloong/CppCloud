#!/bin/bash

set -e # o pipefail

ping -c 3 cppcloud_serv
cppcloud-web -s cppcloud_serv:4800

