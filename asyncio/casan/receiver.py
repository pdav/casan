"""
This module contains the Receiver class.
"""

from datetime import datetime, timedelta
import asyncio

from util.debug import print_debug, dbg_levels
from .msg import Msg, exchange_lifetime
from .slave import Slave


class Receiver():
    """
    This class reads data from a L2 network interface and processes it by sending replies to the Sender.
    """
    def __init__(self, casan_instance, net, slavelist, loop):
        """
        Receiver constructor.
        :param casan_instance: CASAN engine instance.
        :param net: L2 network to receive data from.
        :param slavelist: list of configured slaves for this network.
        :param loop: event loop to use for asynchronous I/O
        """
        self.running = False
        self.l2net = net
        self.casan_instance = casan_instance
        self.slavelist = slavelist
        self.next_hello = datetime.max
        self.next_hello_task = None
        self.deduplist = []
        self.broadcast = Slave()
        self.broadcast.l2_net = net
        self.broadcast.addr = net.broadcast
        self.loop = loop

    def run(self):
        """
        Override of the Thread.run method.
        This function loops reading for data and processes it.
        """
        #print_debug(dbg_levels.MESSAGE, 'Receiver for network interface {} lives!'.format(self.l2net.port.name))
        #while self.keep_running:
        m = Msg()
        saddr = m.recv(self.l2net)
        if saddr is None:
            return  # Message reception timeout
        print_debug(dbg_levels.MESSAGE, 'Received something from {}!'.format(saddr))

        m.expire = datetime.now() + timedelta(milliseconds=exchange_lifetime(self.l2net.max_latency))
        self.clean_deduplist()

        if not self.find_peer(m, saddr):
            print_debug(dbg_levels.MESSAGE, 'Warning : sender not in authorized peers.')
            return

        # Is the received message a reply to pending request?
        req = self.correlate(m)
        if req is not None:
            # Ignore if a reply was already received
            if req.req_rep is not None:
                return
            # This is the first reply : stop message retransmissions, link the reply to the
            # original request, and wake the emitter.
            req.stop_retransmit()
            Msg.link_req_rep(req, m)
            with self.casan_instance.condition_var:
                self.casan_instance.condition_var.notify()
            # If it is not a CASAN message, i.e. it doesn't need processing, keep loopin'
            if m.casan_type is Msg.CasanTypes.CASAN_UNKNOWN:
                m.find_casan_type()
            if m.casan_type is m.CasanTypes.CASAN_NONE:
                return

        # Was the message already received? If so, ignore it if we already replied.
        dup_msg = self.deduplicate(m)
        if dup_msg is not None:
            if dup_msg.req_rep is not None:
                return

        # Process messages.
        # Process CASAN control messages first.
        if m.find_casan_type() is not Msg.CasanTypes.CASAN_NONE:
            m.peer.process_casan(self.casan_instance, m)
            return

        # Orphaned message
        if m.peer.status is Slave.StatusCodes.SL_RUNNING:
            print_debug(dbg_levels.MESSAGE, 'Orphaned message from ' + str(m.peer.addr) + ', id=' + str(m.id))

        print_debug(dbg_levels.MESSAGE, 'Receiver for network interface {} dies!'.format(self.l2net.port.name))

    def find_peer(self, mess, addr):
        """
        Check if the sender of a message is an authorized peer. If not, check if it is an CASAN slave
        coming up.
        :param mess: received message
        :param addr: MAC address of the slave to find.
        :return: True if the sender is an authorized peer or a new CASAN slave.
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
            r = mess.is_casan_discover()
            if r[0]:
                with self.casan_instance.casan_lock:
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
            with self.casan_instance.casan_lock:  # May need to be moved above the if
                id_ = mess.id
                for mreq in self.casan_instance.mlist:
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

    def stop(self):
        """
        Stops the receiver.
        """
        self.running = False
        if self.next_hello_task is not None:
            if not self.next_hello_task.cancel():
                # If the task is not cancellable, it must be executing. Wait for it to terminate
                # and cancel the new scheduled hello.
                asyncio.wait(self.next_hello_task)
                self.next_hello_task.cancel()
