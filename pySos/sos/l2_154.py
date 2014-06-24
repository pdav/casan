'''
This module contains the subclasses of l2 and l2net for 802.15.4 networking.
'''
from sos import l2
import sys
import os
import termios
from time import sleep
from util.debug import *
from util.enum import Enum
from conf import Conf
import serial

import pdb

class l2addr_154(l2.l2addr):
    '''
    This class represents a 802.15.4 network address.
    '''
    def __init__(self, s):
        '''
        Initializes the object from a string representation such as 'ca:fe'
        '''
        sl = s.split(':')
        self.addr = bytes([int(ss, 16) for ss in sl])

    def __eq__(self, other):
        '''
        Tests for equality between 2 l2addr_154 objects.
        '''
        return self.addr == other.addr

    def __ne__(self, other):
        '''
        Tests for difference between 2 l2addr_154 objects.
        '''
        return self.addr != other.addr

    def __repr__(self):
        '''
        Returns a printable representation of a l2addr_154 object.
        '''
        rep = ''
        for b in self.addr:
            rep = rep + hex(b)[2:]
        return rep
        
    def __str__(self):
        '''
        Returns a human readable string representation of a l2addr_154.
        '''
        rep = []
        for b in self.addr:
            atom = hex(b)[2:]
            if len(atom) == 1 : atom = '0' + atom # Prefix single-digits with 0
            rep.append(atom)
        return ':'.join(rep)


class l2net_154(l2.l2net):
    '''
    This class represents a L2 802.15.4 network connection.
    '''
    # Constants
    L2_154_HEADER_SIZE = 9
    L2_154_FCS = 2
    L2_154_MTU = 127
    XBEE_MTU = 100 + L2_154_HEADER_SIZE + L2_154_FCS
    XBEE_START = 0x7E
    XBEE_TX_SHORT = 0x01
    XBEE_MIN_FRAME_SIZE = 5
    XMEE_MAX_FRAME_SIZE = 115
    RX_SHORT_OPT_BROADCAST = 0x02
    broadcast = l2addr_154('ff:ff:ff:ff:ff:ff:ff:ff')

    # Enums
    frame_type = Enum('frame_type', {'TX_STATUS' : 0x89,
                                     'RX_LONG' : 0x80,
                                     'RX_SHORT' : 0x81 })

    def __str__(self):
        '''
        Returns a string representation of the network object.
        Used for debugging purposes.
        '''
        s = 'Network type : 802.15.4\n'
        s = s + 'Interface : ' + self.iface + '\n'
        s = s + 'Interface address : ' + str(self.a) + '\n'
        s = s + 'PANID : ' + str(self.pan) + '\n'
        s = s + 'MTU : ' + str(self.mtu) + '\n'

    def __init__(self):
        '''
        Constructs a l2net_154 object with some default values.
        '''
        self.maxlatency = 5
        self.fd = -1
        self.framelist = []
        self.buffer = bytearray()

    def init(self, iface, type_, mtu, addr, panid, channel):
        '''
        Initializes a l2net_154 object, opens and sets up the network 
        interface.
        iface : interface to start. You can spedify the full path to the
                interface (eg : '/dev/ttyUSB0') ot just it's name (eg : 'ttyUSB0')
        type_ : type of the interface. Use 'xbee' here.
        mtu : MTU to use for interface iface. If not valid, will be set to a 
              default value. If you're not sure, set this to 0.
        addr : physical address to use for interface iface
        panid : string representing the PANID for the interface iface.
        channel : channel to use for the communications in the PAN
        Returns True if the operation succeeds, False otherwise.
        '''
        self.a = l2addr_154(addr)
        self.pan = l2addr_154(panid)
        self.channel = channel

        def sendATCommand(serialLink, command):
            '''
            Sends an AT command not returning a value to the xbee and checks
            the xbee reponse. This function will raise a RuntimeError if the
            reponse is not equal to b'OK\r'
            serialLink : the serial object representing the serial connection
            command : bytes object containing the AT command to send.
            '''
            serialLink.write(command)
            if not serialLink.read(3) == b'OK\r':
                raise RuntimeError('Error sending AT command to XBEE.')

        if type_ == 'xbee':
            self.mtu_ = mtu if (0 < mtu <= self.XBEE_MTU) else self.XBEE_MTU
            if not (10 < self.channel < 26):
                return False
            try:
                # Open and initialize device
                self.port = serial.Serial(iface if iface.startswith('/dev')
                                          else '/dev/' + iface)
                self.fd = self.port.fileno()
                attrs = termios.tcgetattr(self.fd)
                attrs[:4] = [termios.IGNBRK | termios.IGNPAR,
                             0,
                             termios.CS8 | termios.CREAD | termios.B9600,
                             0]
                for attr in attrs[6]:
                    # POSIX_VDISABLE may not be implemented in python, at 
                    # least I couldn't find any information about it, maybe
                    # check in python stdlib source code?
                    #attr = termios.POSIX_VDISABLE
                    pass
                termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
                # Initialize API mode
                sendATCommand(self.port, b'+++')
                sleep(1)
                sendATCommand(self.port, b'ATRE\r')

                # Short address
                sa = ( b'ATMY' + bytes(hex(self.a.addr[1])[2:], 'utf-8') + 
                       bytes(hex(self.a.addr[0])[2:], 'utf-8') + b'\r' )
                sendATCommand(self.port, sa)
                
                # PANID
                panid = (b'ATID' + bytes(hex(self.pan.addr[1])[2:], 'utf-8') +
                        bytes(hex(self.pan.addr[0])[2:], 'utf-8') + b'\r')
                sendATCommand(self.port, panid)

                # Channel
                chan = b'ATCH' + bytes(hex(channel)[2:], 'utf-8') + b'\r'
                sendATCommand(self.port, chan)

                # 802.15.4 w/ ACKs
                sendATCommand(self.port, b'ATMM2\r')

                # Enter API mode
                sendATCommand(self.port, b'ATAP1\r')

                # Quit AT command mode
                sendATCommand(self.port, b'ATCN\r')

            except Exception as e:
                sys.stderr.write(str(e) + '\n')
                sys.stderr.write('Error : cannot open interface ' + iface)
                return False 
        else:
            self.mtu = mtu if (self.mtu > 0) and (self.mtu <= self.L2_154_MTU) else self.L2_154_MTU
        return True

    def term(self):
        '''
        Closes the connection.
        '''
        os.close(self.fd)
        self.fd = -1

    def encode_transmit(self, destAddr, data):
        '''
        Creates a 802.15.4 frame
        '''
        fdlen = 5 + len(data)
        cmdlen = 4 + fdlen
        b = bytearray()
        b[0] = self.XBEE_START
        b[1] = (fdlen & 0xFF00) >> 8
        b[2] = fdlen & 0x00FF
        b[3] = self.XBEE_TX_SHORT
        b[4] = 0x41
        b[5] = destAddr.addr[1]
        b[6] = destAddr.addr[0]
        b[7] = 0
        b = b + data
        b = b + compute_checksum(b)

        return b

    def compute_checksum(self, buf):
        '''
        Computes the checksum of a 802.15.4 frame.
        '''
        def i16_from_2i8(i1, i2):
            '''
            Returns a 16bits integer made from 2 8bits integers.
            The first integer will be the most significant byte of the result
            '''
            return (i1 << 8) | i2
        paylen = i16_from_2i8(buf[1], buf[2]) + 4
        c = 0
        for i in range(3, paylen):
            c = (c + buf[i]) & 0xFF
        return 0xFF - c

    def send(self, destAddr, data):
        '''
        Sends a frame over the network.
        '''
        if len(data) <= self.L2_154_MTU:
            cmd = encode_transmit(destAddr, data)
            cmdLen = len(cmd)
            n = 0
            while cmdLen > 0:
                r = os.write(self.fd, cmd[:cmdLen])
                if r == -1:
                    return -1
                else:
                    cmdLen = cmdLen - r
                    cmd = cmd[r:]
                    n = n + r
            return n

    def bcastaddr(self):
        return self.broadcast

    def bsend(self, data):
        '''
        Broadcasts a frame over the network.
        '''
        return self.send(self.broadcast, data)

    def recv(self):
        '''
        Receive a frame.
        Returns a tuple such as :
        (PACKET TYPE, (SOURCE ADDRESS, DATA, LENGTH))
        '''
        r = (l2.pktype.PK_NONE,)
        while r[0] == l2.pktype.PK_NONE:
            r = self.extract_received_packet()
            if r[0] == l2.pktype.PK_NONE:
                if self.read_complete_frame() == -1:
                    return 
        print_debug(dbg_levels.STATE, 'Received packet (' + str(r[0][2]) + ' bytes)')
        return r

    def extract_received_packet(self):
        '''
        Extracts a packet from the frame list.
        Returns a tuple containing :
            - The packet type
            - A tuple containing:
                - The source address
                - The received data
                - The length of the data
        '''
        r = l2.pktype.PK_NONE
        for frame in self.framelist:
            if frame.type == self.frame_type.RX_SHORT:
                a = l2addr_154(hex(f.rx_short & 0x00FF)[2:] + 
                               hex((f.rx_short & 0xFF00) >> 8)[2:])
                if len_ >= f.rx_short.len_ :
                    data = bytes(f.rx_short.data)
                if f.rx_short.options & self.RX_SHORT_OPT_BROADCAST:
                    r = l2.pktype.PK_BCAST
                else:
                    r = l2.pktype.PK_ME
                break
        return (r, (a, data, len_))

    def read_complete_frame(self):
        '''
        Reads a full frame from the input buffer and adds it to the frame list.
        '''
        found = False
        try:
            while not found:
                buf = os.read(self.fd, 1024) # Test value, but should do it
                self.buffer = self.buffer + buf
                while self.is_frame_complete():
                    found = True
                    self.extract_frame_to_list()
            return 0 if found else -1
        except Exception as e:
            # TODO
            sys.stderr.write('Exception caught in read_complete_frame, go fix your code!\n')
            raise

    def is_frame_complete(self):
        '''
        Checks if there is a complete frame available for extraction in
        the input buffer.
        '''
        complete, invalid = False, True
        while invalid:
            if self.XBEE_START not in self.buffer:
                # No start byte found
                break
            # Align start byte to buffer start
            self.buffer = self.buffer[self.buffer.index(self.XBEE_START):]

            # Buffer long enough to extract frame size?
            buflen = len(self.buffer)
            if buflen < self.XBEE_MIN_FRAME_SIZE:
                break;

            framelen = (self.buffer[1] << 8) | self.buffer[2]
            if framelen > self.XBEE_MAX_FRAME_SIZE:
                buffer = self.buffer[1:]
                continue
            
            # Got checksum?
            pktlen = framelen + 4
            if pktlen < buflen:
                break

            cksum = compute_checksum(self.buffer)
            if cksum != self.buffer[pktlen - 1]:
                buffer = self.buffer[1:]
                continue
            invalid, complete = False, True
        # At this point, we have a full valid frame in the buffer
        return complete


    def extract_frame_to_list(self):
        '''
        Extracts a complete frame from the input buffer and adds it to the
        received frames list.
        '''
        framelen = (self.buffer[1] << 8) | self.buffer[2]
        pktlen = framelen + 4
        f = self.frame()
        f.type = self.buffer[3]
        if f.type == self.frame_type.TX_STATUS:
            f.tx_status.frame_id = self.buffer[4]
            f.tx_status.status = self.buffer[5]
        elif f.type == self.frame_type.RX_SHORT:
            f.rx_short.saddr = (self.buffer[4] << 8) | self.buffer[5]
            f.rx_short.len_ = framelen - 5
            f.rx_short.data = bytearray(self.buffer[i] for i in range(8, f.rx_short.len + 1))
            f.rx_short.rssi = self.buffer[6]
            f.rx_short.options = self.buffer[7]
        else:
            sys.stderr.write('PKT API ' + str(f.type) + ' unrecognised\n')
            # Return to avoid appending unknown frame ?
            return
        self.framelist.append(f)
        self.buffer = self.buffer[pktlen:]
