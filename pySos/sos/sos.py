"""
This module contains the SOS class.
"""

from sos.msg import Msg
from datetime import datetime, timedelta
from util.debug import *
from .receiver import Receiver
from threading import Lock, Condition
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
        self.rlist, self.slist, self.mlist = [], [], []
        self.sos_lock = Lock()
        self.condition_var = Condition(Lock())

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
        print_debug(dbg_levels.MESSAGE, 'Stopping SOS. Shutting down receivers first...')
        for i in range(len(self.rlist)):
            r = self.rlist[0]
            del self.rlist[0]  # Ensures the sender won't restart the thread
            r.stop()
            r.join()  # Wait until the thread stops
        print_debug(dbg_levels.MESSAGE, 'Shutting down sender.')
        with self.condition_var:
            self.tsender.stop()
            self.condition_var.notify()
        self.tsender.join()

    def find_slave(self, sid):
        r = None
        for s in self.slist:
            if s.id == sid:
                r = sid
                break
        return r

    def add_request(self, req):
        with self.condition_var:
            with self.sos_lock:
                self.mlist.append(req)
                self.condition_var.notify()
