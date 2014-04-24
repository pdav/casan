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
