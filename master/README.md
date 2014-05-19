SOS master control program
==========================

This directory contains the source code of the master control program.

It includes the following subdirectories:
- SOS protocol handling
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


Compilation
-----------

Prerequisites for the SOS master program are:
- `g++` version >= 4.7 (see `g++ -v`)
- ASIO library (Debian package `libasio-dev`)

Compilation options (see `Makefile`):
- `L2ETH`: define to `USE_PF_PACKET` for Linux (`USE_PCAP` for FreeBSD
	is not available now) or nothing to disable Ethernet support
- `ASIOINC`: specific options needed to compile with ASIO library


Running the master program
--------------------------

To run the master program, adapt the `sosd.conf` configuration file to
your needs, and run :

    # ./sosd -d all -c ./sosd.conf

You can selectively enable or disable debug options with the "+/-" syntax.
For example :
- `-d +all-message-conf`: enable `all` debug messages but `message` and `conf` ones
- `-d +message+option+cache`: enable `message`, `option` and `cache` debug messages
- `-d all -d -conf -d conf`: enable `all` debug messages, disable `conf` debug messages and re-enable `conf` debug messages
To know all debug options, run `./sosd -h`

You need root privileges since the master program:
- access the Ethernet layer directly (without TCP or UDP)
- open the serial device (if using the Digi XBee USB stick)

On Linux, you can add yourself to the `dialout` group in order to run
the master program as an ordinary user.

See also ../README.md for a note on hardwiring serial devices on Linux.


Documentation
-------------

Program documentation is generated with Doxygen. To generate it, you need
`doxygen`, `dot` (part of graphviz) and `pdflatex`.

Just type `make pdf` to generate the `doc/latex/refman.pdf` file.
