CASAN master control program
============================

This directory contains the Python version of the master control program.


Prequisites
-----------

Prerequisites for the CASAN master program are:

* Python >= 3.4
* Aiohttp >= 0.14 (<https://github.com/KeepSafe/aiohttp>)


Running the master program
--------------------------

To run the master program, adapt the `casand.conf` configuration file
and run :

    # ./casand.py -c ./casand.conf

It is possible to add debug options: `-d msg`, `-d slave`, etc.
or to group debug options: `-d msg,slave,...`

To know all debug options, run `./casand -h`

The master program needs root privileges, in order to:

* access the Ethernet layer directly (without TCP or UDP)
* open the serial device (if using the Digi XBee USB stick)

On Linux, you can add yourself to the `dialout` group in order to run
the master program as an ordinary user.

See also `../README.md` for a note on hardwiring serial devices on Linux.


Using the event logger
----------------------

The CASAN master can provide access (read and optionally write access)
to its internal event log.

### Configuration

To configure the event logger, simply add the following sections in the
configuration file:

    [evlog]
    size = 1000
    add = False
    
    [namespace evlog]
    uri = /evlog

The first section specify the event log parameters:

* the `size` parameter gives the maximum number of event log entries,
  the value 0 means that the event log is not limited (which is not
  a good idea).
* `add` means that external programs may add new events (which is not
  a good idea) in the event log with the HTTP server if the `evlog`
  namespace is enabled.

The second section (`namespace evlog`) makes the event log available
with the embedded HTTP server. The `uri` parameter specifies the base
URI of the event log.

### Example

For example, with this configuration file:

    [evlog]
    size = 1000
    add = True
    
    [namespace evlog]
    uri = /foo

To get the remembered events since the "Fri May 29 10:46:39 CEST 2015",
just access the URI `http://localhost:8004/foo/get` with the `since`
parameter (number of seconds since the Epoch). For example:

    curl http://localhost:8004/foo/get?since=1432889199

Since the addition of new events is allowed
In order to add a new event, just access the URI
`http://localhost:8004/foo/add` with the `src`, `msg` and optionally
the `msg` parameters. For example:

    curl http://localhost:8004/foo/add?src=example1&msg=test%20foo%20bar
