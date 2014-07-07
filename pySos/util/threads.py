from threading import Thread
import abc


class ThreadBase(Thread, metaclass=abc.ABCMeta):
    """
    Abstract base class for threads. Subclasses must override the run method.
    """
    def __init__(self):
        super().__init__()
        self.keepRunning = True

    @abc.abstractmethod
    def run(self):
        pass

    def stop(self):
        """
        Requests the thread to stop, but doesn't wait until it stops to return. If you need to wait
        until the thread stopped, use the join method.
        """
        self.keepRunning = False
