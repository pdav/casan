"""
This module contains the SOS class.
"""

from sos.msg import Msg
from datetime import datetime, timedelta
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
    SOS engine class
    """
    # Constants
    
    # Methods
    def __init__(self):
        self.timer_first_hello = 0
        self.timer_interval_hello = 0
        self.timer_slave_ttl = 0
        self.tsender = None
        self.sos_lock = Lock()
        self.rlist, self.slist, self.mlist = [], [], []

    def init(self):
        if self.tsender is None:
            self.tsender = Sender(self)
            self.tsender.start()

    def start_net(self, net):
        with self.sos_lock:
            r = Receiver(self, net, self.slist)
            r.next_hello = datetime.now()
            r.next_hello += timedelta(seconds=self.timer_first_hello)
            r.hello_id = r.next_hello.microsecond % 1000

            r.hello_msg = Msg()
            r.hello_msg.peer = r.broadcast
            r.hello_msg.msg_type = Msg.MsgTypes.MT_NON
            r.hello_msg.msg_code = Msg.MsgCodes.MC_POST
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
