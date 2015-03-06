"""
This module contains the subclasses of l2 and l2net for 802.15.4 networking.
"""
import sys
import os
import fcntl
import termios
import time

from casan import l2


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
        s += 'Interface : ' + self.iface + '\n'
        s += 'Interface address : ' + str (self.a) + '\n'
        s += 'PANID : ' + str (self.pan) + '\n'
        s += 'MTU : ' + str (self.mtu) + '\n'

    def __init__ (self):
        """
        Construct a l2net_154 object with some default values.
        """
        self.max_latency = 5
        self.fd = -1
        self.framelist = []
        self.buffer = bytearray ()
        self.iface = None

    def _init_xbee (self, iface, addr, panid, channel):
        """
        Initialize access to the Digi XBee chip
        """
        if not (0 < self.mtu <= self.XBEE_MTU):
            self.mtu = self.XBEE_MTU
        if not (10 < self.channel < 26):
            return False
        try:
            # Open and initialize device
            if not iface.startswith ('/dev'):
                iface = '/dev/' + iface
            self.fd = os.open (iface, os.O_RDWR)
            attrs = termios.tcgetattr (self.fd)
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
            termios.tcsetattr (self.fd, termios.TCSANOW, attrs)

            def send_AT_command (s):
                """
                Sends an AT command over the serial port and checks
                for the returned value.
                """
                os.write (self.fd, s)
                r = bytearray ()
                while True:
                    c = os.read (self.fd, self.XBEE_READ_MAX)
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
                  bytes (hex (self.a.addr [1]) [2:], 'utf-8') +
                  bytes (hex (self.a.addr [0]) [2:], 'utf-8') +
                  b'\r')
            send_AT_command (sa)

            # PANID
            panid = (b'ATID' +
                     bytes (hex (self.pan.addr [1]) [2:], 'utf-8') +
                     bytes (hex (self.pan.addr [0]) [2:], 'utf-8') +
                     b'\r')
            send_AT_command (panid)

            # Channel
            chan = b'ATCH' + bytes (hex (channel) [2:], 'utf-8') + b'\r'
            send_AT_command (chan)

            # 802.15.4 w/ ACKs
            send_AT_command (b'ATMM2\r')

            # Enter API mode
            send_AT_command (b'ATAP1\r')

            # Quit AT command mode
            send_AT_command (b'ATCN\r')

        except Exception as e:
            sys.stderr.write (str (e) + '\n')
            sys.stderr.write ('Error : cannot open interface ' + iface)
            return False

        # configure the device to non-blocking mode for asyncio
        if self.asyncio:
            fcntl.fcntl (self.fd, fcntl.F_SETFL, os.O_NDELAY)

        return True

    def init (self, iface, type_, mtu, addr, panid, channel, asyncio=False):
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
        :param channel: channel
        :type  channel: integer
        :param asyncio: True if this object is used by asyncio
        :type  asyncio: boolean
        :return: True if ok, False if error.
        """

        self.a = l2addr_154 (addr)
        self.pan = l2addr_154 (panid)
        self.channel = channel
        self.mtu = mtu
        self.asyncio = asyncio

        if type_ == 'xbee':
            r = self._init_xbee (iface, addr, panid, channel)
        else:
            raise RuntimeError ('Unknown type ' + str (type_))
        return r

    def term (self):
        """
        Close the connection.
        """
        os.close (self.fd)
        self.fd = -1

    def handle (self):
        """
        Return file handle, needed for asyncio
        :return: file handle
        """
        return self.fd

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
                    r = os.write (self.fd, cmd [:clen])
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
        :return: tuple (PACKET_TYPE, SOURCE_ADDRESS, DATA)
                 PACKET_TYPE = see l2.PkType
        """
        r = (l2.PkType.PK_NONE, None, None)
        while r [0] is l2.PkType.PK_NONE:
            r = self.extract_received_packet ()
            if r [0] is l2.PkType.PK_NONE:
                x = self.read_complete_frame ()
                if not x:
                    break
        return r

    def extract_received_packet (self):
        """
        Extract a packet from the frame list.
        :return: tuple (PACKET_TYPE, (SOURCE_ADDRESS, DATA, LENGTH))
        """
        r = l2.PkType.PK_NONE
        a, data, = None, None
        for frame in self.framelist:
            self.framelist.remove (frame)
            if frame.type == self.XBEE_FT_RX_SHORT:
                a = l2addr_154 ()
                a.addr = bytes ([frame.rx_short.saddr & 0x00ff,
                                 (frame.rx_short.saddr & 0xff00) >> 8])
                data = frame.rx_short.data
                if frame.rx_short.options & self.RX_SHORT_OPT_BROADCAST:
                    r = l2.PkType.PK_BCAST
                else:
                    r = l2.PkType.PK_ME
                    break
        return (r, a, data)

    def read_complete_frame (self):
        """
        Read a full frame from the input buffer and adds it to the frame list.
        This function blocks until a complete frame is received or
        returns False when no input is available (in asyncio case)
        """
        found = False
        while not found:
            try:
                buf = os.read (self.fd, self.XBEE_READ_MAX)
            except BlockingIOError:
                # Exception raised when data is not available and
                # read set in non-blocking mode (i.e. asyncio mode)
                break
            if not buf:
                break
            self.buffer = self.buffer + buf
            while self.is_frame_complete ():
                self.extract_frame_to_list ()
                found = True

        return found

    def is_frame_complete (self):
        """
        Checks if there is a complete frame available for extraction from
        the input buffer.
        """
        complete = False
        while not complete:
            if self.XBEE_START not in self.buffer:
                # No start byte found
                break
            # Align start byte to buffer start
            self.buffer = self.buffer [self.buffer.index (self.XBEE_START):]

            # Is buffer long enough to extract a frame?
            buflen = len (self.buffer)
            if buflen < self.XBEE_MIN_FRAME_SIZE:
                break

            framelen = (self.buffer [1] << 8) | self.buffer [2]
            if framelen > self.XBEE_MAX_FRAME_SIZE:
                self.buffer = self.buffer [1:]
                continue

            # Got checksum?
            pktlen = framelen + 4
            if pktlen > buflen:
                break

            cksum = self.compute_checksum (self.buffer)
            if cksum != self.buffer [pktlen - 1]:
                self.buffer = self.buffer [1:]
                continue

            # At this point, we have a full valid frame in the buffer
            complete = True

        return complete

    def extract_frame_to_list (self):
        """
        Extract a complete frame from the input buffer and adds it to the
        list of received frames
        """
        class Frame:
            pass
        framelen = (self.buffer [1] << 8) | self.buffer [2]
        pktlen = framelen + 4
        f = Frame ()
        f.type = self.buffer [3]
        if f.type == self.XBEE_FT_TX_STATUS:
            f.tx_status = Frame ()
            f.tx_status.frame_id = self.buffer [4]
            f.tx_status.status = self.buffer [5]
        elif f.type == self.XBEE_FT_RX_SHORT:
            f.rx_short = Frame ()
            f.rx_short.saddr = (self.buffer [4] << 8) | self.buffer [5]
            f.rx_short.len = framelen - 5
            f.rx_short.data = bytes (self.buffer [8: f.rx_short.len + 8])
            f.rx_short.rssi = self.buffer [6]
            f.rx_short.options = self.buffer [7]
        else:
            sys.stderr.write ('PKT API ' + str (f.type) + ' unrecognized\n')
            # Return to avoid appending unknown frame
            return
        self.framelist.append (f)
        self.buffer = (bytearray ()
                       if len (self.buffer) == pktlen
                       else self.buffer [pktlen:])
