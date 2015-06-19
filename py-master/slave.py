"""
This module contains the Slave class
"""

import datetime
import re

import msg
import resource
import g


# pylint: disable=too-many-instance-attributes
class Slave (object):
    """
    This class describes a slave in the CASAN system.
    """
    class Status:
        """
        Possible status codes for slaves
        """
        INACTIVE = 0
        RUNNING = 1

    def __init__(self, loop, sid, ttl, mtu):
        """
        Default constructor
        """

        # Public attributes
        self.sid = sid                      # Slave id
        self.ttl = ttl                      # Slave TTL
        self.defmtu = mtu                   # Slave default MTU

        self.curmtu = 0                     # Negociated MTU
        self.l2n = None                     # L2 network
        self.addr = None                    # Address
        self.status = self.Status.INACTIVE  # Current status
        self.reslist = []                   # Resource list

        # Private attributes
        self._disc_curmtu = 0               # Advertised by slave (Discov msg)
        self._loop = loop                   # Asyncio loop
        self._timeout = None                # Association timeout
        self._observers = []                # Observers

    def __str__(self):
        """
        Return a string describing the slave status and resources
        """

        l1 = 'Slave ' + str (self.sid) + ' '
        if self.status == self.Status.INACTIVE:
            l2 = 'INACTIVE'
        elif self.status == self.Status.RUNNING:
            l2 = 'RUNNING (curmtu=' + str (self.curmtu)
            l2 += ', ttl= ' + self._timeout.isoformat () + ')'
        else:
            l2 = '(unknown state)'
        l3 = ' mac=' + str (self.addr) + '\n'
        res = ''
        for r in self.reslist:
            res += str (r) + '\n'
        obs = 'Observers'
        for o in self._observers:
            obs += '\n' + str (o)
        return l1 + l2 + l3 + '\t' + res + '\n' + obs

    def html (self):
        """
        Return an string describing the slave status and resources
        """

        l1 = 'Slave ' + str (self.sid) + ' '
        if self.status == self.Status.INACTIVE:
            l2 = '<b>inactive</b>'
        elif self.status == self.Status.RUNNING:
            l2 = '<b>running</b> (curmtu=' + str (self.curmtu)
            l2 += ', ttl= ' + self._timeout.isoformat () + ')'
        else:
            l2 = '<b>(unknown state)</>'
        l3 = ' mac=' + str (self.addr) + '\n'
        res = '</br>Resources: <ul>'
        for r in self.reslist:
            res += '<li>' + r.html () + '</li>'
        res += '</ul>'
        obs = 'Observers: <ul>'
        for o in self._observers:
            obs += '<li>' + o.html () + '</li>'
        obs += '</ul>'
        return l1 + l2 + l3 + res + obs

    def reset (self):
        """
        Resets the slave to its default state
        """

        self.addr = None
        self.l2n = None
        self.reslist.clear ()
        self.curmtu = 0
        self.status = self.Status.INACTIVE
        self._timeout = None
        g.d.m ('slave', 'Slave {}: status set to INACTIVE'.format (self.sid))
        g.e.add ('master', 'slave {} status set to INACTIVE'.format (self.sid))

    ##########################################################################
    # Various utilities
    ##########################################################################

    def isrunning (self):
        return self.status == self.Status.RUNNING

    ##########################################################################
    # Initialize values from a Discover message
    ##########################################################################

    def reset_discover (self, l2n, addr, mtu):
        """
        When we receive the Discover message, prepare the slave status
        for further negociation
        :param l2n: the network we received the message on
        :param addr: source address of the message (future slave address)
        :param mtu: mtu advertised by the slave
        """

        if self.addr is not None and self.addr != addr:
            g.d.m ('slave', 'Discover from slave {}: address move from '
                            '{} to {}'.format (self.sid, self.addr, addr))
            g.e.add ('master', 'discover from slave {}: address move from '
                               '{} to {}'.format (self.sid, self.addr, addr))

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
            (_, mtu) = m.is_casan_discover ()
            self.reset_discover (m.l2n, m.peer, mtu)
            massoc = msg.Msg ()
            massoc.mk_assoc (m.l2n, m.peer, self.ttl, self._disc_curmtu)
            r = massoc.send ()
            if not r:
                g.d.m ('slave', 'Error while sending AssocRequest message')

        elif cat == msg.Msg.Categories.ASSOC_REQUEST:
            g.d.m ('slave', 'Received AssocRequest from another master')
            g.e.add ('master', 'received a AssocRequest message from another master (address {})'.format (m.peer))

        elif cat == msg.Msg.Categories.ASSOC_ANSWER:
            if m.req_rep is not None:
                g.d.m ('slave', 'Received AssocAnswer for slave.')
                g.e.add ('master', 'received AssocAnswer for slave {}'.format (self.sid))
                if self.parse_resource_list (m.payload):
                    # XXX check address or L2 move
                    self.addr = m.peer
                    self.l2n = m.l2n
                    self.curmtu = self._disc_curmtu
                    self.status = self.Status.RUNNING

                    #
                    # Association timer: arrange for _slave_timeout ()
                    # to be called when the timer will expire
                    #
                    now = datetime.datetime.now ()
                    self._timeout = now + datetime.timedelta (seconds=self.ttl)
                    self._loop.call_later (self.ttl, self._slave_timeout)
                    g.d.m ('slave', 'Slave {}: status set to '
                                    'RUNNING'.format (self.sid))
                    g.e.add ('master', 'slave {} status set to RUNNING'.format (self.sid))

                    #
                    # Start observers, if any
                    #
                    self.start_all_observers ()

                else:
                    g.d.m ('slave', 'Slave {}: cannot parse '
                                    'resource list'.format (self.sid))
                    g.e.add ('master', 'unparsable resource list from slave {}'.format (self.sid))

        elif cat == msg.Msg.Categories.HELLO:
            g.d.m ('slave', 'Received Hello from another master')
            g.e.add ('master', 'received a Hello message from another master (address {})'.format (m.peer))

        else:
            g.d.m ('slave', 'Received an unrecognized message')
            g.e.add ('master', 'unrecognized message from address {}'.format (m.peer))

    ##########################################################################
    # Association timeout
    ##########################################################################

    def _slave_timeout (self):
        """
        Process the association timeout for the slave. Called by
        asyncio loop when the timer expires. If the slave has
        re-associated more recently, the current timeout value
        is updated, and this call will be ignored.
        """
        if self.status == self.Status.RUNNING and self._timeout is not None:
            now = datetime.datetime.now ()
            if now >= self._timeout:
                self.reset ()

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
                if match:
                    key = match.group (1)
                    val = match.group (2)
                    res.add_attribute (key, val)
                else:
                    match = re.match ('^([^=]+)$', linkparam)
                    if match:
                        key = match.group (1)
                        res.add_attribute (key, None)
                    else:
                        return False

            self.reslist.append (res)

        return True

    ##########################################################################
    # Observation management
    ##########################################################################

    def find_observer (self, res, token):
        """
        Check if an observer is registered for the sid and the resource.
        :param res: string or iterable object containing the resource path.
        :type  res: list
        :param token: token used to identify the observer
        :type  token: str
        :return: observer object or None
        """

        o = None
        for ol in self._observers:
            if ol.match (token, res):
                o = ol
                break
        return o

    def add_observer (self, obs):
        """
        Add an observer to the list
        :param obs: observer
        :type  obs: class Observer
        """

        self._observers.append (obs)
        if self.isrunning () and self.find_resource (obs.res) is not None:
            obs.start ()

    def del_observer (self, obs):
        """
        Add an observer to the list
        :param obs: observer
        :type  obs: class Observer
        """

        self._observers.remove (obs)

    def start_all_observers (self):
        """
        Start all observers (with valid resource) registered for
        the slave.
        """

        if not self.isrunning ():
            return

        for o in self._observers:
            if self.find_resource (o.res) is not None:
                o.start ()

    def recv_observe (self, m):
        """
        Process an observe event (message) from this slave.
        :param m: message containing the observe event
        :type  m: class Msg
        """

        # Find the observer responsible for this event
        o = None
        for ol in self._observers:
            if ol.match (m.token):
                o = ol
                break

        if o is None:
            g.d.m ('slave', 'Slave {} emitted an unsollicited observe'.format (self.sid))
            g.e.add ('master', 'slave {} emitted an unsollicited observe'.format (self.sid))
        else:
            o.set_value (m)
