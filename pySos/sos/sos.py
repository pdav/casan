from util import threads
from util.debug import *
from datetime import datetime
from .receiver import Receiver
from threading import Lock

SOS_VERSION = 1


class SOS:
    """
    This is the SOS engine class
    """
    # Constants
    
    # Methods
    def __init__(self):
        self.tsender = None
        self.sos_lock = Lock()
        self.mlist = []

    def init(self):
        self.rlist, self.slist = [], []
        if self.tsender == None:
            self.tsender = Sender()
            self.tsender.start()

    def start_net(self, net):
        r = Receiver(self, net, self.slist)
        self.rlist.append(r)

    def stop(self):
        print_debug(dbg_levels.MESSAGE, 'Stopping SOS.')
        for i in range(len(self.rlist)):
            r = self.rlist[0]
            del self.rlist[0] # Ensures the sender won't restart the thread
            r.stop()
        self.tsender.stop()

    def add_request(self, req):
        with self.sos_lock:
            self.mlist.append(req)


class Sender(threads.ThreadBase):
    """
     Sender thread

    The sender thread spends its life blocking on a condition variable,
    waiting for events:
    - timer event, signalling that a request timeout has expired
    - change in L2 network handling (new L2, or removed L2)
    - a new request is added (from an exterior thread)

    The sender thread manages:
    - the list of l2 networks (the thread must start a new receiver
        thread for each new l2 network)
    - the list of all slaves in order to expire the "active" status
        if needed
    - the list of all outgoing messages in order to:
       - send a new message
       - retransmit a message if no answer has been received yet
       - expire an old message without any received answer. In this
        case, the message will only be deleted if there is no
        thread waiting for this message.
    """
    def run(self):
        print_debug(dbg_levels.MESSAGE, 'Sender thread lives!')
        while self.keepRunning:
            now = datetime.now()
            for receiver in engine.rlist:
                if not receiver.is_alive(): # Start the receiver ?
                    print_debug(dbg_levels.MESSAGE, 'Found a receiver to start')
                    receiver.start()
                if now >= receiver.next_hello: # Needs update ?
                    pass
                    #receiver.hellomsg.id_
        print_debug(dbg_levels.MESSAGE, 'Sender thread dies!')

engine = SOS()
