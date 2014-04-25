Sos library - Core of the SOS engine
====================================

Introduction
------------

The Sos library is the core of the SOS engine for Arduino-compatible
cards. It provides a very simple API for resource constrained devices.
It relies on ../L2-* libraries for network access.


API description
---------------

In order to program a SOS slave device, one must:
* provide handler functions for its resources
* in the application `setup` function:
  * create an L2 object, with appropriate parameters, in order to gain network access
  * create as many resources as needed, and provide handlers for these resources
  * create a Sos object, using the L2 object
  * register resources
* in the application `loop` function:
  * call the Sos **loop** method in order to associate with the Sos master, answer its requests and retransmit lost messages


Sub-libraries and dependancies
------------------------------

The Sos library is not tied to any particular network technology.
Network access is abstracted by l2addr and l2net virtual classes.

Real network access is provided by specific derived classes,
using specific directories in order to compile only what you
really need:
* L2-eth: Ethernet access using the Wiznet W5100 found on the
    Arduino Ethernet shield. This library needs in turn the
    Arduino standard libraries SPI and Ethernet.
* L2-154: IEEE 802.15.4 access using the ATmel ATmega128RFA1
    microcontroller as found on the Zigduino r2 for example.
    This library needs in turn the ZigMsg library (included
    in the ../ZigMsg directory)
