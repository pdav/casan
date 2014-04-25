Arduino SOS libraries
=====================

Introduction
------------

This directory contains various libraries needed for programming
SOS applications on Arduino:

* Sos: core of the SOS engine.
* L2-eth: Ethernet support library for Sos.
    It is designed to work with the Wiznet W5100 chip
    found on the Arduino Ethernet shield.
* L2-154: IEEE 802.15.4 support library for Sos.
    It is designed to work with an ATmega128RFA1
    chip as found on the Zigduino r2 cards.
* ZigMsg: low-level library to work with ATmega128RFA1 (needed
    for the L2-154 library)


Examples
--------

Examples of use can be found in ../tests directory


Makefile
--------

This directory contains a `Makefile` file. It is used only for
documentation generation with Doxygen. Available targets are:
* `all`: generates the HTML documentation in ./doc/html/index.html
* `pdf`: generates the PDF documentation in ./doc/pdf/refman.pdf.
	Packages needed: `pdflatex` and `graphviz`
* `clean`: removes the ./doc directory


Installation
------------

There is no need to install something. Just make the `USER_LIB_PATH`
variable points to this directory in your application `Makefile`.

However, in order to use the Zigduino platform, keep in mind that you
have to install specific files as described on the Zigduino web site:
* /usr/share/arduino/hardware/arduino/boards.txt
* /usr/share/arduino/hardware/arduino/cores/zigduino/
* /usr/share/arduino/hardware/arduino/variants/zigduino_r2
