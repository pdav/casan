import l2
import sys
import os
import termios
import debug
from time import sleep
from enum import Enum

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
        return self.addr == other.addr

    def __ne__(self, other):
        return self.addr != other.addr

    def __repr__(self):
        rep = ''
        for b in self.addr:
            rep = rep + hex(b)[2:]
        return rep
        
    def __str__(self):
        rep = []
        for b in self.addr:
            atom = hex(b)[2:]
            if len(atom) == 1 : atom = '0' + atom # Prefix single-digits with 0
            rep.append(atom)
        return ':'.join(rep)

broadcast = l2addr_154('ff:ff:ff:ff:ff:ff:ff:ff')

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

    # Enums
    frame_type = Enum('frame_type', {'TX_STATUS' : 0x89,
                                     'RX_LONG' : 0x80,
                                     'RX_SHORT' : 0x81 })

    # Nested types
    class frame:
        pass

    def __init__(self):
        self.maxlatency = 5
        self.fd = -1
        self.framelist = []
        self.buffer_ = bytearray()

    def init(self, iface, type_, mtu, addr, panid, channel):
        self.a = l2addr_154(addr)
        self.pan = l2addr_154(panid)
        n = -1
        
        if type_ == 'xbee':
            self.mtu_ = mtu if (0 < mtu <= self.XBEE_MTU) else self.XBEE_MTU
            if not (10 < self.channel < 26):
                return -1 # Will change
            try:
                # Open and initialize device
                self.fd = os.open(iface if iface.startswith('/dev') else '/dev/' + iface,
                                       os.O_RDWR)
                attrs = termios.tcgetattr(self.fd)
                attrs[:4] = [termios.IGNBRK | termios.IGNPAR,
                             0,
                             termios.CS8 | termios.CREAD | termios.B9600,
                             0]
                for attr in attrs[6]:
                    attr = termios._POSIX_VDISABLE
                termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
                # Initialize API mode
                sleep(2)
                os.write(self.fd, '+++')
                sleep(2)
                os.write(self.fd, 'ATRE\r')

                # Short address
                sa = 'ATMY' + hex(self.a.addr[1])[2:] + hex(self.a.addr[0])[2:] + '\r'
                os.write(self.fd, sa)
                
                # PANID
                panid = 'ATID' + hex(self.pan.addr[1])[2:] + hex(self.pan.addr[0])[2:] + '\r'
                os.write(self.fd, panid)

                # Channel
                chan = 'ATCH' + hex(channel)[2:] + '\r'
                os.write(self.fd, chan)

                # 802.15.4 w/ ACKs
                os.write(self.fd, 'ATMM2\r')

                # Enter API mode
                os.write(self.fd, 'ATAP1\r')

                # Quit AT command mode
                os.write(self.fd, 'ATCN\r')

                n = 0

            except:
                sys.stderr.write('Error : cannot open interface ' + iface)
                return -1 # Will change
        else:
            self.mtu = mtu if (self.mtu > 0) and (self.mtu <= self.L2_154_MTU) else self.L2_154_MTU
        return n


    '''
    Closes the connection
    '''
    def term(self):
        os.close(self.fd)
        self.fd = -1

    '''
    Creates a 802.15.4 frame
    '''
    def encode_transmit(self, destAddr, data):
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
        def i16_from_2i8(i1, i2):
            return (i1 << 8) | i2
        paylen = i16_from_2i8(buf[1], buf[2]) + 4
        c = 0
        for i in range(3, paylen):
            c = (c + buf[i]) & 0xFF
        return 0xFF - c

    def send(self, destAddr, data):
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
        return broadcast

    def bsend(self, data):
        return self.send(self.bcastaddr(), data)

    def recv(self):
        r = (l2.pktype.PK_NONE,)
        while r[0] == l2.pktype.PK_NONE:
            r = self.extract_received_packet()
            if r[0] == l2.pktype.PK_NONE:
                if self.read_complete_frame() == -1:
                    return 
        debug.print_debug('Received packet (' + str(r[0][2]) + ' bytes)')
        return r

    def extract_received_packet(self):
        r = l2.pktype.PK_NONE
        for frame in self.framelist:
            if frame.type_ == self.frame_type.RX_SHORT:
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
        found = False
        try:
            while not found:
                buf = os.read(self.fd, )
                self.buffer_ = self.buffer_ + buf
                while self.is_frame_complete():
                    found = True
                    self.extract_frame_to_list()
            return 0 if found else -1
        except Exception as e:
            # TODO
            sys.write('Exception caught in read_complete_frame, go fix your code!\n')
            raise

    def is_frame_complete(self):
        complete, invalid = False, True
        while invalid:
            if self.XBEE_START not in self.buffer_:
                # No start byte found
                break
            # Align start byte to buffer start
            self.buffer_ = self.buffer_[self.buffer_.index(self.XBEE_START):]

            # Buffer long enough to extract frame size?
            buflen = len(self.buffer_)
            if buflen < self.XBEE_MIN_FRAME_SIZE:
                break;

            framelen = (self.buffer_[1] << 8) | self.buffer_[2]
            if framelen > self.XBEE_MAX_FRAME_SIZE:
                buffer_ = self.buffer_[1:]
                continue
            
            # Got checksum?
            pktlen = framelen + 4
            if pktlen < buflen:
                break

            cksum = compute_checksum(self.buffer_)
            if cksum != self.buffer_[pktlen - 1]:
                buffer_ = self.buffer_[1:]
                continue
            invalid, complete = False, True
        # At this point, we have a full valid frame in the buffer
        return complete


    def extract_frame_to_list(self):
        framelen = (self.buffer_[1] << 8) | self.buffer_[2]
        pktlen = framelen + 4
        f = self.frame()
        f.type_ = self.buffer_[3]
        if f.type_ == self.frame_type.TX_STATUS:
            f.tx_status.frame_id = self.buffer_[4]
            f.tx_status.status = self.buffer_[5]
        elif f.type_ == self.frame_type.RX_SHORT:
            f.rx_short.saddr = (self.buffer_[4] << 8) | self.buffer_[5]
            f.rx_short.len_ = framelen - 5
            f.rx_short.data = bytearray(self.buffer_[i] for i in range(8, f.rx_short.len_ + 1))
            f.rx_short.rssi = self.buffer_[6]
            f.rx_short.options = self.buffer_[7]
        else:
            sys.stderr.write('PKT API ' + str(f.type_) + ' unrecognised\n')
            # Return to avoid appending unknown frame ?
            return
        self.framelist.append(f)
        self.buffer_ = self.buffer_[pktlen:]
