import threading
import time
import debug

class sos:
    def __init__(self):
        self.tsender = None

    def init(self):
        self.rlist = []

engine = sos()

# Threads
class senderT(threading.Thread):
    def __init__(self):
        pass

    def run(self):
        now = time.datetime.now()
        for receiver in engine.rlist:
            if receiver.thr == None: # Start the receiver ?
                debug.print_debug(debug.dbg_levels.MESSAGE, 'Found a receiver to start')
                receiver.run()
            if now >= receiver.next_hello: # Needs update ?
                pass
                #receiver.hellomsg.id_
