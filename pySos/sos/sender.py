from util import threads
from threading import Condition, Lock
from util.debug import *
from datetime import datetime, timedelta
from sys import stderr

MAX_RETRANSMIT = 4

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
        self.condition_var = Condition(Lock())
        self.sos_instance = sos_instance

    def run(self):
        print_debug(dbg_levels.MESSAGE, 'Sender thread lives!')
        while self.keepRunning:
            self.condition_var.acquire()
            now = datetime.now()

            # Traverse the receiver list and start new receivers
            for receiver in self.sos_instance.rlist:
                if not receiver.is_alive():
                    print_debug(dbg_levels.MESSAGE, 'Found a receiver to start')
                    receiver.start()
                if now >= receiver.next_hello: # Needs update ?
                    receiver.hello_msg.id = 0
                    receiver.hello_msg.send()
                    receiver.hello_msg.next_hello = datetime.now() + timedelta(seconds = self.sos_instance.timer_interval_hello)

            # Traverse slavelist and check ttl
            for slave in self.sos_instance.slist:
                if slave.status is slave.StatusCodes.SL_RUNNING and slave.next_timeout <= now:
                    slave.reset()

            # Traverse message list to send new messages or retransmit old ones.
            for mess in self.sos_instance.mlist:
                if mess.ntrans == 0 or (mess.ntrans < MAX_RETRANSMIT and now >= mess.next_timeout):
                    if not mess.send():
                        stderr.write('Error : cannot send message.')
            self.mlist = [m for m in self.sos_instance.mlist if datetime.now() > m.expire]

            # Determine date of next action.
            self.next_timeout = datetime.max

            for r in self.sos_instance.rlist:
                # Is current timeout the next hello?
                if self.next_timeout > r.next_hello:
                    self.next_timeout = r.next_hello
            for s in self.sos_instance.slist:
                # Same here
                if s.status is s.StatusCodes.SL_RUNNING and self.next_timeout > s.next_hello:
                    self.next_timeout = s.next_hello
            for m in self.sos_instance.mlist:
                # Is current timeout the time of this message's next retransmission
                if m.ntrans < MAX_RETRANSMIT and self.next_timeout > m.next_timeout:
                    self.next_timeout = m.next_timeout

            if self.next_timeout == datetime.max:
                print_debug(dbg_levels.MESSAGE, 'WAIT INDEFINITELY')
                self.condition_var.wait()
            else:
                delay = (self.next_timeout - datetime.now()).total_seconds()
                assert delay > 0.  # Just to be safe
                print_debug(dbg_levels.MESSAGE, 'WAIT FOR: {} seconds'.format(delay))
                self.condition_var.wait(delay)

            self.condition_var.release()

        print_debug(dbg_levels.MESSAGE, 'Sender thread dies!')
