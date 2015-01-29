SOS master in Python
====================

Prerequisites
-------------
- Python >= 3.4
- pySerial (tested with 2.6.1 but older versions 'should' work)

Known issues
------------
- The program won't shut down gracefully if no read timeout is set on a network interface a receiver is
  started for.
- The first hello message is not sent after the time specified by timer_first_hello. I believe this is
  caused by the wait on the condition variable, which is probably stalling the event loop.

TODO
----

### Ethernet support

See <http://stackoverflow.com/questions/12229155/how-do-i-send-an-raw-ethernet-frame-in-python>


### Asyncio

Finish porting to asyncio.

#### What has been done

- Receivers and sender are not thread objects anymore. There are only 2 managed threads in the program:
    - The main thread deals with the initialization.
    - The second thread is started by the master and is the SOS control thread. The SOS class IS a thread.
- Hello messages are sent by a callback which reschedules itself at each iteration. After sending the
first hello, you can forget they even exist.

- The receivers are executed in a ThreadPoolExecutor. This is mandatory, because receivers block when
waiting for data, which can stall the event loop. They are executed in a way similar to hello messages
emission. When the sender starts a receiver, it schedules it for execution and registers a callback
to reschedule it over and over.

#### What's left to do

- Clean up the whole 'next timeout' mess at the bottom of sender.run. There's gotta be a better,
cleaner, simpler way to do this. As a bonus, I think it would fix known issue #2.

- Switching from a hand managed thread based model to asyncio means a lot of things must change. It's
possible that some comments or docstrings are out of date, mainly in the sender and receivers.

#### What could maybe be done but I wouldn't bet my life on that

- Implementing L2's as asyncio transports could make the program MUCH simpler. This could
allow using the high level stream API on L2 networks.
Problem is, I couldn't find ANY documentation on the subject. Not very surprising since asyncio is
very recent, but I don't even know if it would be possible.
