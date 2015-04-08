#!/bin/bash

# 

export PRECMD='echo p'
SCREEN_OPT=-L make autoretest
PID1=$!

export PRECMD='echo P'
MONITOR_PORT=/dev/ttyUSB1 make autoretest
PID2=$!

echo ${PID1} ${PID2}

sleep 10

pkill ${PID1}
pkill ${PID2}
