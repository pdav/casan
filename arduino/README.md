## intro
This is the source code for the slave processor running on
Arduino boards.

## install

$ cd /usr/share/arduino/libraries/
$ ln -s /your/sos/repository/arduino/libraries/sos/ sos

## usage

% cd /your/sos/repository/arduino/test/sos/
% make && make upload
Or
% make test

There is a Makefile in every test sketch, change the device you have to use 
to upload your sketch on the arduino (generally /dev/ttyACM0).

## the library documentation

See libraries/sos/doc/.
