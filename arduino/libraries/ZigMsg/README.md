ZigMsg library
==============

Introduction
------------

The ZigMsg library provides an easy to use API for IEEE 802.15.4 network
programming on the Atmega128RFA1 chip (as found on the Zigduino r2 card)

At the lowest level, it is based on the uracoli library 

API description
---------------

The ZigMsg library defines a single object ZigMsg (of class cZigMsg)
to access the radio. It provides the following methods:

    void start (void)
    bool sendto (addr2_t a, uint8_t len, uint8_t payload [])
    ZigReceivedFrame *get_received (void)

The **start** method should be called in the Arduino `setup` function. It
enables radio receiver and message transmission.

The **sendto** method starts sending a message to a given IEEE 802.15.4
short address.

If a frame has been received, the **get_received** method returns the
address of a ZigReceivedFrame which describes the received frame. The IEEE
802.15.4 MAC fields are decoded and may be fetched in the ZigReceivedFrame
data structure (as well as the payload). If no frame has been received,
**get_received** returns NULL.

In addition, the ZigMsg library provides the following methods in order
to set parameters:

    void channel (channel_t chan)
    void addr2 (addr2_t addr)
    void addr8 (addr8_t addr)         // not supported yet. Do not use.
    void panid (panid_t panid)
    void txpower (txpwr_t txpower)
    void promiscuous (bool promisc)
    void msgbufsize (int msgbufsize)

These methods must be called before the **start** method. They have a
"get" counterpart, without parameter (which may be called after the
**start** method) to get parameter values. Note: **msgbufsize** is the
size (in number of frames) of the receive buffer.


Uracoli
-------

The [uracoli](http://uracoli.nongnu.org) library is used as a low-level
library to access the Atmega128RFA1 microcontroller and its radio
subcomponent. To get current uracoli sources, use:

    hg clone http://hg.savannah.nongnu.org/hgweb/uracoli/ /tmp/uracoli

The needed files are copied in this directory using the script:

    sh IMPORT.sh /tmp/uracoli

Current files are imported from the Apr 1, 2014 (change set 2737).
At least if this file is updated...
