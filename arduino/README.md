SOS slave for Arduino
=====================

Introduction
------------

This is the source code for the slave processor running on
Arduino boards.

Installation
------------

$ cd /usr/share/arduino/libraries/
$ ln -s /your/sos/repository/arduino/libraries/sos/ sos

Usage
-----

% cd /your/sos/repository/arduino/test/sos/
% make && make upload
Or
% make test

There is a Makefile in every test sketch, change the device you have to use 
to upload your sketch on the arduino (generally /dev/ttyACM0).

Documentation
-------------

the library documentation

See libraries/sos/doc/.


Notice
------

The mk/ subdirectory is a complete Makefile for Arduino sketches,
thanks to Sudar
It is extracted from github:
    git clone git@github.com:sudar/Arduino-Makefile.git
