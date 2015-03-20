"""
This module contains the Option class
"""


class Option (object):
    """
    Represent an individual option in a CoAP message
    Option attributes are accessed by the friend classes (Msg).
    They are:
    - optcode: option code
    - optbin: option value, binary encoded (type bytearray)
    - optval: option value, decoded (type int, string or bytes)
    """

    class Codes:
        """
        All valid option codes (see RFC 7252, sec 12.2)
        """
        CONTENT_FORMAT = 12
        ETAG = 4
        LOCATION_PATH = 8
        LOCATION_QUERY = 20
        MAX_AGE = 14
        PROXY_URI = 35
        PROXY_SCHEME = 39
        URI_HOST = 3
        URI_PATH = 11
        URI_PORT = 7
        URI_QUERY = 15
        ACCEPT = 16
        IF_NONE_MATCH = 5
        IF_MATCH = 1
        SIZE1 = 60

    # Option descriptions: optdesc [optcode] = (type, minlen, maxlen)
    # type in ['opaque', 'string', 'uint', 'empty']
    optdesc = {
        Codes.CONTENT_FORMAT: ('opaque', 0, 8),
        Codes.ETAG: ('opaque', 1, 8),
        Codes.LOCATION_PATH: ('string', 0, 255),
        Codes.LOCATION_QUERY: ('string', 0, 255),
        Codes.MAX_AGE: ('uint', 0, 4),
        Codes.PROXY_URI: ('string', 1, 1034),
        Codes.PROXY_SCHEME: ('string', 1, 255),
        Codes.URI_HOST: ('string', 1, 255),
        Codes.URI_PATH: ('string', 1, 255),
        Codes.URI_PORT: ('uint', 0, 2),
        Codes.URI_QUERY: ('string', 0, 255),
        Codes.ACCEPT: ('uint', 0, 2),
        Codes.IF_NONE_MATCH: ('empty', 0, 0),
        Codes.IF_MATCH: ('opaque', 0, 8),
        Codes.SIZE1: ('uint', 0, 4),
    }

    def __init__(self, optcode, optval=None, optbin=None):
        """
        Default constructor for the option class.
        This method works for encoding or decoding:
        - when a message is received, the optbin parameter is given
        - when a message is being built, the optval parameter is given
        :param optcode: option code
        :type  optcode: integer (see Option.Codes values)
        :param optval: option value or None
        :type  optval: int or str or bytes/bytearray
        :param optbin: option value encoded or None
        :type  optbin: bytearray
        """

        # Sanity checks
        if optcode not in Option.optdesc:
            raise ValueError ('Invalid option code')

        self.optcode = optcode
        (otype, minlen, maxlen) = Option.optdesc [optcode]

        if otype == 'empty':
            if optval is not None or optbin is not None:
                raise RuntimeError ('Option value given for an empty option')
            self.optval = None
            self.optbin = bytearray ()
            return

        if optval is not None and optbin is not None:
            raise RuntimeError ('Duplicate option value and option bin')

        if optval is not None or otype == 'empty':
            self.optval = optval
            if otype == 'uint':
                self.optbin = self.int_to_bytes (optval)
            elif otype == 'string':
                self.optbin = optval.encode (encoding='utf-8')
            elif otype == 'opaque':
                self.optbin = optval
            else:
                raise RuntimeError ('Unrecognized option type ' + otype)

        elif optbin is not None:
            self.optbin = optbin
            if otype == 'uint':
                self.optval = self.bytes_to_int (optval)
            elif otype == 'string':
                self.optval = optbin.decode (encoding='utf-8')
            elif otype == 'opaque':
                self.optval = optval
            else:
                raise RuntimeError ('Unrecognized option type ' + otype)

        else:
            # No optval and no optbin
            raise RuntimeError ('No option value')

        if not minlen <= len (self.optbin) <= maxlen:
            raise RuntimeError ('Invalid option length')

    def __lt__(self, other):
        """
        Comparison operator '<'. Implemented for the sole purpose of
        sorting an option list.
        Options are sorted by option code.
        """
        return self.optcode < other.optcode

    def __eq__(self, other):
        """
        Equality test operator

        """
        return (self.optcode == other.optcode
                and self.optval == other.optval)

    def __ne__(self, other):
        """
        Difference test operator
        """
        return (self.optcode != other.optcode
                or self.optbin != other.optbin)

    def len (self):
        """
        Length of option
        """
        return len (self.optbin)

    def is_critical (self):
        """
        Returns True if the option is critical (see CoAP draft 5.4.1)
        """
        return self.optcode & 0x01

    def is_unsafe (self):
        """
        Returns True if the option is unsafe to forward (see CoAP draft 5.4.2)
        """
        return self.optcode & 0x02

    def is_nocachekey (self):
        """
        Returns True if the option is nocachekey (see CoAP draft 5.4.2)
        """
        return (self.optcode & 0x1e) == 0x1c

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
    def bytes_to_int (b):
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
