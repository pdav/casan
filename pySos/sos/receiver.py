from . import l2
from util import debug, threads
from datetime import datetime

class Receiver(threads.ThreadBase):
    def __init__(self, id_ = None):
        super().__init__()
        self.id_ = id_
        self.next_hello = datetime.now()

    def run(self):
        print('Receiver thread n°{} lives!'.format(self.id_))
        while self.keepRunning:
            pass
        print('Receiver thread n°{} dies!'.format(self.id_))
