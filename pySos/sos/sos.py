from sos.msg import Msg
from time import time
from util.debug import *
from .receiver import Receiver
from threading import Lock
from .sender import Sender
import random

# Seed the RNG
random.seed()
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
            self.tsender = Sender(self)
            self.tsender.start()

    def start_net(self, net):
        r = Receiver(self, net, self.slist)
        r.hello_id = int(time()) % 1000
        r.next_hello = int(time()) + random.randrange(1000)
        r.hello_msg = Msg()
        r.hello_msg.peer = r.broadcast
        r.hello_msg.type = Msg.MsgTypes.MT_NON
        r.hello_msg.code = Msg.MsgCodes.MC_POST
        r.hello_msg.mk_ctrl_hello(r.hello_id)

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

engine = SOS()
