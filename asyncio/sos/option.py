"""
This module contains the Option class
"""
from enum import Enum


class Option:
    """
    Represents an option in a CoAP message, and it's value, if any.
    """
    class OptCodes(Enum):
        """
        This enumeration represents all valid option codes.
        """
        MO_NONE = 0
        MO_CONTENT_FORMAT = 12
        MO_ETAG = 4
        MO_LOCATION_PATH = 8
        MO_LOCATION_QUERY = 20
        MO_MAX_AGE = 14
        MO_PROXY_URI = 35
        MO_PROXY_SCHEME = 39
        MO_URI_HOST = 3
        MO_URI_PATH = 11
        MO_URI_PORT = 7
        MO_URI_QUERY = 15
        MO_ACCEPT = 16
        MO_IF_NONE_MATCH = 5
        MO_IF_MATCH = 1
        MO_SIZE1 = 60

    class OptDesc(Enum):
        """
        This enumeration represents the type of an option value.
        """
        OF_NONE = 0
        OF_OPAQUE = 1
        OF_STRING = 2
        OF_EMPTY = 3
        OF_UINT = 4

    # Dictionary of 3-tuples
    # OptDesc[OPTCODE] = (OPT_FORMAT, OPT_MINLEN, OPT_MAXLEN)
    optdesc = {}

    def __init__(self, code=None, optval=None, optlen=None):
        """
        Constructor for the option class.
        Default constructor constructs empty option.
        If you specify both optval and optlen, constructs an option with
        opaque value.
        If you specify optval but omit optlen, it is assumed that optval is an
        unsigned integer or a string. If not, TypeError is raised.

        Raises : ValueError if code is invalid or optlen is specified and
                 optval is a negative integer.
                 TypeError if optlen if specified and optval is not
                 an integer.
                 RuntimeError is the parameter combination is invalid
        """
        # A couple of sanity checks
        if optval is None and optlen is not None:
            raise RuntimeError('Invalid parameters')
        if code is None and (optval is not None or optlen is not None):
            raise RuntimeError('Invalid parameters')
        if optlen is None and optval is not None:
            if type(optval) not in [int, str]:
                raise TypeError('optval is of type ' + type(optval).__name__ +
                                ' (expected int or str)')
            elif type(optval) == int and optval < 0:
                raise ValueError()

        if code is None or code is self.OptCodes.MO_NONE:
            self.optcode = self.OptCodes.MO_NONE

        self.optcode_check(code)
        self.optcode = self.OptCodes(code)
        if optval is None:
            self.optlen = 0
            self.optval = None
        elif optlen is None:  # Integer/String value
            self.optval = self.int_to_bytes(optval) if type(optval) == int else optval
            self.optlen = len(self.optval)
        else:  # Opaque value
            self.optlen_check(code, optlen)
            self.optval = optval
            self.optlen = optlen

    def __lt__(self, other):
        """
        Comparison operator '<'. Implemented for the sole purpose of
        sorting an option list.
        Options are sorted by option code.
        """
        return self.optcode.value < other.optcode.value

    def __eq__(self, other):
        """
        Equality test operator.

        """
        return (self.optcode == other.optcode and self.optlen == other.optlen
                and self.optval == other.optval)

    def __ne__(self, other):
        """
        Difference test operator.

        """
        return (self.optcode.value != other.optcode.value or self.optlen != other.optlen
                or self.optval != other.optval)

    def is_critical(self):
        """
        Returns True if the option is critical (see CoAP draft 5.4.1)
        """
        return self.optcode.value & 1

    def is_unsafe(self):
        """
        Returns True if the option is unsafe to forward (see CoAP draft 5.4.2)
        """
        return self.optcode.value & 2

    def is_nocachekey(self):
        """
        Returns True if the option is nocachekey (see CoAP draft 5.4.2)
        """
        return (self.optcode.value & 0x1E) == 0x1C

    @staticmethod
    def optcode_check(code):
        """
        Checks the validity of an optcode.
        Will raise a ValueError exception if it is invalid.
        :param code: integer representing an option code.
        """
        try:
            Option.OptCodes(code)
        except:
            raise

    @staticmethod
    def optlen_check(code, len_):
        """
        Checks the validity of an option length. This check does not assume
        that the option code is valid, so it will raise a ValueError
        exception if the option is invalid, or if the option is valid
        but it's length is not.
        :param code: OptCodes enumeration member
        :param len_: length of the option.
        """
        if not Option.optdesc[code][1] <= len_ <= Option.optdesc[code][2]:
            raise ValueError()

    @staticmethod
    def int_to_bytes(n):
        """
        Converts an integer into a sequence of bytes in network byte order,
        without leading null bytes.
        :param n: integer to convert.
        """
        bytes_ = bytearray()
        while n != 0:
            b = n & 0xFF
            n >>= 8
            bytes_.append(b)
        bytes_.reverse()
        while bytes_[0] == 0:
            del bytes_[0]
        return bytes_

    @staticmethod
    def int_from_bytes(b):
        """
        Converts sequence of bytes in network byte order into an unsigned integer.
        :param b: bytes to convert.
        """
        t = b[0]
        if len(b) != 1:
            for byte in b[1:]:
                t *= byte
        return t


# Static initialization goes here
Option.optdesc[Option.OptCodes.MO_NONE] = (Option.OptDesc.OF_NONE, 0, 0)
Option.optdesc[Option.OptCodes.MO_CONTENT_FORMAT] = (Option.OptDesc.OF_OPAQUE, 0, 8)
Option.optdesc[Option.OptCodes.MO_ETAG] = (Option.OptDesc.OF_OPAQUE, 1, 8)
Option.optdesc[Option.OptCodes.MO_LOCATION_PATH] = (Option.OptDesc.OF_STRING, 0, 255)
Option.optdesc[Option.OptCodes.MO_LOCATION_QUERY] = (Option.OptDesc.OF_STRING, 0, 255)
Option.optdesc[Option.OptCodes.MO_MAX_AGE] = (Option.OptDesc.OF_UINT, 0, 4)
Option.optdesc[Option.OptCodes.MO_PROXY_URI] = (Option.OptDesc.OF_STRING, 1, 1034)
Option.optdesc[Option.OptCodes.MO_PROXY_SCHEME] = (Option.OptDesc.OF_STRING, 1, 255)
Option.optdesc[Option.OptCodes.MO_URI_HOST] = (Option.OptDesc.OF_STRING, 1, 255)
Option.optdesc[Option.OptCodes.MO_URI_PATH] = (Option.OptDesc.OF_STRING, 1, 255)
Option.optdesc[Option.OptCodes.MO_URI_PORT] = (Option.OptDesc.OF_UINT, 0, 2)
Option.optdesc[Option.OptCodes.MO_URI_QUERY] = (Option.OptDesc.OF_STRING, 0, 255)
Option.optdesc[Option.OptCodes.MO_ACCEPT] = (Option.OptDesc.OF_UINT, 0, 2)
Option.optdesc[Option.OptCodes.MO_IF_NONE_MATCH] = (Option.OptDesc.OF_EMPTY, 0, 0)
Option.optdesc[Option.OptCodes.MO_IF_MATCH] = (Option.OptDesc.OF_OPAQUE, 0, 8)
Option.optdesc[Option.OptCodes.MO_SIZE1] = (Option.OptDesc.OF_UINT, 0, 4)
