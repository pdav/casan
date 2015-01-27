"""
This module contains the CASAN class.
"""

import random
from io import StringIO
from threading import Lock, Condition
from datetime import datetime, timedelta

from casan.msg import Msg
from util.debug import print_debug, dbg_levels
from .receiver import Receiver
from .sender import Sender
from .slave import Slave

# Seed the RNG
random.seed()
CASAN_VERSION = 1


class CASAN:
    """
    CASAN engine class
    """
    # Constants
    
    # Methods
    def __init__(self):
        """
        CASAN engine constructor. Initialized the instance with default values, but does not
        start the CASAN system. See 'init' method.
        :return:
        """
        self.timer_first_hello = 0
        self.timer_interval_hello = 0
        self.timer_slave_ttl = 0
        self.tsender = None
        self.rlist, self.slist, self.mlist = [], [], []
        self.httplist = []
        self.casan_lock = Lock()
        self.condition_var = Condition(Lock())

    def __str__(self):
        """
        Dumps the CASAN state into a string.
        """
        ss = StringIO()
        ss.write('Delay for first HELLO message = ' + str(self.timer_first_hello) + '\n')
        ss.write('HELLO interval = ' + str(self.timer_interval_hello) +'\n')
        ss.write('Default TTL = ' + str(self.timer_slave_ttl) + '\nSlaves :\n')
        for s in self.slist:
            ss.write('\t' + str(s))
        return ss.getvalue()

    def init(self):
        """
        Start the engine.
        """
        if self.tsender is None:
            self.tsender = Sender(self)
            self.tsender.start()
        for server in self.httplist:
            server.start()

    def start_net(self, net):
        """
        Start listening on a new L2 network.
        """
        with self.casan_lock:
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
        """
        Stop the engine. Note that this function WILL block if there is a
        managed L2 network without read timeout set, as it's read function
        will wait for data forever. In that case, your best bet is to just
        kill the program, or set a timeout.
        """
        # TODO : find a workaround the timeout issue. Maybe feed some data to the L2 or set a timeout on the fly?
        print_debug(dbg_levels.MESSAGE, 'Stopping CASAN. Shutting down HTTP servers...')
        for h in self.httplist:
            h.stop()
            h.join()

        print_debug(dbg_levels.MESSAGE, 'Shutting down receivers...')
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
        """
        Find a slave managed by the CASAN system from it's identification number.
        :param sid: Slave ID number.
        :return: reference to the slave if found, else None.
        """
        r = None
        for s in self.slist:
            if s.id == sid:
                r = s
                break
        return r

    def add_request(self, req):
        """
        Queue a request for processing by the sender thread.
        :param req: request to queue
        """
        with self.condition_var:
            with self.casan_lock:
                self.mlist.append(req)
                self.condition_var.notify()

    def resource_list(self):
        """
        Returns aggregated /.well-known/casan for all running slaves.
        :return: string containing the global /.well-known/casan resource list
        """
        ss = StringIO()
        for slave in self.slist:
            if slave.status is Slave.StatusCodes.SL_RUNNING:
                for res in slave.res_list:
                    ss.write(str(res))
        return ss.getvalue()
