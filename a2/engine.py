"""
This module contains the Engine class.
"""

import random
import datetime
import asyncio

import l2_eth
import l2_154
import msg
import slave


# Seed the RNG
random.seed ()
CASAN_VERSION = 1


class Engine:
    """
    CASAN engine class
    """

    def __init__(self):
        """
        CASAN engine constructor
        Initialize the instance with default values, but does not
        start the CASAN system. See 'start' method.
        """

        self._conf = None
        self._loop = None

        now = datetime.datetime.now ()
        self._hid = int (datetime.datetime.timestamp (now)) % 1000

        self._slaves = []
        self._deduplist = []        # list of received messages

#        self.timer_slave_ttl = 0

    def __str__(self):
        """
        Dump the current engine state into a string
        """
        s = ''
        #### s += 'Default TTL = ' + str (self.timer_slave_ttl) + '\n'
        s += 'Slaves:\n'
        for sl in self._slaves:
            s += '\t' + str (sl) + '\n'
        return s

    def html (self):
        s = ''
        #### s += 'Default TTL = ' + str (self.timer_slave_ttl) + '\n'
        s += 'Slaves: <ul>'
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
            self._slaves.append (slave.Slave (sid, ttl, mtu))

        #
        # Configure L2 networks
        #

        for (type, dev, mtu, sub) in self._conf.networks:
            if type == 'ethernet':
                (ethertype, ) = sub
                l2n = l2_eth.l2net_eth ()
                r = l2n.init (dev, mtu, ethertype)
            elif type == '802.15.4':
                (subtype, addr, panid, channel) = sub
                l2n = l2_154.l2net_154 ()
                r = l2n.init (dev, subtype, mtu, addr, panid, channel,
                              asyncio=True)
            else:
                raise RuntimeError ('Unknown network type ' + type)

            if r is False:
                raise RuntimeError ('Error in network ' + type + ' init')

            l2n.sentlist = []

            #
            # Packet reception
            #

            self._loop.add_reader (l2n.handle (), lambda: self.l2reader (l2n))

            #
            # Send the periodic hello on this network
            #

            self._loop.call_later (self._conf.timers ['firsthello'],
                                   lambda: self.send_hello (l2n))

    ######################################################################
    # Hello timer handle: send the periodic Hello packet
    ######################################################################

    def send_hello (self, l2n):
        mhello = msg.Msg ()
        mhello.mk_hello (l2n, self._hid)
        mhello.coap_encode ()
        mhello.send (l2n)
        print ('Sent Hello')
        self._loop.call_later (self._conf.timers ['hello'],
                               lambda: self.send_hello (l2n))

    ######################################################################
    # L2 handle routines: receive a packet on the given network interface
    ######################################################################

    # @asyncio.coroutine
    def l2reader (self, l2n):
        """
        :param l2n: network to read on
        :type  l2n: l2net_* (l2net_eth or l2net_154)
        :return: nothing
        """

        m = msg.Msg ()
        if not m.recv (l2n):
            return

        # print (str(msg))

        m.refresh_expiration ()

        # Remove obsolete messages from the deduplication list
        self.clean_deduplist ()

        # Is the slave a valid peer? Either an already associated slave
        # or a slave, cited in the configuration file, trying to associate
        sl = self.find_peer (m)
        if not sl:
            print ('Warning : sender ' + str (m.peer) + ' not authorized')
            return

        #D print ('SL found = ', str (sl))

        #
        # Is the received message a reply to pending request?
        #

        req = self.correlate (m)
        if req is not None:
            # Ignore the message if a reply was already received
            if req.req_rep is not None:
                return

            # This is the first reply we get: stop further message
            # retransmissions, link this reply to the original
            # request, and wake-up the coroutines waiting for
            # this answer to the original request
            req.stop_retransmit ()
            req.link_req_rep (m)
            req.wakeup ()

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

        mdup = self.deduplicate (m)
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

    def init_slaves (self):
        """
        Initialize the slave list from the configuration file
        """

        for (sid, ttl, mtu) in self._conf.slaves:
            sl = slave.Slave (sid, ttl, mtu)
            self._slaves.append (sl)

    def find_peer (self, m):
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
                (sid, mtu) = r
                for sl in self._slaves:
                    if sl.sid == sid:
                        s = sl
                        break

        return s

    ######################################################################
    # Utilities
    ######################################################################

    def clean_deduplist (self):
        """
        Remove expired messages from deduplication list
        :return: None
        """

        now = datetime.datetime.now ()
        self._deduplist = [m for m in self._deduplist if m.expire < now]

    def deduplicate (self, m):
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
            print ('Duplicate message : id=' + omsg.id)
            if omsg.req_rep is not None:
                # XXX HOW TO RESEND A MSG?
                omsg.req_rep.send ()

        return omsg

    def correlate (self, m):
        """
        Check if a received message is an answer to a sent request
        :param m: received message to check
        :type  m: class Msg
        :return: either None or the correlated message
        """

        r = None
        if m.msgtype in (msg.Msg.Types.ACK, msg.Msg.Types.RST):
            i = m.id
            for mreq in m.l2n.sentlist:
                if mreq.id == i:
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
            if sl.status == Slave.Status.RUNNING:
                for res in sl.res_list:
                    s += str (res)
        return s
