"""
This module contains the Sender class.
"""
from datetime import datetime, timedelta
from sys import stderr

from util.debug import print_debug, dbg_levels
from util import threads

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
        self.sos_instance = sos_instance
        self.next_timeout = datetime.now() + timedelta(seconds=1)

    def run(self):
        """
        Override of the Thread.run method.
        This method sleeps and wakes up periodically when there is something to be done, i.e starting a Receiver
        for a new L2 network, or sending a pending message.
        """
        print_debug(dbg_levels.MESSAGE, 'Sender thread lives!')
        while self.keepRunning:
            now = datetime.now()
            with self.sos_instance.condition_var:
                with self.sos_instance.sos_lock:
                    # Traverse the receiver list and start new receivers
                    for receiver in self.sos_instance.rlist:
                        if not receiver.is_alive():
                            print_debug(dbg_levels.MESSAGE, 'Found a receiver to start')
                            receiver.start()
                        if now >= receiver.next_hello:  # Needs update ?
                            # Set ID to 0 will generate a new one, but we need to clear
                            # the binary representation of the message.
                            receiver.hello_msg.id = 0
                            receiver.hello_msg.msg = None
                            if not receiver.hello_msg.send():
                                print_debug(dbg_levels.MESSAGE, 'Error sending hello!')
                            receiver.next_hello = (datetime.now() +
                                                   timedelta(seconds=self.sos_instance.timer_interval_hello))

                    # Traverse slavelist and check ttl
                    for slave in self.sos_instance.slist:
                        if slave.status is slave.StatusCodes.SL_RUNNING and slave.next_timeout <= now:
                            slave.reset()

                    # Traverse message list to send new messages or retransmit old ones.
                    m = None
                    for mess in self.sos_instance.mlist:
                        if mess.ntrans == 0 or (mess.ntrans < MAX_RETRANSMIT and now >= mess.next_timeout):
                            m = mess
                            if not mess.send():
                                stderr.write('Error : cannot send message.')
                    self.sos_instance.mlist = [m for m in self.sos_instance.mlist if datetime.now() < m.expire]
                    if m is not None and m not in self.sos_instance.mlist:
                        pass

                    # Determine date of next action.
                    self.next_timeout = datetime.max

                    for r in self.sos_instance.rlist:
                        # Is current timeout the next hello?
                        if self.next_timeout > r.next_hello:
                            self.next_timeout = r.next_hello
                    for s in self.sos_instance.slist:
                        # Same here
                        if s.status is s.StatusCodes.SL_RUNNING and self.next_timeout > s.next_timeout:
                            self.next_timeout = s.next_timeout
                    for m in self.sos_instance.mlist:
                        # Is current timeout the time of this message's next retransmission
                        if m.ntrans < MAX_RETRANSMIT and self.next_timeout > m.next_timeout:
                            self.next_timeout = m.next_timeout
                # Lock released
                if self.next_timeout == datetime.max:
                    print_debug(dbg_levels.MESSAGE, 'WAIT INDEFINITELY')
                    self.sos_instance.condition_var.wait()
                else:
                    delay = (self.next_timeout - datetime.now()).total_seconds()
                    if delay < 0:  # Just to be safe
                        print_debug(dbg_levels.MESSAGE, ('WARNING : negative delay ({}), '
                                                         'trying to continue.'.format(delay)))
                    else:
                        print_debug(dbg_levels.MESSAGE, 'WAIT FOR: {} seconds'.format(delay))
                        self.sos_instance.condition_var.wait(delay)
            # Condition var released

        print_debug(dbg_levels.MESSAGE, 'Sender thread dies!')
