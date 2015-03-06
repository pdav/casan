"""
This module defines the base classes used for L2 networking
"""


class PkType:
    """
    Enumerates packet types (sent to me, broadcasted, whatever...)
    """
    PK_NONE = -1
    PK_ME = 0
    PK_BCAST = 1

##import abc
##from enum import Enum
##
##
##class PkTypes (Enum):
##    """
##    Enumerates packet types (sent to me, broadcasted, whatever...)
##    """
##    PK_ME = 0
##    PK_BCAST = 1
##    PK_NONE = 2
##
##
##class l2addr (metaclass=abc.ABCMeta):
##    """
##    This abstract class is the base for all l2addr-* subclasses
##    """
##    @abc.abstractmethod
##    def __eq__ (self, other):
##        """
##        Equality test operator
##        """
##        pass
##
##    @abc.abstractmethod
##    def __ne__ (self, other):
##        """
##        Difference test operator
##        """
##        pass
##
##    @abc.abstractmethod
##    def __repr__ (self):
##        """
##        Compact string representation
##        """
##        pass
##
##    @abc.abstractmethod
##    def __str__ (self):
##        """
##        "Pretty" string representation
##        """
##        pass
##
##
##class l2net (metaclass=abc.ABCMeta):
##    """
##    This abstract class is the base for all l2net-* subclasses
##    """
##    def __init__ (self):
##        """
##        Base constructor for l2nets.
##        :return:
##        """
##        self.mtu = 0
##        self.max_latency = 0
##
##    @abc.abstractmethod
##    def term (self):
##        """
##        Close the connection.
##        """
##        pass
##
##    @abc.abstractmethod
##    def send (self, daddr, data):
##        """
##        Send some data to a specific address.
##        :param daddr: destination address.
##        :param data: data to send.
##        """
##        pass
##
##    @abc.abstractmethod
##    def bsend (self, data):
##        """
##        Broadcast some data.
##        :param data: data to send.
##        """
##        pass
##
##    @abc.abstractmethod
##    def recv (self):
##        """
##        Receive some data from the network.
##        """
##        pass
