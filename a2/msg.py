"""
This module contains the Msg class and a few helper functions
"""

from datetime import datetime, timedelta
import random
import sys
# import asyncio

# from util.debug import print_debug, dbg_levels

import option


glob_msg_id = random.randrange (0xFFFF)

# CASAN related constants
CASAN_VERSION = 1
CASAN_NAMESPACE1 = '.well-known'
CASAN_NAMESPACE2 = 'casan'
casan_namespace = (CASAN_NAMESPACE1, CASAN_NAMESPACE2)

# CoAP Related constants
# Note : all times are expressed in seconds unless explicitly specified.
ACK_RANDOM_FACTOR = 1.5
ACK_TIMEOUT = 1
COAP_MAX_TOKLEN = 8
MAX_RETRANSMIT = 4
MAX_TRANSMIT_SPAN = ACK_TIMEOUT * ((1 >> MAX_RETRANSMIT) - 1) * ACK_RANDOM_FACTOR
PROCESSING_DELAY = ACK_TIMEOUT


# Some helper functions that don't really belong to the msg class
def exchange_lifetime (max_lat):
    return MAX_TRANSMIT_SPAN + (2 * max_lat) + PROCESSING_DELAY


def max_rtt (max_lat):
    return 2 * max_lat + PROCESSING_DELAY


class Msg:
    """
    An object of class Msg represents a message,
    either received or to be sent

    This class represents messages to be sent to the network, or
    received from the network.

    Message attributes are tied to CoAP specification: a message has
    a type (CON, NON, ACK, RST), a code (GET, POST, PUT, DELETE, or a numeric
    value for an answer in an ACK), an Id, a token, some options and
    a payload.

    In order to be sent to the network, a message is transparently
    encoded (by the `send` method) according to CoAP specification.
    Similarly, a message is transparently decoded (by the `recv`
    method) upon reception according to the CoAP specification.

    The binary (encoded) form is kept in the message private variable.
    However, each modification of an attribute will squeeze the
    binary form, which will be automatically re-created if needed.

    Each request sent has a "waiter" which represents a thread to
    awake when the answer will be received.

    @bug should wake the waiter if a request is abandoned due to a time-out
    """
    class MsgTypes:
        MT_CON = 0
        MT_NON = 1
        MT_ACK = 2
        MT_RST = 3

    class MsgCodes:
        MC_EMPTY = 0
        MC_GET = 1
        MC_POST = 2
        MC_PUT = 3
        MC_DELETE = 4

    class CasanTypes:
        CASAN_NONE = 0
        CASAN_DISCOVER = 1
        CASAN_ASSOC_REQUEST = 2
        CASAN_ASSOC_ANSWER = 3
        CASAN_HELLO = 4
        CASAN_UNKNOWN = 5

    # XXX
    def __init__(self):
        """
        Initialize instance attributes
        """
        self.pktype = None
        self.msgcode = self.MsgCodes.MC_EMPTY
        self.msgtype = None
        self.casan_type = self.CasanTypes.CASAN_UNKNOWN
        self.req_rep = None
        self.peer = None
        self.bmsg = b''
        self.token = b''
        self.payload = b''
        self.ntrans = 0
        self.timeout = timedelta ()
        self.next_timeout = datetime.max
        self.expire = datetime.max
        self.id = 0
        if self.req_rep is not None:
            self.req_rep.req_rep = None
            self.req_rep = None
        self.optlist = []

    def __eq__(self, other):
        """
        Equality test operator. Valid for received messages only.
        """
        return self.bmsg == other.bmsg

    def __str__(self):
        """
        Cast to string, allows printing of packet data.
        """
        expstr = str (self.expire - datetime.now ())
        ntostr = str (self.next_timeout - datetime.now ())
        return ('msg <id=' + str (self.id)
                + ', toklen=' + str (len (self.token))
                + ', paylen=' + str (len (self.payload))
                + ', ntrans=' + str (self.ntrans)
                + ', expire=' + expstr
                + ', next_timeout=' + str (ntostr)
                )

    # XXX
    def _reset (self):
        """
        Resets all the values of the class to their defaults, but keeps
        the message binary data, the payload and the options list.
        """
        self.pktype = None
        self.peer = None
        if self.req_rep is not None:
            self.req_rep.req_rep = None
            self.req_rep = None
        self.bmsg = b''
        self.payload = b''
        self.token = b''
        self.ntrans = 0
        self.timeout = timedelta ()
        self.next_timeout = datetime.max
        self.expire = datetime.max
        self.pktype = None
        #### self.casan_type = self.CasanTypes.CASAN_UNKNOWN
        self.msgtype = None
        self.id = 0
        self.optlist = []

    def decode (self, tp):
        """
        Decode a received message according to the CoAP spec
        :param tp: received message as a tuple (type, srcaddr, data)
              with data is a bytearray (see l2net_*.recv() methods)
        :return: True if the message has been successfuly decoded
        """
        self._reset ()
        (self.pktype, self.peer, self.bmsg) = tp

        if ((self.bmsg [0] >> 6) & 0x03) != CASAN_VERSION:
            return False

        self.msgtype = (self.bmsg [0] >> 4) & 0x03
        toklen = self.bmsg [0] & 0x0f
        self.msgcode = self.bmsg [1]
        self.id = (self.bmsg [2] << 8) | self.bmsg [3]

        i = 4 + toklen
        if toklen > 0:
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
                delta = ((self.bmsg [i] << 8) | self.bmsg [i+1]) + 269
                i += 2
            elif delta == 15:
                success = False
            optnb += delta

            if optlen == 13:
                optlen = self.bmsg [i] + 13
                i += 1
            elif optlen == 14:
                optlen = ((self.bmsg [i] << 8) | self.bmsg [i+1]) + 269
                i += 2
            elif optlen == 15:
                success = False

            if success:
                if optnb in option.Option.optdesc:
                    # print ('OPT opt=' + str (optnb) + ', len=' + str (optlen))
                    (otype, minlen, maxlen) = option.Option.optdesc [optnb]
                    o = None
                    if optlen == 0 or otype == 'empty':
                        o = option.Option (optnb)
                    elif otype == 'opaque':
                        o = option.Option (optnb, self.bmsg [i:i+optlen], optlen)
                    elif otype == 'string':
                        o = option.Option (optnb, self.bmsg [i:i+optlen].decode (encoding='utf-8'))
                    elif otype == 'uint':
                        o = option.Option (optnb, Option.int_from_bytes (self.bmsg [i:i+optlen]))

                    self.optlist.append (o)
                else:
                    print ('Unknown option code: ' + str (optnb))
                    # we continue decoding, through

            i += optlen

        paylen = msglen - i - 1  # Mind the 0xff marker
        if success and paylen > 0:
            if self.bmsg [i] != 0xff:  # Oops
                success = False
            else:
                self.payload = self.bmsg [i+1:]
        else:
            self.payload = b''

        return success

    # XXX
    def send (self):
        """
        Sends a CoAP message on the network.
        """
        r = True
        if self.bmsg is None:
            self.coap_encode ()
        print_debug (dbg_levels.MESSAGE, 'TRANSMIT id={}, ntrans={}'.format (self.id, self.ntrans))
        if not self.peer.l2_net.send (self.peer, self.bmsg):
            sys.stderr.write ('Error during packet transmission.\n')
            r = False
        else:
            if self.msgtype is Msg.MsgTypes.MT_CON:
                if self.ntrans == 0:
                    rand_timeout = random.uniform (0, ACK_RANDOM_FACTOR - 1)
                    n_sec = ACK_TIMEOUT * (rand_timeout + 1)
                    self.timeout = timedelta (seconds = n_sec)
                    self.expire = datetime.now () + timedelta (seconds=exchange_lifetime (self.peer.l2_net.max_latency))
                else:
                    self.timeout *= 2
                self.next_timeout = datetime.now () + self.timeout
                self.ntrans += 1
            elif self.msgtype is Msg.MsgTypes.MT_NON: self.ntrans = MAX_RETRANSMIT
            elif self.msgtype in [Msg.MsgTypes.MT_ACK, Msg.MsgTypes.MT_RST]:
                self.ntrans = MAX_RETRANSMIT
                self.expire = datetime.now () + timedelta (seconds=max_rtt (self.peer.l2_net.max_latency))
            else:
                print_debug (dbg_levels.MESSAGE, 'Error : unknown message type (this shouldn\'t happen, go and fix the code!)')
                r = False
        return r

    def coap_encode (self):
        """
        Encode a message according to CoAP spec
        """
        # Compute message size
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
            msglen += o.optlen

            optlen = o.optlen
            if optlen >= 269:
                msglen += 2
            elif optlen >= 13:
                msglen += 1

            optnb = o.optcode
            msglen += o.optlen

        paylen = len (self.payload)
        if paylen > 0:
            msglen += 1 + paylen

        # Compute an ID
        global glob_msg_id
        if self.id == 0:
            self.id = glob_msg_id
            glob_msg_id = glob_msg_id + 1 if glob_msg_id < 0xffff else 1

        # Build message header and token
        self.bmsg = bytearray (4)
        self.bmsg [0] = (CASAN_VERSION << 6) | (self.msgtype << 4) | toklen
        self.bmsg [1] = self.msgcode
        self.bmsg [2] = (self.id & 0xff00) >> 8
        self.bmsg [3] = self.id & 0xff
        if toklen > 0:
            self.bmsg += self.token

        # Build options
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

            optlen = o.optlen
            if optlen >= 269:
                opthdr [0] |= 0x0e
                opthdr += (optlen - 269).to_bytes (2, 'big')
            elif optlen >= 13:
                opthdr [0] |= 0x0d
                opthdr += (optlen - 13).to_bytes (1, 'big')
            else:
                opthdr [0] |= optlen

            self.bmsg += opthdr

            self.bmsg += o.optval.encode ()

        # Append payload-marker and payload if any
        if len (self.payload) > 0:
            self.bmsg.append (b'\xff')
            self.bmsg += self.payload

        self.ntrans = 0

    # XXX
    def stop_retransmit (self):
        """
        Prevents the message from being retransmitted.
        """
        self.ntrans = MAX_RETRANSMIT

    # XXX
    def max_age (self):
        """
        Looks for the option Max-Age in the option list and returns it's value
        if any, else, returns None.
        """
        for opt in self.optlist:
            if opt.optcode is Option.OptCodes.MO_MAX_AGE:
                return opt.optval
        return None

    # XXX
    def cache_match (self, other):
        """
        Checks whether two messages match for caching.
        See CoAP spec (5.6)
        """
        if self.msgtype is not other.msgtype:
            return False
        else:
            # Sort both option lists
            self.optlist.sort ()
            other.optlist.sort ()
            i, j, imax, jmax = 0, 0, len (self.optlist)-1, len (other.optlist)-1
            while self.optlist [i].is_nocachekey () and i <= imax:
                i += 1
            while other.optlist [j].is_nocachekey () and j <= jmax:
                j += 1
            while True:
                if j == jmax and i == imax:  # Both at the end : success!
                    return True
                elif j == jmax or i == imax:  # One at the end : failure
                    return False
                elif self.optlist [i] == other.optlist [j]:
                    i, j = i + 1, j + 1
                else:  # No match : failure
                    return False

    # XXX
    @staticmethod
    def link_req_rep (m1, m2):
        """
        Mutually link a request message and it's reply.
        """
        if m2 is None:
            if m1.req_rep is not None:
                # I suspect there is a bug in the C++ program here :
                    # if (m1->reqrep_ != nullptr)
                    # m2->reqrep_->reqrep_ = nullptr ;
                # I assumed it should be :
                    # if (m1->reqrep_ != nullptr)
                    # m1->reqrep_->reqrep_ = nullptr ;
                if m1.req_rep.req_rep is not None:
                    m1.req_rep.req_rep = None
            m1.req_rep = None
        else:
            m1.req_rep = m2
            m2.req_rep = m1

    def is_casan_ctrl_msg (self):
        r = True
        i = 0
        nslen = len (casan_namespace)
        for o in self.optlist:
            if o.optcode == option.Option.OptCodes.MO_URI_PATH:
                r = False
                if i >= nslen:
                    break
                if casan_namespace [i] != o.optval:
                    break
                r = True
                i += 1
        if r and (i != nslen):
            r = False
        print ('It\'s ' + ('' if r else 'not ') + 'a control message.')
        return r

    # XXX
    def is_casan_discover (self):
        """
        Checks if a message is a CASAN discover message.
        :return: a tuple whose first field is True if the message is a CASAN discover message, otherwise it is the
                 singleton False.
                 If the message is a CASAN discover, then the second field contains the slave id, and the third field
                 contains the mtu.
        """
        sid, mtu = (0, 0)
        if self.msgcode is Msg.MsgCodes.MC_POST and self.msgtype is Msg.MsgTypes.MT_NON and self.is_casan_ctrl_msg ():
            for opt in self.optlist:
                if opt.optcode is Option.OptCodes.MO_URI_QUERY:
                    if opt.optval.startswith ('slave='):
                        # Expected value : 'slave=<slave ID>'
                        sid = int (opt.optval.partition ('=') [2])
                    elif opt.optval.startswith ('mtu='):
                        mtu = int (opt.optval.partition ('=') [2])
                    else:
                        sid = 0
                        break
        if sid > 0 and mtu >= 0:
            self.casan_type = self.CasanTypes.CASAN_DISCOVER
        r = (self.casan_type is self.CasanTypes.CASAN_DISCOVER,)
        if r [0]:
            r = (r, sid, mtu)
        return r

    def is_casan_associate (self):
        """
        Checks if a message is a CASAN associate message.
        :return: True if the message is a CASAN associate message, False otherwise.
        """
        found = False
        if self.casan_type is self.CasanTypes.CASAN_UNKNOWN:
            if self.msgtype is self.MsgTypes.MT_CON and self.msgcode is self.MsgCodes.MC_POST and self.is_casan_ctrl_msg () == True:
                for opt in self.optlist:
                    if opt.optcode is Option.OptCodes.MO_URI_QUERY:
                        # TODO: maybe use regular expressions here
                        found = True in (opt.optval.startswith (pattern) for pattern in ['mtu=', 'ttl='])
                if found:
                    self.casan_type = self.CasanTypes.CASAN_ASSOC_REQUEST
        return self.casan_type is self.CasanTypes.CASAN_ASSOC_REQUEST

    def find_casan_type (self, check_req_rep=True):
        if self.casan_type is self.CasanTypes.CASAN_UNKNOWN:
            if not self.is_casan_associate () and not self.is_casan_discover () [0]:
                if check_req_rep and self.req_rep is not None:
                    st = self.req_rep.find_casan_type (False)
                    if st is self.CasanTypes.CASAN_ASSOC_REQUEST:
                        self.casan_type = self.CasanTypes.CASAN_ASSOC_ANSWER
            if self.casan_type is self.CasanTypes.CASAN_UNKNOWN:
                self.casan_type = self.CasanTypes.CASAN_NONE
        return self.casan_type

    def add_path_ctrl (self):
        """
        Adds casan_namespace members to message as URI_PATH options.
        """
        for ns in casan_namespace:
            self.optlist.append (option.Option (option.Option.OptCodes.MO_URI_PATH, ns))

    # XXX
    def mk_ctrl_assoc (self, ttl, mtu):
        self.add_path_ctrl ()
        s = 'ttl=' + str (ttl)
        self.optlist.append (option.Option (option.Option.OptCodes.MO_URI_QUERY, s, len (s)))
        s = 'mtu=' + str (mtu)
        self.optlist.append (option.Option (option.Option.OptCodes.MO_URI_QUERY, s, len (s)))

    # XXX
    def mk_ctrl_hello (self, hello_id):
        """
        Builds a hello message.
        :param hello_id:
        """
        self.add_path_ctrl ()
        s = 'hello={}'.format (hello_id)
        self.optlist.append (option.Option (option.Option.OptCodes.MO_URI_QUERY, s, len (s)))
