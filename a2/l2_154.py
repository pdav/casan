"""
This module contains the l2addr and l2net classes for 802.15.4 networking
"""

import sys
import os
import fcntl
import termios
import time


class l2addr_154:
    """
    This class represents a 802.15.4 network address.
    """
    def __init__ (self, s=None):
        """
        Initialize the object from a string representation such as 'ca:fe'
        """
        if s is None:
            self.addr = None
        else:
            sl = s.split (':')
            self.addr = bytes ([int (ss, 16) for ss in sl])

    def __eq__ (self, other):
        """
        Test for equality between 2 l2addr_154 objects.
        """
        return False if other is None else self.addr == other.addr

    def __ne__ (self, other):
        """
        Test for difference between 2 l2addr_154 objects.
        """
        return not self.__eq__ (other)

    def __repr__ (self):
        """
        Return a printable representation of a l2addr_154 object.
        """
        rep = ''
        for b in self.addr:
            rep += hex (b) [2:]
        return rep

    def __str__ (self):
        """
        Return a human readable string representation of a l2addr_154.
        """
        rep = []
        for b in self.addr:
            atom = hex (b) [2:]
            if len (atom) == 1:
                atom = '0' + atom        # Prefix single-digits with 0
            rep.append (atom)
        return ':'.join (rep)


class l2net_154:
    """
    This class represents a L2 802.15.4 network connection.
    """
    #
    # Constants (Python makes them class variables)
    #
    HEADER_SIZE = 9
    FCS = 2
    MTU = 127
    XBEE_MTU = 100 + HEADER_SIZE + FCS
    XBEE_START = 0x7e
    XBEE_TX_SHORT = 0x01
    XBEE_MIN_FRAME_SIZE = 5
    XBEE_MAX_FRAME_SIZE = 115
    RX_SHORT_OPT_BROADCAST = 0x02
    XBEE_READ_MAX = 1000
    # XBee frame types
    XBEE_FT_TX_STATUS = 0x89
    XBEE_FT_RX_LONG = 0x80
    XBEE_FT_RX_SHORT = 0x81

    broadcast = l2addr_154 ('ff:ff:ff:ff:ff:ff:ff:ff')

    def __str__ (self):
        """
        Return a string representation of the network object.
        Used for debugging purposes.
        """
        s = 'Network type : 802.15.4\n'
        s += 'Interface : ' + self._iface + '\n'
        s += 'Interface address : ' + str (self._a) + '\n'
        s += 'PANID : ' + str (self._pan) + '\n'
        s += 'MTU : ' + str (self._mtu) + '\n'
        return s

    def __init__ (self):
        """
        Construct a l2net_154 object with some default values.
        """
        self._max_latency = 5             # XXX needed ?
        self._fd = -1
        #######################self._framelist = []
        self._buffer = bytearray ()
        self._iface = None
        self._channel = None
        self._a = None

    def _init_xbee (self):
        """
        Initialize access to the Digi XBee chip
        """
        if not (0 < self._mtu <= self.XBEE_MTU):
            self._mtu = self.XBEE_MTU
        try:
            # Open and initialize device
            if not self._iface.startswith ('/dev'):
                self._iface = '/dev/' + self._iface
            self._fd = os.open (self._iface, os.O_RDWR)
            attrs = termios.tcgetattr (self._fd)
            attrs [:4] = [termios.IGNBRK | termios.IGNPAR,
                          0,
                          termios.CS8 | termios.CREAD | termios.B9600,
                          0]
            for attr in attrs [6]:
                # POSIX_VDISABLE may not be implemented in python, at
                # least I couldn't find any information about it, maybe
                # check in python stdlib source code?
                #attr = termios.POSIX_VDISABLE
                pass
            termios.tcsetattr (self._fd, termios.TCSANOW, attrs)

            def send_AT_command (s):
                """
                Sends an AT command over the serial port and checks
                for the returned value.
                """
                os.write (self._fd, s)
                r = bytearray ()
                while True:
                    c = os.read (self._fd, self.XBEE_READ_MAX)
                    r += c
                    if c == b'\r':
                        if r [-3:] == b'OK\r':
                            return
                        elif r [-6:] == b'ERROR\r':
                            raise RuntimeError ('Error sending AT command'
                                                + ' to XBEE.\nCommand : '
                                                + s.decode ())
                        else:
                            r = bytearray ()

            # Initialize API mode
            time.sleep (0.2)
            send_AT_command (b'+++')
            time.sleep (0.2)
            send_AT_command (b'ATRE\r')

            # Short address
            sa = (b'ATMY' +
                  bytes (hex (self._a.addr [1]) [2:], 'utf-8') +
                  bytes (hex (self._a.addr [0]) [2:], 'utf-8') +
                  b'\r')
            send_AT_command (sa)

            # PANID
            panid = (b'ATID' +
                     bytes (hex (self._pan.addr [1]) [2:], 'utf-8') +
                     bytes (hex (self._pan.addr [0]) [2:], 'utf-8') +
                     b'\r')
            send_AT_command (panid)

            # Channel
            chan = b'ATCH' + bytes (hex (self._channel) [2:], 'utf-8') + b'\r'
            send_AT_command (chan)

            # 802.15.4 w/ ACKs
            send_AT_command (b'ATMM2\r')

            # Enter API mode
            send_AT_command (b'ATAP1\r')

            # Quit AT command mode
            send_AT_command (b'ATCN\r')

        except Exception as e:
            sys.stderr.write (str (e) + '\n')
            sys.stderr.write ('Error : cannot open interface ' + self._iface)
            return False

        # configure the device to non-blocking mode for asyncio
        if self._asyncio:
            fcntl.fcntl (self._fd, fcntl.F_SETFL, os.O_NDELAY)

        return True

    def init (self, iface, type, mtu, addr, panid, channel, asyncio=False):
        """
        Initialize a l2net_154 object, opens and sets up the network interface
        :param iface: interface to start. Full path (eg: '/dev/ttyUSB0') or
                    just device name (eg: 'ttyUSB0').
        :type  iface: string
        :param type_: type of the interface. Use 'xbee' here.
        :type  type_: string
        :param mtu: MTU for interface iface. If not valid, will be set to a
                    default value. 0 means default value.
        :type  mtu: integer
        :param addr: physical address to use for interface iface.
        :type  addr: string
        :param panid: PANID
        :type  panid: string
        :param channel: channel number (11..26)
        :type  channel: integer
        :param asyncio: True if this object is used by asyncio
        :type  asyncio: boolean
        :return: True if ok, False if error.
        """

        if not (11 <= channel <= 26):
            raise RuntimeError ('Invalid 802.15.4 channel ' + str (channel))

        self._a = l2addr_154 (addr)
        self._iface = iface
        self._pan = l2addr_154 (panid)
        self._channel = channel
        self._mtu = mtu
        self._asyncio = asyncio

        if type == 'xbee':
            r = self._init_xbee ()
        else:
            raise RuntimeError ('Unknown 802.15.4 subtype ' + str (type))
        return r

    def term (self):
        """
        Close the connection.
        """
        os.close (self._fd)
        self._fd = -1

    def handle (self):
        """
        Return file handle, needed for asyncio
        :return: file handle
        """
        return self._fd

    def encode_transmit (self, destAddr, data):
        """
        Encode data into a valid 802.15.4 frame
        """
        dlen = 5 + len (data)
        cmdlen = 4 + dlen
        b = bytearray ()
        b.append (self.XBEE_START)
        b.append ((dlen & 0xff00) >> 8)
        b.append (dlen & 0x00ff)
        b.append (self.XBEE_TX_SHORT)
        b.append (0x41)
        b.append (destAddr.addr [1])
        b.append (destAddr.addr [0])
        b.append (0)
        b += data
        b.append (self.compute_checksum (b))
        return b

    @staticmethod
    def compute_checksum (buf):
        """
        Compute the checksum of a frame.
        """
        def i16_from_2i8 (i1, i2):
            """
            Returns a 16bits integer made from 2 8bits integers.
            The first integer will be the most significant byte of the result
            """
            return (i1 << 8) | i2
        paylen = i16_from_2i8 (buf [1], buf [2]) + 4
        c = 0
        for i in range (3, paylen - 1):
            c = (c + buf [i]) & 0xff
        return 0xff - c

    def send (self, dest_slave, data):
        """
        Send a frame on the network.
        :return: True if success, False either.
        """
        if len (data) <= self.MTU:
            cmd = self.encode_transmit (dest_slave.addr, data)
            clen = len (cmd)
            try:
                while clen > 0:
                    r = os.write (self._fd, cmd [:clen])
                    if r == 0:
                        return False
                    else:
                        clen = clen - r
                        cmd = cmd [r:]
                return True
            except Exception:
                return False

    def bsend (self, data):
        """
        Broadcast a frame on the network.
        """
        return self.send (self.broadcast, data)

    def recv (self):
        """
        Receive a frame.
        :return: tuple (packet-dest, src-addr, data) or None
                 packet-dest = 'me' or 'bcast'
        """
        r = None
        while r is None:
            # Get a received 802.15.4 message from the buffer
            r = self.extract_packet ()
            if r is None:
                try:
                    buf = os.read (self._fd, self.XBEE_READ_MAX)
                except BlockingIOError:
                    # Exception raised when data is not available and
                    # read set in non-blocking mode (i.e. asyncio mode)
                    break

                if buf is None:
                    # EOF on the device?
                    break

                self._buffer += buf

        return r

    def extract_packet (self):
        """
        Extract a packet from the input buffer
        :return: tuple (packet-dest, src-addr, data) or None
                 packet-dest = 'me' or 'bcast'
        """
        r = None
        while r is None:
            # Extract a complete XBee frame (frame data only, type bytearray)
            frame = self.get_frame ()
            if frame is None:
                # No frame found. Stop.
                break

            type = frame [0]
            if type == self.XBEE_FT_TX_STATUS:
                frame_id = frame [1]
                status = frame [2]
                # XXX we should check if the last packet was successfully
                # transmitted

            elif type == self.XBEE_FT_RX_SHORT:
                a = l2addr_154 ()
                a.addr = bytes ([frame [2], frame [1]])
                rssi = frame [3]
                options = frame [4]
                data = frame [5:]

                if options & self.RX_SHORT_OPT_BROADCAST:
                    pktype = 'bcast'
                else:
                    pktype = 'me'

                r = (pktype, a, data)

            else:
                sys.stderr.write ('PKT API ' + str (type) + ' unrecognized\n')

        return r

    def get_frame (self):
        """
        Check if a complete XBee frame is available for extraction from
        the input buffer, and return frame data.
        :return: XBee frame data (bytearray) or None
        """
        frame = None
        while frame is None:
            if self.XBEE_START not in self._buffer:
                # No start byte found
                break
            # Align start byte to buffer start
            self._buffer = self._buffer [self._buffer.index (self.XBEE_START):]

            # Is buffer long enough to extract a frame?
            buflen = len (self._buffer)
            if buflen < self.XBEE_MIN_FRAME_SIZE:
                break

            # Is the decoded framelen a valid frame length?
            framelen = (self._buffer [1] << 8) | self._buffer [2]
            if framelen > self.XBEE_MAX_FRAME_SIZE:
                self._buffer = self._buffer [1:]
                continue

            # Got checksum?
            pktlen = framelen + 4
            if pktlen > buflen:
                # Not yet: we don't have a complete frame in the buffer
                break

            # Is the checksum valid?
            cksum = self.compute_checksum (self._buffer)
            if cksum != self._buffer [pktlen - 1]:
                self._buffer = self._buffer [1:]
                continue

            # At this point, we have a full valid frame in the buffer.
            # Remove the whole frame from the buffer and return frame
            # data only
            frame = self._buffer [3:3 + framelen]
            del (self._buffer [:pktlen])

        return frame
