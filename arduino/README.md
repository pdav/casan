SOS slave for Arduino
=====================

Introduction
------------

This is the source code for the slave processor running on
Arduino/Zigduino boards.


Prerequisites
-------------

The SOS slave program needs:
- the Arduino IDE (not used at all, but installing it will bring all
    required dependancies for compilation of Arduino sketches)
- the `screen` program (in order to access serial ports)


Zigduino installation
---------------------

If you intend to use IEEE 802.15.4 with the Zigduino board, you
need to download the proper Zigduino firmware from
https://github.com/logos-electromechanical/Zigduino-1.0

To install it, do the following steps:

    # ZDIR=..../Zigduino-1.0/hardware/arduino
    # ADIR=/usr/share/arduino/hardware/arduino		# standard Arduino IDE
    # ln -s $ZDIR/cores/zigduino $ADIR/cores
    # ln -s $ZDIR/variants/zigduino_r2 $ADIR/variants
    # mv $ADIR/boards.txt $ADIR/boards.txt.old
    # ln -s $ZDIR/boards.txt $ADIR


Test sketches and examples
--------------------------

The `tests` subdirectory contains some individual tests sketches
for various C++ classes of the SOS slave. Moreover, the `tests/sos`
subdirectory contains a ready-to-use example applcation.

To run these tests:

    $ cd test/sos/
    $ make && make upload			# or make test

There is a Makefile in every test sketch, change the device you have to use 
to upload your sketch on your platform (see ../README.md for a note on
hardwiring serial devices on Linux).


Documentation
-------------

Program documentation is generated with Doxygen. To generate it, you need
`doxygen`, `dot` (part of graphviz) and `pdflatex`.

Just type `make pdf` to generate the `doc/latex/refman.pdf` file.
the library documentation


Notice
------

The mk/ subdirectory is a complete Makefile for Arduino sketches,
thanks to Sudar
It is extracted from github:
    git clone git@github.com:sudar/Arduino-Makefile.git
