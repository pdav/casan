"""
This module contains the Sender class.
"""
from datetime import datetime, timedelta
from sys import stderr
import asyncio

from util.debug import print_debug, dbg_levels

MAX_RETRANSMIT = 4


class Sender():
    """
    The sender spends its life blocking on a condition variable,
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
    def __init__(self, sos_instance, loop):
        """
        Sender constructor.
        :param sos_instance: SOS engine instance.
        :param loop: event loop to use for asynchronous I/O.
        """
        self.keep_running = True
        self.sos_instance = sos_instance
        self.next_timeout = datetime.now() + timedelta(seconds=1)
        self.loop = loop
        self.future_packets = {}

    def add_task(self, t, *args, **kwargs):
        """
        Adds a task to the event loop for parallel processing.
        This functions uses the loop.create_task method, which is new in python 3.4.2.
        If python 3.4.2 is not available, an alternative is to use loop.run_in_executor.
        Ubuntu (14.04) only provides python 3.4.0 so far, so this method is not used yet.
        :param t: task to add
        :param args: arguments to pass to the task
        :param kwargs: keyword arguments to pass to the task
        """
        self.loop.create_task(t(*args, **kwargs))

    def start_receiver(self, r):
        """
        Starts a receiver.
        :param r: receiver to start
        :return: reference to the started receiver.
        """
        # Schedule the first hello message.
        self.loop.call_later(self.sos_instance.timer_first_hello, self.send_hello, r)

        # Schedule receiver execution
        self.future_packets[r] = asyncio.async(self.loop.run_in_executor(None, r.run), loop=self.loop)

        # Add a callback to reschedule execution when done.
        # The callback will be passed only one parameter : the done future (see python doc)
        self.future_packets[r].add_done_callback(self.schedule_receiver)
        r.running = True
        return r

    def schedule_receiver(self, fut):
        """
        Callback invoked when a receiver finished processing a received message.
        It reschedules execution of the receiver.run method, and registers itself
        as a callback when execution finishes.
        :param fut: done future.
        """
        r = None
        # Use the future we get as a parameter to find which receiver returned.
        for rec, f in self.future_packets.items():
            if fut is f:
                r = rec
        # Reschedule execution
        # Do NOT yield from run_in_executor. This will make this function a coroutine
        # and prevent it from being scheduled as a callback.
        self.future_packets[r] = asyncio.async(self.loop.run_in_executor(None, r.run), loop=self.loop)
        self.future_packets[r].add_done_callback(self.schedule_receiver)

    def send_hello(self, r):
        """
        Sends a hello message for a specific receiver and schedules the next hello.
        :param r: Can be :
                          - a future whose result is the receiver to send the hello message for.
                          - the receiver to send the message for.
        """
        r = r.result() if type(r) == asyncio.Task else r
        # We need to clear the binary representation of the message to increment the ID
        r.hello_msg.id = 0
        r.hello_msg.msg = None

        res = r.hello_msg.send()
        if not res:
            print_debug(dbg_levels.MESSAGE, 'Error sending hello!')

        # Schedule next hello
        r.next_hello = (datetime.now() + timedelta(seconds=self.sos_instance.timer_interval_hello))
        r.next_hello_task = self.loop.call_later(self.sos_instance.timer_interval_hello, self.send_hello, r)

    @asyncio.coroutine
    def run(self):
        """
        This method sleeps and wakes up periodically when there is something to be done, i.e starting a Receiver
        for a new L2 network, or sending a pending message.
        """
        now = datetime.now()
        h = None
        with self.sos_instance.condition_var:
            with self.sos_instance.sos_lock:
                # First, traverse future list and reschedule

                # Traverse the receiver list and start new receivers
                for receiver in self.sos_instance.rlist:
                    if not receiver.running:
                        print_debug(dbg_levels.MESSAGE, 'Found a receiver to start')
                        self.start_receiver(receiver)

                # Traverse slavelist and check ttl
                for slave in self.sos_instance.slist:
                    if slave.status is slave.StatusCodes.SL_RUNNING and slave.next_timeout <= now:
                        slave.reset()

                # Traverse message list to send new messages or retransmit old ones.
                m = None
                for mess in self.sos_instance.mlist:
                    if mess.ntrans == 0 or (mess.ntrans < MAX_RETRANSMIT and now >= mess.next_timeout):
                        r = mess.send()
                        if not r:
                            stderr.write('Error : cannot send message.')
                self.sos_instance.mlist = [m for m in self.sos_instance.mlist if datetime.now() < m.expire]
                if m is not None and m not in self.sos_instance.mlist:
                    pass

                # Determine date of next action.
                self.next_timeout = datetime.now() + timedelta(seconds=10)  # Temporary

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
