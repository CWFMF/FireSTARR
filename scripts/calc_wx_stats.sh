#!/bin/bash
DIR=`dirname $(realpath "$0")`
NAME=$1
${DIR}/calc_wx.sh ${NAME} | ministat -n | sed "1s/.*/${NAME}/; s/^x /  /g"
