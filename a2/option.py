"""
This module contains the Option class
"""


class Option:
    """
    Represents an option in a CoAP message
    Option attributes are accessed by the friend Msg class.
    """

    class OptCodes:
        """
        All valid option codes.
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

    class OptTypes:
        """
        Types of option values
        """
        OF_OPAQUE = 1
        OF_EMPTY = 3

    # Option descriptions: optdesc [optcode] = (type, minlen, maxlen)
    # type in ['none', 'opaque', 'string', 'uint', 'empty']
    optdesc = {
        OptCodes.MO_NONE: ('none', 0, 0),
        OptCodes.MO_CONTENT_FORMAT: ('opaque', 0, 8),
        OptCodes.MO_ETAG: ('opaque', 1, 8),
        OptCodes.MO_LOCATION_PATH: ('string', 0, 255),
        OptCodes.MO_LOCATION_QUERY: ('string', 0, 255),
        OptCodes.MO_MAX_AGE: ('uint', 0, 4),
        OptCodes.MO_PROXY_URI: ('string', 1, 1034),
        OptCodes.MO_PROXY_SCHEME: ('string', 1, 255),
        OptCodes.MO_URI_HOST: ('string', 1, 255),
        OptCodes.MO_URI_PATH: ('string', 1, 255),
        OptCodes.MO_URI_PORT: ('uint', 0, 2),
        OptCodes.MO_URI_QUERY: ('string', 0, 255),
        OptCodes.MO_ACCEPT: ('uint', 0, 2),
        OptCodes.MO_IF_NONE_MATCH: ('empty', 0, 0),
        OptCodes.MO_IF_MATCH: ('opaque', 0, 8),
        OptCodes.MO_SIZE1: ('uint', 0, 4),
    }

    def __init__(self, code=None, optval=None, optlen=None):
        """
        Default constructor for the option class.
        If you specify both optval and optlen, constructs an option with
        opaque value.
        If you specify optval but omit optlen, it is assumed that optval is an
        unsigned integer or a string. If not, TypeError is raised.

        Raises : ValueError if code is invalid or optlen is specified and
                 optval is a negative integer.
                 TypeError if optlen if specified and optval is not
                 an integer.
                 RuntimeError is the parameter combination is invalid

        :param code: option code
        :type  code: integer (see Option.OptCodes values)
        :param optval: option value
        :type  optval: int or str
        :param optlen: option length
        :type  optlen: int
        """

        self.code = None
        self.optval = None
        self.optval = None

        # Sanity checks
        if optval is None and optlen is not None:
            raise RuntimeError ('Invalid parameters: optlen without optval')
        if code is None and not (optval is None and optlen is None):
            raise RuntimeError ('Invalid parameters: optval/len without code')
        if optlen is None and optval is not None:
            if type (optval) not in [int, str]:
                raise TypeError ('Unexpected type ' + type (optval).__name__)
            elif type (optval) == int and optval < 0:
                raise ValueError ('Invalid option value')

        if code is None:
            code = self.OptCodes.MO_NONE
        if code not in Option.optdesc:
            raise ValueError ('Invalid option code')
        self.optcode = code

        if optval is None:
            self.optlen = 0
            self.optval = None

        elif optlen is None:    # Integer/String value
            if type (optval) == int:
                self.optval = self.int_to_bytes (optval)
            else:
                self.optval = optval
            self.optlen = len (self.optval)

        else:                    # Opaque value
            od = Option.optdesc [self.optcode]
            if not od [1] <= optlen <= od [2]:
                raise ValueError ('Invalid option length')
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
        Equality test operator

        """
        return (self.optcode == other.optcode
                and self.optlen == other.optlen
                and self.optval == other.optval)

    def __ne__(self, other):
        """
        Difference test operator
        """
        return (self.optcode.value != other.optcode.value
                or self.optlen != other.optlen
                or self.optval != other.optval)

    def is_critical (self):
        """
        Returns True if the option is critical (see CoAP draft 5.4.1)
        """
        return self.optcode.value & 0x01

    def is_unsafe (self):
        """
        Returns True if the option is unsafe to forward (see CoAP draft 5.4.2)
        """
        return self.optcode.value & 0x02

    def is_nocachekey (self):
        """
        Returns True if the option is nocachekey (see CoAP draft 5.4.2)
        """
        return (self.optcode.value & 0x1e) == 0x1c

    @staticmethod
    def int_to_bytes (n):
        """
        Convert an integer into a sequence of bytes in network byte order,
        without leading null bytes.
        :param n: integer to convert.
        """
        bytes_ = bytearray ()
        while n != 0:
            b = n & 0xff
            n >>= 8
            bytes_.append (b)

        bytes_.reverse ()
        while bytes_ [0] == 0:
            del bytes_ [0]

        return bytes_

    @staticmethod
    def int_from_bytes (b):
        """
        Convert a sequence of bytes in network byte order into an
        unsigned integer.
        :param b: bytes to convert.
        :type  b: bytearray
        """
        t = b [0]
        if len (b) != 1:
            for byte in b [1:]:
                t *= byte
        return t
