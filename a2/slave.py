"""
This module contains the Slave class
"""

import datetime
import re

import msg
import resource


class Slave:
    """
    This class describes a slave in the CASAN system.
    """
    class Status:
        """
        Possible status codes for slaves
        """
        INACTIVE = 0
        RUNNING = 1

    # XXX
    def __init__(self, sid, ttl, mtu):
        """
        Default constructor
        """

        self.sid = sid
        self.ttl = ttl
        self.defmtu = mtu

        self.curmtu = 0
        self.l2n = None
        self.addr = None
        self.status = self.Status.INACTIVE
        self.reslist = []
        self.next_timeout = datetime.datetime.now ()

        # Value obtained from the last Discover message
        self._disc_curmtu = 0

    def __str__(self):
        """
        Return a string describing the slave status and resources
        """

        l1 = 'Slave ' + str (self.sid) + ' '
        if self.status == self.Status.INACTIVE:
            l2 = 'INACTIVE'
        elif self.status == self.Status.RUNNING:
            l2 = 'RUNNING (curmtu=' + str (self.curmtu)
            l2 += ', ttl= ' + self.next_timeout.isoformat () + ')'
        else:
            l2 = '(unknown state)'
        l3 = ' mac=' + str (self.addr) + '\n'
        res = ''
        for r in self.reslist:
            res += str (r) + '\n'
        return l1 + l2 + l3 + '\t' + res

    def html (self):
        """
        Return an string describing the slave status and resources
        """

        l1 = 'Slave ' + str (self.sid) + ' '
        if self.status == self.Status.INACTIVE:
            l2 = '<b>inactive</b>'
        elif self.status == self.Status.RUNNING:
            l2 = '<b>running</b> (curmtu=' + str (self.curmtu)
            l2 += ', ttl= ' + self.next_timeout.isoformat () + ')'
        else:
            l2 = '<b>(unknown state)</>'
        l3 = ' mac=' + str (self.addr) + '\n'
        res = '<ul>'
        for r in self.reslist:
            res += '<li>' + r.html () + '</li>'
        res += '</ul>'
        return l1 + l2 + l3 + res

    def reset (self):
        """
        Resets the slave to its default state
        """

        self.addr = None
        self.l2n = None
        self.reslist.clear ()
        self.curmtu = 0
        self.status = self.Status.INACTIVE
        self.next_timeout = datetime.datetime.now ()
        print ('Slave' + str (self.sid) + ' status set to INACTIVE.')

    ##########################################################################
    # Initialize values from a Discover message
    ##########################################################################

    def reset_discover (self, l2n, addr, mtu):
        """
        Prepare
        :
        """

        if self.addr is not None and self.addr != addr:
            print ('Discover from slave ' + str (self.sid)
                   + ': address move from ' + str (self.addr)
                   + ' to ' + str (addr))

        if self.addr is None:
            self.addr = addr
        if self.l2n is None:
            self.l2n = l2n

        l2mtu = l2n.mtu
        defmtu = self.defmtu if 0 < self.defmtu <= l2mtu else l2mtu
        self._disc_curmtu = mtu if 0 < mtu <= defmtu else defmtu

    ##########################################################################
    # Process control message for this slave
    ##########################################################################

    def process_casan (self, m):
        """
        Process a received control message for this slave
        :param m: received message
        :type  m: class Msg
        """

        cat = m.category ()
        if cat == msg.Msg.Categories.DISCOVER:
            # XXX one more call to is_casan_discover
            (sid, mtu) = m.is_casan_discover ()
            self.reset_discover (m.l2n, m.peer, mtu)
            massoc = msg.Msg ()
            massoc.mk_assoc (m.peer, self.ttl, self._disc_curmtu)
            r = massoc.send (m.l2n)
            if not r:
                print ('Error while sending AssocRequest message')

        elif cat == msg.Msg.Categories.ASSOC_REQUEST:
            print ('Received AssocRequest from another master')

        elif cat == msg.Msg.Categories.ASSOC_ANSWER:
            if m.req_rep is not None:
                print ('Received AssocAnswer for slave.')
                if self.parse_resource_list (m.payload):
                    # XXX check address or L2 move
                    self.addr = m.peer
                    self.l2n = m.l2n
                    self.curmtu = self._disc_curmtu
                    self.status = self.Status.RUNNING
                    self.next_timeout = datetime.datetime.now () + datetime.timedelta (seconds=self.ttl)
                    print ('Slave ' + str (self.sid) + ' status set to RUNNING')
                else:
                    print ('Slave ' + str (self.sid) + ' cannot parse resource list.')

        elif cat == msg.Msg.Categories.HELLO:
            print ('Received HELLO from another master')

        else:
            print ('Received an unrecognized message')

    ##########################################################################
    # Management of resources advertised by the slave
    ##########################################################################

    def find_resource (self, res):
        """
        Look for a given resource on the slave.
        This function expects the resource to be an iterable object
        containing the resource path, e.g for resource '/a/b/c' : ['a','b','c']
        If the resource is not found, returns None
        :param res: iterable object containing the resource path.
        :return: resource object or None
        """

        for r in self.reslist:
            if r == res:
                return r
        return None

    def resource_list (self):
        """
        Return the list of all the resources managed by the slave
        """

        return self.reslist

    def parse_resource_list (self, payload):
        """
        Parse the resource list advertised by a slave according to RFC 6690
        :param payload: payload of a CASAN Assoc-Answer message.
        :type  payload: bytearray
        :return: True (decoding successful) or False
        """

        self.reslist = []
        p = payload.decode (encoding='utf-8')

        for r in p.split (','):
            lvl = r.split (';')
            linkval = lvl [0]
            match = re.match ('<([^>]+)>', linkval)
            if not match:
                return False
            uri = match.group (1)

            res = resource.Resource (uri)

            for linkparam in lvl [1:]:
                match = re.match ('^([^=]+)=(.*)', linkparam)
                if not match:
                    return False
                key = match.group (1)
                val = match.group (2)

                res.add_attribute (key, val)

            self.reslist.append (res)

        return True
