"""
This module contains the L2addr and L2net classes for Ethernet networking
"""

import socket


# pylint: disable=invalid-name, too-few-public-methods
class L2addr_eth (object):
    """
    This class represents an Ethernet network address.
    """
    def __init__ (self, s=None):
        """
        Initialize the object from a string representation such as
        'e8:e0:b7:29:03:63'
        """
        if s is None:
            self.addr = None
        else:
            sl = s.split (':')
            self.addr = bytes ([int (ss, 16) for ss in sl])

    def __eq__ (self, other):
        """
        Test for equality between 2 L2addr_eth objects.
        """
        return False if other is None else self.addr == other.addr

    def __ne__ (self, other):
        """
        Test for difference between 2 L2addr_eth objects.
        """
        return not self.__eq__ (other)

    def __repr__ (self):
        """
        Return a printable representation of a L2addr_eth object.
        """
        rep = ''
        for b in self.addr:
            rep += hex (b) [2:]
        return rep

    def __str__ (self):
        """
        Return a human readable string representation of a L2addr_eth.
        """
        rep = []
        for b in self.addr:
            atom = hex (b) [2:]
            if len (atom) == 1:
                atom = '0' + atom        # Prefix single-digits with 0
            rep.append (atom)
        return ':'.join (rep)


# pylint: disable=invalid-name, too-few-public-methods
class L2net_eth (object):
    """
    This class represents a L2 Ethernet network connection.
    """
    #
    # Constants (Python makes them class variables)
    #
    MTU = 1536
    ETHTYPE = 0x88b5
    READ_MAX = 2000

    broadcast = L2addr_eth ('ff:ff:ff:ff:ff:ff')

    def __str__ (self):
        """
        Return a string representation of the network object.
        Used for debugging purposes.
        """
        s = 'Network type : Ethernet\n'
        s += 'Interface : ' + self._iface + '\n'
        s += 'Ethertype : ' + hex (self._ethtype) + '\n'
        s += 'MTU : ' + str (self.mtu) + '\n'
        return s

    def __init__ (self):
        """
        Construct a L2net_eth object with some default values.
        """

        # public interface
        self.max_latency = 50
        self.mtu = self.MTU

        # private attributes
        self._iface = None              # Interface name (see netstat -i)
        self._ethtype = None            # Ethernet frame type
        self._fd = None                 # Socket descriptor

    def init (self, iface, mtu, ethtype):
        """
        Initialize a L2net_eth object, opens and sets up the network interface
        :param iface: interface to start (e.g. 'eth0').
        :type  iface: string
        :param mtu: MTU for interface iface. If not valid, will be set to a
                    default value. 0 means default value.
        :type  mtu: integer
        :param ethtype: Ethernet type (e.g. 0x88b5). 0 means default value.
        :type  ethtype: integer
        :return: True if ok, False if error.
        """

        self._iface = iface

        if mtu == 0:
            mtu = self.MTU
        self.mtu = mtu

        if ethtype == 0:
            ethtype = self.ETHTYPE
        self._ethtype = ethtype

        e = socket.ntohs (ethtype)

        self._fd = socket.socket (socket.AF_PACKET, socket.SOCK_DGRAM, e)
        self._fd.bind ((iface, ethtype))

    def term (self):
        """
        Close the connection
        """
        self._fd.close ()
        self._fd = None

    def handle (self):
        """
        Return file handle, needed for asyncio
        :return: file handle
        """
        return self._fd

    def send (self, dest, data):
        """
        Send a frame on the network.
        :param dest: destination address
        :type  dest: L2eth_addr
        :param data: Ethernet payload
        :type  data: bytes
        :return: True if success, False either.
        """
        r = False
        dlen = len (data)
        if dlen <= self.mtu:
            #
            # Ethernet specific: add message length at the beginning of
            # the payload
            #
            dlen += 2
            b = bytes ((dlen & 0xff00) >> 8, dlen & 0x00ff)
            data = b + data
            addr = (self._iface, self._ethtype, 0, dest.addr)
            nbytes = self._fd.sendto (data, addr)
            if nbytes > 0:
                r = True

        return r

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
        (data, addr) = self._fd.recvfrom (self.READ_MAX)
        # (iface, ethtype, pktype, hatype, haaddr) = addr
        (_, _, pktype, _, haaddr) = addr

        if pktype == socket.PACKET_HOST:
            ptype = 'me'
        elif pktype == socket.PACKET_BROADCAST:
            ptype = 'bcast'
        elif pktype == socket.PACKET_MULTICAST:
            ptype = 'bcast'
        else:
            return None

        # get source address: haaddr is a byte array
        a = L2addr_eth ()
        a.addr = haaddr

        return (ptype, a, data)
