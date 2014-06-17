from . import l2
from util import debug, threads
from datetime import datetime, time
from .receiver import Receiver

class SOS:
    def __init__(self):
        self.tsender = None

    def init(self):
        self.rlist = []
        if self.tsender == None:
            self.tsender = Sender()
            self.tsender.start()

    def start_net(self, addr):
        r = Receiver()
        self.rlist.append(r)

    def stop(self):
        debug.print_debug(debug.dbg_levels.MESSAGE, 'Stopping SOS.')
        for i in range(len(self.rlist)):
            r = self.rlist[0]
            del self.rlist[0] # Ensures the sender won't restart the thread
            r.stop()
        self.tsender.stop()

class Sender(threads.ThreadBase):
    def run(self):
        debug.print_debug(debug.dbg_levels.MESSAGE, 'Sender thread lives!')
        while self.keepRunning:
            now = datetime.now()
            for receiver in engine.rlist:
                if not receiver.is_alive(): # Start the receiver ?
                    debug.print_debug(debug.dbg_levels.MESSAGE, 'Found a receiver to start')
                    receiver.start()
                if now >= receiver.next_hello: # Needs update ?
                    pass
                    #receiver.hellomsg.id_
        debug.print_debug(debug.dbg_levels.MESSAGE, 'Sender thread dies!')

engine = SOS()
