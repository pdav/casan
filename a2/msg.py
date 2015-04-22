"""
This module contains the Msg class and a few helper functions
"""

import datetime
import random
import sys

import asyncio

import option

# CASAN related constants
CASAN_VERSION = 1
NAMESPACE = ('.well-known', 'casan')

# CoAP Related constants
# Note : all times are expressed in seconds unless explicitly specified.
ACK_RANDOM_FACTOR = 1.5
ACK_TIMEOUT = 1
COAP_MAX_TOKLEN = 8
MAX_RETRANSMIT = 4
MAX_XMIT_SPAN = ACK_TIMEOUT * ((1 >> MAX_RETRANSMIT) - 1) * ACK_RANDOM_FACTOR
PROCESSING_DELAY = ACK_TIMEOUT


class Msg (object):
    """
    An object of class Msg represents a message, either received
    or to be sent

    Message attributes are tied to CoAP specification: a message has
      * a type (CON, NON, ACK, RST)
      * a code (GET, POST, PUT, DELETE, or a numeric value for an
        answer in an ACK)
      * an Id
      * a token
      * some options
      * and a payload

    In order to be sent to the network, a message is transparently
    encoded (by the `send` method) according to CoAP specification.
    Similarly, a message is transparently decoded (by the `recv`
    method) upon reception according to the CoAP specification.

    The binary (encoded) form is kept in a message private attribute.
    However, the binary form must be recreated (via coap_encode) if any
    attribute is modified.

    Each request sent has a "waiter" which represents a thread to
    awake when the answer will be received.

    @bug should wake the waiter if a request is abandoned due to a time-out
    """

    class Types:
        """
        CoAP message types.
        """
        CON = 0
        NON = 1
        ACK = 2
        RST = 3

    class Codes:
        """
        CoAP message codes.
        This class does not include numeric values for answer codes.
        """
        EMPTY = 0
        GET = 1
        POST = 2
        PUT = 3
        DELETE = 4

    class Categories:
        """
        Casan message categories:
        - UNKNOWN: message not yet analyzed
        - NONE: normal message (i.e. not a control message)
        - remaining values: control messages
        """
        UNKNOWN = 0
        NONE = 1
        DISCOVER = 2
        ASSOC_REQUEST = 3
        ASSOC_ANSWER = 4
        HELLO = 5

    # Class variable: choose an initial message-id value
    gmid = random.randrange (0xffff)

    # XXX
    def __init__ (self):
        """
        Initialize instance attributes.
        """

        # Public attributes
        self.pktype = None                  # Recvd msg sent to 'bcast' or 'me'
        self.msgcode = self.Codes.EMPTY     # See Codes above
        self.msgtype = None                 # See Types above
        self.peer = None                    # L2 address
        self.bmsg = None                    # Binary encoded message
        self.token = b''                    # Token
        self.payload = b''                  # Payload
        self.expire = datetime.datetime.max  # Expiration date in l2n.sentlist
        self.mid = 0                        # Message ID
        self.l2n = None                     # L2 network
        self.req_rep = None                 # Reply for this request
        self.optlist = []                   # List of options

        # Private attributes
        self._msgcateg = self.Categories.UNKNOWN    # See Categories above
        self._event = None                  # asyncio.Event for synchronisation
        self._ntrans = 0                    # Number of retransmissions
        self._timeout = None                # Time to next retransmission

    def __eq__ (self, other):
        """
        Equality test operator. Valid for received messages only.
        """
        return self.bmsg == other.bmsg

    def __str__ (self):
        """
        Cast to string, allows printing of packet data.
        """
        return ('msg <mid=' + str (self.mid)
                + ', token=' + str (len (self.token))
                + ', payload=' + str (len (self.payload))
                + ', ntrans=' + str (self._ntrans)
                + ', timeout=' + str (self._timeout)
                + ', expire=' + str (self.expire - datetime.datetime.now ())
                + '>'
               )

    def html (self):
        """
        Make an HTML string
        """
        return ('mid=' + str (self.mid)
                + ', token=' + str (len (self.token))
                + ', payload=' + str (len (self.payload))
                + ', ntrans=' + str (self._ntrans)
                + ', timeout=' + str (self._timeout)
                + ', expire=' + str (self.expire - datetime.datetime.now ())
                + ''
               )

    ##########################################################################
    # Send and receive messages
    ##########################################################################

    def recv (self, l2n=None):
        """
        Receive a message on the given network
        :param l2n: network (default: network already associated with the msg)
        :type  l2n: class L2net_*
        :return: True (message received) or False (not received or not decoded)
        """

        if l2n is not None:
            self.l2n = l2n

        tp = self.l2n.recv ()
        if tp is None:
            return False

        (self.pktype, self.peer, self.bmsg) = tp
        r = self.coap_decode ()

        return r

    def send (self):
        """
        Send an individual CoAP message (destination address and
        destination networks are specified in the message)
        :return: True (message sent) or False (error)
        """

        if self.bmsg is None:
            self.coap_encode ()

        print ('TRANSMIT id={}, ntrans={}'.format (self.mid, self._ntrans))
        if not self.l2n.send (self.peer, self.bmsg):
            sys.stderr.write ('Error during packet transmission.\n')
            return False

        self.refresh_expiration ()
        self.l2n.sentlist.append (self)

        return True

    def coap_decode (self):
        """
        Decode a received message according to the CoAP spec
        :return: True if the message has been successfuly decoded
        """

        if ((self.bmsg [0] >> 6) & 0x03) != CASAN_VERSION:
            return False

        self.msgtype = (self.bmsg [0] >> 4) & 0x03
        toklen = self.bmsg [0] & 0x0f
        self.msgcode = self.bmsg [1]
        self.mid = (self.bmsg [2] << 8) | self.bmsg [3]

        i = 4 + toklen
        if toklen == 0:
            self.token = b''
        else:
            self.token = self.bmsg [4:i]

        #
        # Option analysis
        #

        success = True
        optnb = 0
        msglen = len (self.bmsg)

        while i < msglen and success and self.bmsg [i] != 0xff:
            delta = self.bmsg [i] >> 4
            optlen = self.bmsg [i] & 0x0f
            i += 1

            # Handle special values for optdelta/optlen
            if delta == 13:
                delta = self.bmsg [i] + 13
                i += 1
            elif delta == 14:
                delta = ((self.bmsg [i] << 8) | self.bmsg [i + 1]) + 269
                i += 2
            elif delta == 15:
                success = False
            optnb += delta

            if optlen == 13:
                optlen = self.bmsg [i] + 13
                i += 1
            elif optlen == 14:
                optlen = ((self.bmsg [i] << 8) | self.bmsg [i + 1]) + 269
                i += 2
            elif optlen == 15:
                success = False

            if success:
                if optnb in option.Option.optdesc:
                    #D print ('OPT o=' + str (optnb) + ', len=' + str (optlen))
                    o = option.Option (optnb, optbin=self.bmsg [i:i + optlen])
                    self.optlist.append (o)
                else:
                    print ('Unknown option code: ' + str (optnb))
                    # we continue decoding, through

            i += optlen

        paylen = msglen - i - 1         # Mind the 0xff marker
        if success and paylen > 0:
            if self.bmsg [i] != 0xff:   # Oops
                success = False
            else:
                self.payload = self.bmsg [i + 1:]
        else:
            self.payload = b''

        return success

    def coap_encode (self):
        """
        Encode a message according to CoAP spec
        """

        #
        # Compute message size
        #

        toklen = len (self.token)
        msglen = 4 + toklen

        self.optlist.sort ()
        optnb = 0

        for o in self.optlist:
            msglen += 1
            delta = o.optcode - optnb
            if delta >= 269:
                msglen += 2
            elif delta >= 13:
                msglen += 1

            optlen = o.len ()

            if optlen >= 269:
                msglen += 2
            elif optlen >= 13:
                msglen += 1

            optnb = o.optcode
            msglen += optlen

        paylen = len (self.payload)
        if paylen > 0:
            msglen += 1 + paylen

        if msglen > self.l2n.mtu:
            raise RuntimeError ('PACKET TOO LARGE')

        #
        # get a suitable ID
        #

        if self.mid == 0:
            self.mid = Msg.gmid
            Msg.gmid = Msg.gmid + 1 if Msg.gmid < 0xffff else 1

        #
        # Build message header and token
        #

        self.bmsg = bytearray (4)
        self.bmsg [0] = (CASAN_VERSION << 6) | (self.msgtype << 4) | toklen
        self.bmsg [1] = self.msgcode
        self.bmsg [2] = (self.mid & 0xff00) >> 8
        self.bmsg [3] = self.mid & 0xff
        if toklen > 0:
            self.bmsg += self.token

        #
        # Build options
        #

        optnb = 0
        for o in self.optlist:
            opthdr = bytearray (1)

            delta = o.optcode - optnb
            if delta >= 269:
                opthdr [0] |= 0xe0
                opthdr += (delta - 269).to_bytes (2, 'big')
            elif delta >= 13:
                opthdr [0] |= 0xd0
                opthdr += (delta - 13).to_bytes (1, 'big')
            else:
                opthdr [0] |= delta << 4
            optnb = o.optcode

            optlen = o.len ()
            if optlen >= 269:
                opthdr [0] |= 0x0e
                opthdr += (optlen - 269).to_bytes (2, 'big')
            elif optlen >= 13:
                opthdr [0] |= 0x0d
                opthdr += (optlen - 13).to_bytes (1, 'big')
            else:
                opthdr [0] |= optlen

            self.bmsg += opthdr
            self.bmsg += o.optbin

        #
        # Append payload-marker and payload if any
        #

        if len (self.payload) > 0:
            self.bmsg.append (b'\xff')
            self.bmsg += self.payload

        self._ntrans = 0


    ##########################################################################
    # Cache control
    ##########################################################################

    def max_age (self):
        """
        Look for the option Max-Age in the option list and
        returns its value if any, else returns None.
        :return: int or None
        """
        for o in self.optlist:
            if o.optcode == option.Option.Codes.MAX_AGE:
                return o.optval
        return None

    def cache_match (self, m):
        """
        Check if two messages match for caching.
        See CoAP spec (5.6)
        :param m: the other message
        :type  m: class Msg
        :return: True or False
        """

        if self.msgtype != m.msgtype:
            return False

        # Sort both option lists
        self.optlist.sort ()
        m.optlist.sort ()

        i, j = 0, 0
        imax, jmax = len (self.optlist) - 1, len (m.optlist) - 1
        r = True
        finished = False
        while not finished:
            # Skip the NoCacheKey options
            while self.optlist [i].is_nocachekey () and i <= imax:
                i += 1
            while m.optlist [j].is_nocachekey () and j <= jmax:
                j += 1

            # Stop if end of an optlist has been found
            if j == jmax and i == imax:
                # Both at the end: success!
                r = True
                finished = True
            elif j == jmax or i == imax:
                # Only one at the end: failure
                r = False
                finished = True
            elif self.optlist [i] == m.optlist [j]:
                # Match: continue
                i += 1
                j += 1
            else:
                # No match: failure
                r = False
                finished = True

        return r

    ##########################################################################
    # Link between requests and replies
    ##########################################################################

    def link_req_rep (self, m):
        """
        Mutually link a request message and its reply.
        This method can also be used to unlink two messages.
        :param m: the message to link with the current message or None
        :type  m: class Msg
        """
        if m is None:
            # Remove old link if any
            old = self.req_rep
            if old is not None:
                old.req_rep = None
            self.req_rep = None
        else:
            self.req_rep = m
            m.req_rep = self

    ##########################################################################
    # CoAP timers for retransmission control
    ##########################################################################

    def _initial_timeout (self):
        """
        Set the initial timeout for message retransmission
        and return the current value
        :return: timeout value (in seconds, not integral)
        :rtype: float
        """

        self._ntrans = 0
        self._timeout = ACK_TIMEOUT * (random.uniform (1, ACK_RANDOM_FACTOR))
        return self._timeout

    def _next_timeout (self):
        """
        Set the next timeout value, and return the current value.
        Return None if there is no need for further retransmission
        (reasons: answer received, or max number of retransmission
        was reached)
        :return: timeout value (in seconds, not integral) or None
        :rtype: float
        """

        if self._ntrans < MAX_RETRANSMIT:
            self._ntrans += 1
            self._timeout *= 2
        else:
            self._timeout = None

        return self._timeout

    def _stop_retransmit (self):
        """
        Prevent further message retransmissions by setting the
        retransmission count to the maximum value
        """

        self._ntrans = MAX_RETRANSMIT

    ##########################################################################
    # Message retransmission and synchronisation
    ##########################################################################

    @asyncio.coroutine
    def send_request (self):
        """
        Send a request (which should be a CON message) to a CASAN
        slave and wait for an answer or a timeout.
        The request may be retransmitted if needed.
        In order to known if an answer has been received, caller
        may test m.req_rep is not None
        :param m: message to send
        :type  m: class msg
        :return: reply message or None
        :rtype: class Msg
        """

        if self.peer is None:
            raise RuntimeError ('Unknown destination address')
        self.req_rep = None
        self.send ()

        if self.msgtype == Msg.Types.CON:
            t = self._initial_timeout ()
            self._event = asyncio.Event ()

            while t is not None:
                try:
                    yield from asyncio.shield (
                        asyncio.wait_for (self._wait_for_reply (), t))
                    t = None
                except asyncio.TimeoutError:
                    print ('Resend msgid=', self.mid)
                    self.send ()
                    t = self._next_timeout ()
                    print ('TIMEOUT=', t)

        return self.req_rep

    @asyncio.coroutine
    def _wait_for_reply (self):
        """
        Wait until the message gets a reply (via the Engine._l2reader
        method) in which case our "wakeup" method is called.
        """

        yield from self._event.wait ()
        #D print ('Awaken, msgid =', self.mid)

    def got_reply (self, rep):
        """
        The original request got a reply.
        Stop further retransmissions, link the reply to the request
        and wake-up the co-routines which are waiting for the reply
        """

        self._stop_retransmit ()
        self.link_req_rep (rep)
        if self._event is not None:
            self._event.set ()

    ##########################################################################
    # Control message analysis
    ##########################################################################

    def is_casan_ctrl_msg (self):
        """
        Check if a message is a CASAN control message
        :return: True or False
        """

        r = True
        i = 0
        nslen = len (NAMESPACE)
        for o in self.optlist:
            if o.optcode == option.Option.Codes.URI_PATH:
                r = False
                if i >= nslen:
                    break
                if NAMESPACE [i] != o.optval:
                    break
                r = True
                i += 1
        if r and (i != nslen):
            r = False
        #D print ('It\'s ' + ('' if r else 'not ') + 'a control message.')
        return r

    def is_casan_discover (self):
        """
        Check if a message is a CASAN Discover message.
        :return: tuple (slave-id, mtu) or None
        """

        if self.msgcode != Msg.Codes.POST:
            return None
        if self.msgtype != Msg.Types.NON:
            return None
        if not self.is_casan_ctrl_msg ():
            return None

        r = True
        (sid, mtu) = (0, 0)
        for o in self.optlist:
            if o.optcode == option.Option.Codes.URI_QUERY:
                (qname, sep, qval) = o.optval.partition ('=')
                if qname == 'slave':
                    try:
                        sid = int (qval)
                    except ValueError:
                        r = False
                elif o.optval.startswith ('mtu='):
                    try:
                        mtu = int (qval)
                    except ValueError:
                        r = False
                else:
                    # Unrecognized query string ('???=')
                    r = False
                if r == False:
                    break
        if not r or sid == 0 or mtu == 0:
            return None

        self._msgcateg = self.Categories.DISCOVER
        return (sid, mtu)

    def is_casan_associate (self):
        """
        Check if a message is a CASAN AssocRequest message (and
        update the _msgcateg attribute accordingly)
        :return: True or False
        """

        return self._msgcateg == self.Categories.ASSOC_REQUEST

    def category (self, checkreq=True):
        """
        Return the category (see Msg.Categories class) of the message
        :param checkreq: True if linked message must be checked too
        :type  checkreq: bool
        :return: message category
        :rtype: int value from Msg.Categories
        """

        if self._msgcateg == self.Categories.UNKNOWN:
            if self.is_casan_discover () is None:
                if checkreq and self.req_rep is not None:
                    reqcat = self.req_rep.category (False)
                    if reqcat == self.Categories.ASSOC_REQUEST:
                        self._msgcateg = self.Categories.ASSOC_ANSWER

        # If we get here without finding a category, it is a regular message
        if self._msgcateg == self.Categories.UNKNOWN:
            self._msgcateg = self.Categories.NONE

        return self._msgcateg

    def add_path_ctrl (self):
        """
        Adds namespace members to message as URI_PATH options.
        """
        up = option.Option.Codes.URI_PATH
        for n in NAMESPACE:
            self.optlist.append (option.Option (up, optval=n))

    def mk_assoc (self, l2n, addr, ttl, mtu):
        """
        Configure options for an Assoc Request message
        :param l2n: l2 network
        :type  l2n: class L2net_*
        :param addr: address of slave
        :type  addr: class L2addr_*
        :param ttl: ttl
        :type  ttl: int
        :param mtu: negociated mtu
        :type  mtu: int
        """

        self._msgcateg = self.Categories.ASSOC_REQUEST
        self.msgtype = self.Types.CON
        self.msgcode = self.Codes.POST
        self.l2n = l2n
        self.peer = addr
        self.add_path_ctrl ()
        uq = option.Option.Codes.URI_QUERY
        s = 'ttl={}'.format (ttl)
        self.optlist.append (option.Option (uq, optval=s))
        s = 'mtu={}'.format (mtu)
        self.optlist.append (option.Option (uq, optval=s))

    def mk_hello (self, l2n, hid):
        """
        Builds a hello message.
        :param l2n: the network
        :type  l2n: class L2net_*
        :param hid: hello id
        :type  hid: int
        """

        self._msgcateg = self.Categories.HELLO
        self.msgtype = self.Types.NON
        self.msgcode = self.Codes.POST
        self.l2n = l2n
        self.peer = l2n.broadcast
        self.add_path_ctrl ()
        uq = option.Option.Codes.URI_QUERY
        s = 'hello={}'.format (hid)
        self.optlist.append (option.Option (uq, optval=s))

    ##########################################################################
    # Message expiration
    ##########################################################################

    def refresh_expiration (self):
        """
        Set maximum message lifetime for the list of messages sent
        (l2n.sentlist) or the list of messages received
        (engine._deduplist)
        """

        now = datetime.datetime.now ()
        if self.msgtype == Msg.Types.CON:
            d = self._exchange_lifetime ()
            self.expire = now + datetime.timedelta (seconds=d)
        elif self.msgtype == Msg.Types.NON:
            self.expire = now
        elif self.msgtype in [Msg.Types.ACK, Msg.Types.RST]:
            d = self._max_rtt ()
            self.expire = now + datetime.timedelta (seconds=d)
        else:
            self.expire = now

    def _exchange_lifetime (self):
        """
        Return the maximum exchange lifetime (function of l2 network
        maximum latency)
        """
        return MAX_XMIT_SPAN + (2 * self.l2n.max_latency) + PROCESSING_DELAY

    def _max_rtt (self):
        """
        Return the maximum RTT (function of l2 network latency)
        """
        return 2 * self.l2n.max_latency + PROCESSING_DELAY
