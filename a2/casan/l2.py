"""
This module defines the base classes used for L2 networking
"""


class PkType:
    """
    Enumerates packet types (sent to me, broadcasted, whatever...)
    """
    PK_ME = 0
    PK_BCAST = 1
