Sensor Operating System (SOS)
=============================

This is the source code for SOS.

This directory is structured as follows:

* master: code for the master processor
* arduino: code for the slave processor running on Arduino

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
