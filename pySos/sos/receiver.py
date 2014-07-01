from util import threads
from util.debug import *
from datetime import datetime
from .msg import Msg
from .slave import Slave

class Receiver(threads.ThreadBase):
    def __init__(self, sos_instance, net, slavelist):
        super().__init__()
        self.l2net = net
        self.sos_instance = sos_instance
        self.slavelist = slavelist
        self.next_hello = datetime.now()
        self.deduplist = []
        self.broadcast = Slave()
        self.broadcast.l2n = net
        self.broadcast.addr = net.broadcast

    def run(self):
        print_debug(dbg_levels.MESSAGE, 'Receiver for network interface {} '
                                        'lives!'.format(self.l2net.port.name))
        while self.keepRunning:
            m = Msg()
            saddr = m.recv(self.l2net)
            print(m.msg)
            print_debug(dbg_levels.MESSAGE, 'Received something!')

            # Clean deduplist

            if not self.find_peer(m, saddr):
                print_debug(dbg_levels.MESSAGE, 'Warning : sender not in authorized peers.')

            # Is the received message a reply to pending request?
            req = self.correlate(m)
            if req is not None:
                # TODO : if reply
                pass

            # Was the message already received? If so, ignore it if we already replied.
            dup_msg = self.deduplicate(m)
            if dup_msg is not None:
                if dup_msg.req_rep is not None:
                    continue

            # Process messages.
            # Process SOS control messages first.
            if m.sos_type is not Msg.SosTypes.SOS_NONE:
                m.peer.process_sos(self.sos_instance, m)

            # Orphaned message
            if m.peer.status is Slave.StatusCodes.SL_RUNNING:
                print_debug(dbg_levels.MESSAGE, 'Orphaned message from ' + str(m.peer.addr) + ', id=' + str(m.id))

        print_debug(dbg_levels.MESSAGE, 'Receiver for network interface {} '
                                        'dies!'.format(self.l2net.port.name))

    def find_peer(self, mess, addr):
        """
        Check if the sender of a message is an authorized peer. If not, check if it is an SOS slave
        coming up
        :param mess: received message
        :return: True if the sender is an authorized peer or a new SOS slave.
        """
        # Is the peer already known?
        found = False
        for sl in self.slavelist:
            if addr == sl.addr:
                found = True
                mess.peer = sl
                break

        # If not found, it maybe a new slave coming up.
        if not found:
            r = mess.is_sos_discover()
            # TODO : improve readability (works fine though)
            if r[0]:
                sid, mtu = r[1], r[2]
                for s in self.slavelist:
                    if s.id == sid:
                        s.l2net = self.l2net
                        s.addr = addr
                        mess.peer = s

                        # MTU negociation
                        l2mtu = self.l2net.mtu
                        defmtu = s.defmtu if 0 <= s.defmtu <= l2mtu else l2mtu
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
            # TODO: lock
            id_ = mess.id
            for mreq in self.mlist:
                if mreq.id == id_:
                    print_debug(dbg_levels.MESSAGE, 'Found original request for ID=' + id_)
                    corr_req = mreq
                    break
        return corr_req

    def deduplicate(self, mess):
        """
        Check if a message is a duplicate and if so, returns the original message.
        If a reply was already sent (marked with reqrep), send it again.
        :param mess: message to check
        :return:
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
                # TODO : add to deduplist w/ timeout
                pass
        return orig_msg
