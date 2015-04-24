CASAN master control program (Python version)
=============================================

This directory contains the master control program.


Prequisites
-----------

Prerequisites for the CASAN master program are:
- Python >= 3.4


Running the master program
--------------------------

To run the master program, adapt the `casand.conf` configuration file to
your needs, and run :

    # ./casand.py -c ./casand.conf

You can add debug options: -d msg, -d slave, etc.
or you can group debug options: -d msg,slave,...

To know all debug options, run `./casand -h`

You need root privileges since the master program:
- access the Ethernet layer directly (without TCP or UDP)
- open the serial device (if using the Digi XBee USB stick)

On Linux, you can add yourself to the `dialout` group in order to run
the master program as an ordinary user.

See also ../README.md for a note on hardwiring serial devices on Linux.
