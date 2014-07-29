Sensor Operating System (SOS)
=============================

This is the source code for SOS.

This directory is structured as follows:

* master: code for the master processor
* arduino: code for the slave processor running on Arduino

Please read the specific README in each subdirectory.


Source code style guide
-----------------------

Here are some elements of style for reading the source code:
- all source files are developped with a classical text editor (vi).
- indentation is 4 space wide.
- don't modify the tabulation size in your editor configuration: it
    should remain the standard 8 column, as with your printer.
    So, indentation is a combination of tabulations or spaces.
- SOS master code is documented via Doxygen. To generate the complete
    documentation, you need `doxygen`, `dot` (part of graphviz) and
    `pdflatex`: just type `make pdf` in `./master` directory.
- SOS slave code is documented via Doxygen. To generate the complete
    documentation, you need `doxygen`, `dot` (part of graphviz) and
    `pdflatex`: just type `make pdf` in `./arduino/libraries` directory.


Notes for Linux
---------------

This section give some quirks to work on Linux.

### How to detect USB device for Digi XStick on Linux?

Use the *lsusb* command:

    # lsusb -d 0403:6001			# vendorId : productId
	0403 is hex for FTDI SB-serial
	6015 is hex for Digi dev 		# 6001 for Zigduino's

To find all USB-serial devices:

    # lsusb -d 0403:

However, the lsusb command does not report device path in /dev

### How to assign a symbolic (persistant) name for an USB device?

The problem is that USB devices are automatically numbered by the kernel, and this numbering is not stable over time.

See http://hintshop.ludvig.co.nz/show/persistent-names-usb-serial-devices/

Method:

    # udevadm info --attribute-walk /dev/ttyUSB0 | grep '{serial}' | head -n1
        ATTRS{serial}=="DA00AFPO"

Create a new file called /etc/udev/rules.d/99-usb-serial.rules and put
the following line in there:

    SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6015", ATTRS{serial}=="DA00AFPO", SYMLINK+="zig0"
    SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6015", ATTRS{serial}=="DA00AFPQ", SYMLINK+="zig1"
    SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", SYMLINK+="digi"

and restart udev:

    # service udev restart

Unplug and plug again your devices. You should show them:

    # ls -l /dev/zig*
    lrwxrwxrwx 1 root root 7 Apr 17 11:54 /dev/zig0 -> ttyUSB0
    lrwxrwxrwx 1 root root 7 Apr 17 11:54 /dev/zig1 -> ttyUSB1


Code metrics
------------

Reliable code metrics:

### Master

    $ find master -name "*.[ch]*" | xargs wc -l
    $ find master -name "*.[ch]*" | xargs cat | tr -d -c ";" | wc -c

### Arduino slave

    $ find arduino/libraries -name "*.[ch]*" | grep -v /boards/ | xargs wc -l
    $ find arduino/libraries -name "*.[ch]*" | grep -v /boards/ | xargs cat | tr -d -c ";" | wc -c

Note: sub-directory `/boards` is not counted since it includes all boards
supported by the uracoli library.
