from . import l2
from util import threads
from util.debug import *
from datetime import datetime

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
    def __init__(self, sos_instance):
        super().__init__()
        self.sos_instance = sos_instance

    def run(self):
        print_debug(dbg_levels.MESSAGE, 'Sender thread lives!')
        while self.keepRunning:
            now = datetime.now()
            for receiver in self.sos_instance.rlist:
                if not receiver.is_alive(): # Start the receiver ?
                    print_debug(dbg_levels.MESSAGE, 'Found a receiver to start')
                    receiver.start()
                if now >= receiver.next_hello: # Needs update ?
                    pass
                    #receiver.hellomsg.id_
        print_debug(dbg_levels.MESSAGE, 'Sender thread dies!')
