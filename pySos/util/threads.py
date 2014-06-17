from threading import Thread
import abc

class ThreadBase(Thread, metaclass = abc.ABCMeta):
    '''
    Abstract base class for threads. Subclasses must override the run method.
    '''
    def __init__(self):
        super().__init__()
        self.keepRunning = True

    @abc.abstractmethod
    def run(self):
        pass

    def stop(self):
        '''
        Requests the sender thread to stop. This doesn't return until the 
        thread has actually been stopped, which can take a while.
        '''
        self.keepRunning = False
        self.join()

