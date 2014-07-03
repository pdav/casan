"""
This module defines the various classes used for L2 network addressing.
"""

import abc
from enum import Enum

class PkTypes(Enum):
    PK_ME = 0
    PK_BCAST = 1
    PK_NONE = 2

class l2addr(metaclass = abc.ABCMeta):
    """
    This abstract class is the base for all l2addr-* subclasses
    """
    @abc.abstractmethod
    def __eq__(self, other):
        """
        Equality test operator
        """
        pass

    @abc.abstractmethod
    def __ne__(self, other):
        """
        Difference test operator
        """
        pass
        
    @abc.abstractmethod
    def __repr__(self):
        """
        Compact string representation
        """
        pass

    @abc.abstractmethod
    def __str__(self):
        """
        "Pretty" string representation
        """
        pass

class l2net(metaclass = abc.ABCMeta):
    """
    This abstract class is the base for all l2net-* subclasses
    """
    def __init__(self):
        self.mtu = 0
        self.max_latency = 0

    @abc.abstractmethod
    def term(self):
        """
        """
        pass

    @abc.abstractmethod
    def send(self, daddr, data):
        """
        """
        pass

    @abc.abstractmethod
    def bsend(self, data):
        """
        """
        pass

    @abc.abstractmethod
    def recv(self):
        """
        """
        pass