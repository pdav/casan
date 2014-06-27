from . import l2
from util import threads
from util.debug import *
from datetime import datetime
from .msg import Msg

import pdb

class Receiver(threads.ThreadBase):
    def __init__(self, net):
        super().__init__()
        self.l2net = net
        self.next_hello = datetime.now()

    def run(self):
        print_debug(dbg_levels.MESSAGE, 'Receiver for network interface {} '
                                        'lives!'.format(self.l2net.port.name))
        while self.keepRunning:
            m = Msg()
            m.recv(self.l2net)
            print_debug(dbg_levels.MESSAGE, 'Received something!')

        print_debug(dbg_levels.MESSAGE, 'Receiver for network interface {} '
                                        'dies!'.format(self.l2net.port.name))

    def find_peer(self, addr):
        pass
