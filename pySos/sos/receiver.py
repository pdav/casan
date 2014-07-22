"""
This module contains the Receiver class.
"""

from datetime import datetime, timedelta

from util import threads
from util.debug import print_debug, dbg_levels
from .msg import Msg, exchange_lifetime
from .slave import Slave


class Receiver(threads.ThreadBase):
    """
    This class reads data from a L2 network interface and processes it by sending replies to the Sender.
    """
    def __init__(self, sos_instance, net, slavelist):
        super().__init__(name='Receiver<{}>'.format(net.iface))
        self.l2net = net
        self.sos_instance = sos_instance
        self.slavelist = slavelist
        self.next_hello = datetime.now() + timedelta(seconds=1)
        self.deduplist = []
        self.broadcast = Slave()
        self.broadcast.l2_net = net
        self.broadcast.addr = net.broadcast

    def run(self):
        """
        Override of the Thread.run method.
        This function loops reading for data and processes it.
        """
        print_debug(dbg_levels.MESSAGE, 'Receiver for network interface {} lives!'.format(self.l2net.port.name))
        while self.keepRunning:
            m = Msg()
            saddr = m.recv(self.l2net)
            if saddr is None:
                continue  # Message reception timeout
            print_debug(dbg_levels.MESSAGE, 'Received something from {}!'.format(saddr))

            m.expire = datetime.now() + timedelta(milliseconds=exchange_lifetime(self.l2net.max_latency))
            self.clean_deduplist()

            if not self.find_peer(m, saddr):
                print_debug(dbg_levels.MESSAGE, 'Warning : sender not in authorized peers.')
                continue

            # Is the received message a reply to pending request?
            req = self.correlate(m)
            if req is not None:
                # Ignore if a reply was already received
                if req.req_rep is not None:
                    continue
                # This is the first reply : stop message retransmissions, link the reply to the
                # original request, and wake the emitter.
                req.stop_retransmit()
                Msg.link_req_rep(req, m)
                with self.sos_instance.condition_var as cv:
                    cv.notify()
                # If it is not a SOS message, i.e. it doesn't need processing, keep loopin'
                if m.sos_type is Msg.SosTypes.SOS_UNKNOWN:
                    m.find_sos_type()
                if m.sos_type is m.SosTypes.SOS_NONE:
                    continue

            # Was the message already received? If so, ignore it if we already replied.
            dup_msg = self.deduplicate(m)
            if dup_msg is not None:
                if dup_msg.req_rep is not None:
                    continue

            # Process messages.
            # Process SOS control messages first.
            if m.find_sos_type() is not Msg.SosTypes.SOS_NONE:
                m.peer.process_sos(self.sos_instance, m)
                continue

            # Orphaned message
            if m.peer.status is Slave.StatusCodes.SL_RUNNING:
                print_debug(dbg_levels.MESSAGE, 'Orphaned message from ' + str(m.peer.addr) + ', id=' + str(m.id))

        print_debug(dbg_levels.MESSAGE, 'Receiver for network interface {} dies!'.format(self.l2net.port.name))

    def find_peer(self, mess, addr):
        """
        Check if the sender of a message is an authorized peer. If not, check if it is an SOS slave
        coming up.
        :param mess: received message
        :param addr: MAC address of the slave to find.
        :return: True if the sender is an authorized peer or a new SOS slave.
        """
        # Is the peer already known?
        found = False
        for sl in self.slavelist:
            if addr == sl.addr:
                found = True
                mess.peer = sl
                break

        # If not found, it may be a new slave coming up.
        if not found:
            r = mess.is_sos_discover()
            if r[0]:
                with self.sos_instance.sos_lock:
                    sid, mtu = r[1], r[2]
                    for s in self.slavelist:
                        if s.id == sid:
                            s.l2_net = self.l2net
                            s.addr = addr
                            mess.peer = s

                            # MTU negociation
                            l2mtu = self.l2net.mtu
                            defmtu = s.def_mtu if 0 < s.def_mtu <= l2mtu else l2mtu
                            s.curmtu = mtu if 0 < mtu <= defmtu else defmtu
                            found = True
        return found

    def correlate(self, mess):
        """
        Check if a received message is an answer to a sent request.
        :param mess: received message to check
        :return: the correlated message
        """
        corr_req = None
        if mess.msg_type in (Msg.MsgTypes.MT_ACK, Msg.MsgTypes.MT_RST):
            with self.sos_instance.sos_lock:  # May need to be moved above the if
                id_ = mess.id
                for mreq in self.sos_instance.mlist:
                    if mreq.id == id_:
                        print_debug(dbg_levels.MESSAGE, 'Found original request for ID=' + str(id_))
                        corr_req = mreq
                        break
        return corr_req

    def clean_deduplist(self):
        """
        Removes expired messages from deduplication list.
        :return: None
        """
        now = datetime.now()
        self.deduplist = [m for m in self.deduplist if m.expire < now]

    def deduplicate(self, mess):
        """
        Check if a message is a duplicate and if so, returns the original message.
        If a reply was already sent (marked with req_rep), send it again.
        :param mess: message to check
        :return: original message if mess is a duplicate, None otherwise.
        """
        orig_msg = None
        mt = mess.msg_type
        if mt in (Msg.MsgTypes.MT_CON, Msg.MsgTypes.MT_NON):
            for d in self.deduplist:
                if mess == d:
                    orig_msg = d
                    break
            if orig_msg is not None:
                if orig_msg.req_rep is not None:
                    print_debug(dbg_levels.MESSAGE, 'Duplicate message : id=' + orig_msg.id)
                    orig_msg.req_rep.send()
                else:
                    mess.expire = datetime.now() + timedelta(milliseconds=exchange_lifetime(self.l2net.max_latency))
        self.deduplist.append(mess)
        return orig_msg
