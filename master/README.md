SOS master control program
==========================

This directory contains the source code of the master control program.

It includes the following subdirectories:
- SOS processing
- HTTP server


SOS program flow
----------------

The SOS master program is made of various threads:
- HTTP servers are organized in a pool of threads waiting
    for requests
- a thread is associated to each network device, waiting for
    incoming L2 frames
- a thread is dedicated to outgoing SOS messages (whatever
    the network device is). This threads also checks if
    retransmission is needed
- the main thread just waits for a signal to terminate the
    program

