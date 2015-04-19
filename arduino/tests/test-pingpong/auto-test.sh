#!/bin/bash

# listener
function write_file {
    cat pp.tmpl | sed "s/CARACTERE/$CARACTERE/" | sed "s/MYCHANNEL/$c/" \
        > test-zig.ino
}

function screen_listener {
    MONITOR_PORT=/dev/ttyUSB1 make test
}

function screen_prod {
    SCREEN_OPT=-L make test
}

function kill_screens {
    killall -9 screen
}

mkdir logs

for c in `seq 11 25`
do

    export CARACTERE=P
    write_file
    screen_listener

    export CARACTERE=p
    write_file
    screen_prod

    sleep $((15*60)) # 15 minutes
    kill_screens
    mv screenlog.0 logs/canal_$c
done
