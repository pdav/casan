"""
This module contains the subclasses of l2 and l2net for 802.15.4 networking.
"""
import sys
import termios
from time import sleep
from datetime import timedelta, datetime
from enum import IntEnum

import serial

from sos import l2
from util.debug import print_debug, dbg_levels


class l2addr_154(l2.l2addr):
    """
    This class represents a 802.15.4 network address.
    """
    def __init__(self, s):
        """
        Initializes the object from a string representation such as 'ca:fe'
        """
        sl = s.split(':')
        self.addr = bytes([int(ss, 16) for ss in sl])

    def __eq__(self, other):
        """
        Tests for equality between 2 l2addr_154 objects.
        """
        return False if other is None else self.addr == other.addr

    def __ne__(self, other):
        """
        Tests for difference between 2 l2addr_154 objects.
        """
        return not self.__eq__(other)

    def __repr__(self):
        """
        Returns a printable representation of a l2addr_154 object.
        """
        rep = ''
        for b in self.addr:
            rep += hex(b)[2:]
        return rep
        
    def __str__(self):
        """
        Returns a human readable string representation of a l2addr_154.
        """
        rep = []
        for b in self.addr:
            atom = hex(b)[2:]
            if len(atom) == 1:
                atom = '0' + atom  # Prefix single-digits with 0
            rep.append(atom)
        return ':'.join(rep)


class l2net_154(l2.l2net):
    """
    This class represents a L2 802.15.4 network connection.
    """
    # Constants
    L2_154_HEADER_SIZE = 9
    L2_154_FCS = 2
    L2_154_MTU = 127
    XBEE_MTU = 100 + L2_154_HEADER_SIZE + L2_154_FCS
    XBEE_START = 0x7E
    XBEE_TX_SHORT = 0x01
    XBEE_MIN_FRAME_SIZE = 5
    XBEE_MAX_FRAME_SIZE = 115
    RX_SHORT_OPT_BROADCAST = 0x02
    broadcast = l2addr_154('ff:ff:ff:ff:ff:ff:ff:ff')

    # Enums
    class FrameType(IntEnum):
        """
        Enumerates frame types.
        TODO : use a regular Enum (gotta change every occurence of it though)
        """
        TX_STATUS = 0x89
        RX_LONG = 0x80
        RX_SHORT = 0x81

    def __str__(self):
        """
        Returns a string representation of the network object.
        Used for debugging purposes.
        """
        s = 'Network type : 802.15.4\n'
        s = s + 'Interface : ' + self.iface + '\n'
        s = s + 'Interface address : ' + str(self.addr) + '\n'
        s = s + 'PANID : ' + str(self.panid) + '\n'
        s = s + 'MTU : ' + str(self.mtu) + '\n'
        return s

    def __init__(self):
        """
        Constructs a l2net_154 object with some default values.
        """
        super().__init__()
        self.max_latency = 5
        self.fd = -1
        self.framelist = []
        self.buffer = bytearray()
        self.iface = ''
        self.addr = None
        self.panid = None
        self.channel = 0
        self.port = None

    def init(self, iface, type_, mtu, addr, panid, channel, timeout=None):
        """
        Initializes a l2net_154 object, opens and sets up the network interface.
        :param iface: interface to start. You can specify the full path to the
                      interface (eg : '/dev/ttyUSB0') ot just it's name (eg : 'ttyUSB0').
        :param type_: type of the interface. Use 'xbee' here.
        :param mtu: MTU to use for interface iface. If not valid, will be set to a
                    default value. If you're not sure, set this to 0.
        :param addr: physical address to use for interface iface.
        :param panid: string representing the PANID for the interface iface.
        :param channel: channel used to communicate within the PAN.
        :param timeout: timeout value in seconds for read operations in seconds.
                        If None, then there is no timeout.
                        Note that the timeout value is not very accurate, but in the worst case scenario it should not
                        exceed twice the timeout value.
                        If you do net set a timeout value, then the program won't be able to shut down gracefully
                        and will have to be killed.
        :return: True if the operation succeeds, False otherwise.
        """
        self.addr = l2addr_154(addr)
        self.panid = l2addr_154(panid)
        self.channel = channel

        if type_ == 'xbee':
            self.mtu = mtu if 0 < mtu <= self.XBEE_MTU else self.XBEE_MTU
            if not (10 < self.channel < 26):
                return False
            try:
                # Open and initialize device
                self.port = serial.Serial(iface if iface.startswith('/dev')
                                          else '/dev/' + iface,
                                          timeout=timeout)
                self.fd = self.port.fileno()
                attrs = termios.tcgetattr(self.fd)
                attrs[:4] = [termios.IGNBRK | termios.IGNPAR,
                             0,
                             termios.CS8 | termios.CREAD | termios.B9600,
                             0]
                #for attr in attrs[6]:
                    # POSIX_VDISABLE may not be implemented in python, at 
                    # least I couldn't find any information about it, maybe
                    # check in python stdlib source code?
                    #attr = termios.POSIX_VDISABLE
                termios.tcsetattr(self.fd, termios.TCSANOW, attrs)

                def send_AT_command(s):
                    """
                    This function sends an AT command over the serial port and checks for the returned value.
                    It may seem a bit 'too much', but it is the best solution I came up with.
                    :param s: AT command to send.
                    :raise: RuntimeError: an error occured.
                    """
                    self.port.write(s)
                    r = bytearray()
                    while True:
                        c = self.port.read()  # Read a byte at a time
                        r += c
                        if c == b'\r':
                            if r[-3:] == b'OK\r':
                                return
                            elif r[-6:] == b'ERROR\r':
                                raise RuntimeError('Error sending AT command to XBEE.\nCommand : ' + s.decode())
                            else:
                                r = bytearray()
                # Initialize API mode
                try:
                    sleep(1)
                    send_AT_command(b'+++')
                    sleep(1)
                    send_AT_command(b'ATRE\r')

                    # Short address
                    sa = (b'ATMY' + bytes(hex(self.addr.addr[1])[2:], 'utf-8') +
                          bytes(hex(self.addr.addr[0])[2:], 'utf-8') + b'\r')
                    send_AT_command(sa)

                    # PANID
                    panid = (b'ATID' + bytes(hex(self.panid.addr[1])[2:], 'utf-8') +
                             bytes(hex(self.panid.addr[0])[2:], 'utf-8') + b'\r')
                    send_AT_command(panid)

                    # Channel
                    chan = b'ATCH' + bytes(hex(channel)[2:], 'utf-8') + b'\r'
                    send_AT_command(chan)

                    # 802.15.4 w/ ACKs
                    send_AT_command(b'ATMM2\r')

                    # Enter API mode
                    send_AT_command(b'ATAP1\r')

                    # Quit AT command mode
                    send_AT_command(b'ATCN\r')

                    self.port.flushInput()
                except RuntimeError as e:  # AT command error
                    print(e)
                    return False
            except Exception as e:
                sys.stderr.write(str(e) + '\n')
                sys.stderr.write('Error : cannot open interface ' + iface)
                return False
        else:
            self.mtu = mtu if (self.mtu > 0) and (self.mtu <= self.L2_154_MTU) else self.L2_154_MTU
        return True

    def term(self):
        """
        Closes the connection.
        """
        self.port.close()
        self.fd = -1

    def encode_transmit(self, dest_addr, data):
        """
        Creates a 802.15.4 frame
        :param dest_addr: destination address as an instance of l2addr_154.
        :param data: data to put in the frame.
        """
        fdlen = 5 + len(data)
        b = bytearray()
        b.append(self.XBEE_START)
        b.append((fdlen & 0xFF00) >> 8)
        b.append(fdlen & 0x00FF)
        b.append(self.XBEE_TX_SHORT)
        b.append(0x41)
        b.append(dest_addr.addr[1])
        b.append(dest_addr.addr[0])
        b.append(0)
        b += data
        b.append(self.compute_checksum(b))

        return b

    @staticmethod
    def compute_checksum(buf):
        """
        Computes the checksum of a ZigBee frame
        :param buf: a complete frame from the frame list.
        :return: an integer between 0 and 255 representing the checksum of the frame.
        """
        def i16_from_2i8(i1, i2):
            """
            Returns a 16bits integer made from 2 8bits integers.
            No checks are performed on the integers, so make sure their value
            is between 0 and 255 included.
            :param i1: most signifigant byte
            :param i2: least significant byte
            :return: concatenation of i1 and i2
            """
            return (i1 << 8) | i2
        paylen = i16_from_2i8(buf[1], buf[2]) + 4
        c = 0
        for i in range(3, paylen-1):
            c = (c + buf[i]) & 0xFF
        return 0xFF - c

    def send(self, dest_slave, data):
        """
        Sends a frame over the network.
        :param dest_slave: recipient slave.
        :param data: data to send.
        :return: True if success, else False.
        """
        if len(data) <= self.L2_154_MTU:
            cmd = self.encode_transmit(dest_slave.addr, data)
            cmd_len = len(cmd)
            try:
                while cmd_len > 0:
                    r = self.port.write(cmd[:cmd_len])
                    if r == 0:
                        return False
                    else:
                        cmd_len = cmd_len - r
                        cmd = cmd[r:]
                return True
            except Exception:
                return False


    def bsend(self, data):
        """
        Broadcasts a frame over the network.
        :param data: data to broadcast.
        """
        return self.send(self.broadcast, data)

    def recv(self):
        """
        Receive a frame.
        :return: tuple such as:
                 (PACKET_TYPE, (SOURCE_ADDRESS, DATA, LENGTH))
                 Where PACKET_TYPE is an enumerated type from the l2.PkTypes enumeration.
                 In case of timeout during the read operation on the network interface, returns the
                 singleton (None,)
        """
        r = (l2.PkTypes.PK_NONE,)
        while r[0] is l2.PkTypes.PK_NONE:
            r = self.extract_received_packet()
            if r[0] is l2.PkTypes.PK_NONE:
                t_start = datetime.now()
                if not self.read_complete_frame():
                    if (self.port.timeout is not None and
                       (t_start + timedelta(seconds=self.port.timeout) < datetime.now())):
                        return (None,)
                    else:
                        continue
        print_debug(dbg_levels.STATE, 'Received packet (' + str(r[1][2]) + ' bytes)')
        return r

    def extract_received_packet(self):
        """
        Extracts a packet from the frame list.
        :return: tuple such as:
                 (PACKET TYPE, (SOURCE ADDRESS, DATA, LENGTH))
        """
        r = l2.PkTypes.PK_NONE
        a, data, = None, None
        for frame in self.framelist:
            if frame.type == self.FrameType.RX_SHORT:
                a = l2addr_154(hex(frame.rx_short.saddr & 0x00FF)[2:] + ':' +
                               hex((frame.rx_short.saddr & 0xFF00) >> 8)[2:])
                data = frame.rx_short.data
                if frame.rx_short.options & self.RX_SHORT_OPT_BROADCAST:
                    r = l2.PkTypes.PK_BCAST
                else:
                    r = l2.PkTypes.PK_ME
            self.framelist.remove(frame)
            break
        return r, (a, data, len(data) if data is not None else 0)

    def read_complete_frame(self):
        """
        Reads a full frame from the input buffer and adds it to the frame list.
        This function blocks until a complete frame is received or the timeout
        is reached, if timeout was set when init was called.
        """
        found = False
        t_start = datetime.now()
        try:
            while not found:
                buf = self.port.read(1 if self.port.inWaiting() == 0
                                     else self.port.inWaiting())
                # Do not check right away for the timeout, just in case we actually have a complete frame.
                self.buffer = self.buffer + buf
                while self.is_frame_complete():
                    found = True
                    self.extract_frame_to_list()
                if (self.port.timeout is not None and
                   (t_start + timedelta(seconds=self.port.timeout) < datetime.now())):
                    return False
            return found
        except Exception as e:
            # pySerial doesn't really say which exceptions read() can raise,
            # but I expect most of them to be fatal. Do we need specific error
            # handling ?
            sys.stderr.write('An error happened while reading frame :\n' + str(e))
            raise

    def is_frame_complete(self):
        """
        Checks if there is a complete frame available for extraction from
        the input buffer.
        """
        complete, invalid = False, True
        while invalid:
            if self.XBEE_START not in self.buffer:
                # No start byte found
                break
            # Align buffer start to start byte
            self.buffer = self.buffer[self.buffer.index(self.XBEE_START):]

            # Buffer long enough to extract frame size?
            buflen = len(self.buffer)
            if buflen < self.XBEE_MIN_FRAME_SIZE:
                break

            framelen = (self.buffer[1] << 8) | self.buffer[2]
            if framelen > self.XBEE_MAX_FRAME_SIZE:
                self.buffer = self.buffer[1:]
                continue
            
            # Got checksum?
            pktlen = framelen + 4
            if pktlen > buflen:
                break

            cksum = self.compute_checksum(self.buffer)
            if cksum != self.buffer[pktlen - 1]:
                self.buffer = self.buffer[1:]
                continue
            invalid, complete = False, True
        # At this point, we have a full valid frame in the buffer
        return complete

    def extract_frame_to_list(self):
        """
        Extracts a complete frame from the input buffer and adds it to the
        received frames list.
        """
        class Frame:
            """
            Represents a 802.15.4 frame
            :return:
            """
            def __init__(self):
                self.tx_status = None
                self.rx_short = None
                self.type = None
        framelen = (self.buffer[1] << 8) | self.buffer[2]
        pktlen = framelen + 4
        f = Frame()
        f.type = self.buffer[3]
        if f.type == self.FrameType.TX_STATUS:
            f.tx_status = Frame()
            f.tx_status.frame_id = self.buffer[4]
            f.tx_status.status = self.buffer[5]
        elif f.type == self.FrameType.RX_SHORT:
            f.rx_short = Frame()
            f.rx_short.saddr = (self.buffer[4] << 8) | self.buffer[5]
            f.rx_short.len = framelen - 5
            f.rx_short.data = bytes(self.buffer[8: f.rx_short.len + 8])
            f.rx_short.rssi = self.buffer[6]
            f.rx_short.options = self.buffer[7]
        else:
            sys.stderr.write('PKT API ' + str(f.type) + ' unrecognised\n')
            # Return to avoid appending unknown frame ?
            return
        self.framelist.append(f)
        self.buffer = (bytearray() if len(self.buffer) == pktlen 
                       else self.buffer[pktlen:])
