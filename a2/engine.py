"""
This module contains the CASAN Engine class.

The CASAN engine is responsible for slave maintenance (status, resource
list) and CASAN message handling.

It is the primary recipient of CASAN messages:
- CASAN control messages are received (Discover and Assoc Answer) and
    slave status is set accordingly
- CASAN normal (data) messages are received, deduplicated, and
    rerouted to consumers (which are normally HTTP handlers)

In addition, the engine is able to send messages to CASAN slaves:
- CASAN Hello messages
- CASAN control messages (Assoc Request), when a CASAN Discover
    message is received
- normal messages on behalf of HTTP handlers
"""

import random
import datetime

import l2_eth
import l2_154
import msg
import slave


# Seed the RNG
random.seed ()
CASAN_VERSION = 1


class Engine (object):
    """
    CASAN engine class
    """

    def __init__(self):
        """
        CASAN engine constructor
        Initialize the instance with default values, but does not
        start the CASAN system. See 'start' method.
        """

        self._conf = None           # current configuration
        self._loop = None           # our asyncio eventloop

        now = datetime.datetime.now ()
        self._hid = int (datetime.datetime.timestamp (now)) % 1000

        self._slaves = []           # slave list
        self._deduplist = []        # list of received messages

    def __str__(self):
        """
        Dump the current engine state into a string
        """
        s = 'Slaves:\n'
        for sl in self._slaves:
            s += '\t' + str (sl) + '\n'
        return s

    def html (self):
        """
        Dump the current engine state into an HTML string
        """
        s = 'Slaves: <ul>'
        for sl in self._slaves:
            s += '<li>' + sl.html () + '</li>'
        s += '</ul>'
        return s

    def start (self, conf, loop):
        """
        Initialize the engine
        :param conf: parsed configuration
        :type  conf: class conf.Conf
        :param loop: main event loop (as returned by asyncio.get_evnt_loop)
        :type  loop: class asyncio.EventLoop XXX
        """

        self._conf = conf
        self._loop = loop

        self._slaves = []
        self._deduplist = []

        #
        # Configure slaves
        #

        for (sid, ttl, mtu) in self._conf.slaves:
            self._slaves.append (slave.Slave (loop, sid, ttl, mtu))

        #
        # Configure L2 networks
        #

        for (nettype, dev, mtu, sub) in self._conf.networks:
            if nettype == 'ethernet':
                (ethertype, ) = sub
                l2n = l2_eth.L2net_eth ()
                r = l2n.init (dev, mtu, ethertype)
            elif nettype == '802.15.4':
                (subtype, addr, panid, channel) = sub
                l2n = l2_154.L2net_154 ()
                r = l2n.init (dev, subtype, mtu, addr, panid, channel,
                              asyncio=True)
            else:
                raise RuntimeError ('Unknown network type ' + nettype)

            if r is False:
                raise RuntimeError ('Error in network ' + nettype + ' init')

            l2n.sentlist = []

            #
            # Packet reception
            #

            self._loop.add_reader (l2n.handle (),
                                   lambda l=l2n: self._l2reader (l))

            #
            # Send the periodic hello on this network
            #

            self._loop.call_later (self._conf.timers ['firsthello'],
                                   lambda l=l2n: self._send_hello (l))

    ######################################################################
    # Hello timer handle: send the periodic Hello packet
    ######################################################################

    def _send_hello (self, l2n):
        """
        Build and send a Casan Hello message.
        While we are here, remove expired messages for this l2 network.
        :param l2n: network to send the message on
        :type  l2n: L2net_*
        """

        #
        # Send a new hello message
        #

        mhello = msg.Msg ()
        mhello.mk_hello (l2n, self._hid)
        mhello.l2n = l2n
        mhello.coap_encode ()
        mhello.send ()

        #
        # Periodically remove expired messages from sentlist
        #

        now = datetime.datetime.now ()
        l2n.sentlist = [m for m in l2n.sentlist if m.expire < now]

        #
        # Schedule next call
        #

        self._loop.call_later (self._conf.timers ['hello'],
                               lambda: self._send_hello (l2n))

    ######################################################################
    # L2 handle routines: receive a packet on the given network interface
    ######################################################################

    # @asyncio.coroutine
    def _l2reader (self, l2n):
        """
        :param l2n: network to read on
        :type  l2n: L2net_* (L2net_eth or L2net_154)
        :return: nothing
        """

        m = msg.Msg ()
        if not m.recv (l2n):
            return

        # print (str(msg))

        m.refresh_expiration ()

        # Remove obsolete messages from the deduplication list
        self._clean_deduplist ()

        # Is the slave a valid peer? Either an already associated slave
        # or a slave, cited in the configuration file, trying to associate
        sl = self._find_peer (m)
        if not sl:
            print ('Warning : sender ' + str (m.peer) + ' not authorized')
            return

        #D print ('SL found = ', str (sl))

        #
        # Is the received message a reply to pending request?
        #

        req = self._correlate (m)
        if req is not None:
            # Ignore the message if a reply was already received
            if req.req_rep is not None:
                return

            # This is the first reply we get: stop further message
            # retransmissions, link this reply to the original
            # request, and wake-up the coroutines waiting for
            # this answer to the original request
            req.got_reply (m)

            # Check category of received message.
            # If it is an AssocAnswer response, process it
            # Else, ignore it (there should be a just-awaken
            # coroutine waiting for this answer)

            cat = m.category ()
            if cat == m.Categories.NONE:
                return

        #
        # Was the message already received?
        #

        mdup = self._deduplicate (m)
        if mdup is not None:
            # If already received and already replied, ignore it
            if mdup.req_rep is not None:
                return

        # Process the received message
        cat = m.category ()
        if cat != m.Categories.NONE:
            #D print ('cat=', cat, ', SL=', str (sl))
            sl.process_casan (m)
        else:
            print ('Ignored message ' + str (m))

        return

    ######################################################################
    # Slave utilities
    ######################################################################

    def _find_peer (self, m):
        """
        Check if the sender of a message is an authorized peer.
        If not, check if it is an CASAN slave coming up.
        :param m: received message
        :type  m: class Msg
        :return: found slave (class Slave) or None
        """

        # Is the peer already known?
        s = None
        for sl in self._slaves:
            if m.peer == sl.addr:
                s = sl
                break

        if s is None:
            # If not found, it may be a new slave coming up
            r = m.is_casan_discover ()
            if r is not None:
                sid = r [0]
                s = self.find_slave (sid)

        return s

    def find_slave (self, sid):
        """
        Find a slave by its id
        :param sid: sid
        :type  sid: int
        :return: found slave (class Slave) or None
        """

        s = None
        for sl in self._slaves:
            if sl.sid == sid:
                s = sl
                break
        return s

    ######################################################################
    # Utilities
    ######################################################################

    def _clean_deduplist (self):
        """
        Remove expired messages from deduplication list
        :return: None
        """

        now = datetime.datetime.now ()
        self._deduplist = [m for m in self._deduplist if m.expire < now]

    def _deduplicate (self, m):
        """
        Check if a message is a duplicate and if so, return the
        original message.  If a reply was already sent (marked
        with req_rep), send it again.
        :param m: message to check
        :type  m: class Msg
        :return: original message if m is a duplicate, None otherwise
        """

        if m.msgtype not in (msg.Msg.Types.CON, msg.Msg.Types.NON):
            return None

        omsg = None
        for d in self._deduplist:
            if m == d:
                omsg = d
                break

        if omsg is None:
            m.refresh_expiration ()
            self._deduplist.append (m)
        else:
            print ('Duplicate message : id=' + str (omsg.mid))
            if omsg.req_rep is not None:
                omsg.req_rep.send ()

        return omsg

    def _correlate (self, m):
        """
        Check if a received message is an answer to a sent request
        :param m: received message to check
        :type  m: class Msg
        :return: either None or the correlated message
        """

        r = None
        if m.msgtype in (msg.Msg.Types.ACK, msg.Msg.Types.RST):
            i = m.mid
            for mreq in m.l2n.sentlist:
                if mreq.mid == i:
                    print ('Found original request for ID=' + str (i))
                    r = mreq
                    break
        return r

    ######################################################################
    # Slave and resource management
    ######################################################################

    def resource_list (self):
        """
        Returns aggregated /.well-known/casan for all running slaves
        :return: the /.well-known/casan resource list as a string
        :rtype: str
        """

        s = ''
        for sl in self._slaves:
            if sl.status == slave.Slave.Status.RUNNING:
                for res in sl.reslist:
                    s += str (res)
        return s
