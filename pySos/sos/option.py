'''
This module contains the Option class
'''
from util.enum import Enum

class Option:
    '''
    Represents an option in a CoAP message, and it's value, if any.
    '''
    optcodes = Enum('optcodes', {'MO_NONE' : 0, 'MO_CONTENT_FORMAT' : 12,
                                 'MO_ETAG' : 4, 'MO_LOCATION_PATH' : 8,
                                 'MO_LOCATION_QUERY' : 20, 'MO_MAX_AGE' : 14,
                                 'MO_PROXY_URI' : 35, 'MO_PROXY_SCHEME' : 39,
                                 'MO_URI_HOST' : 3, 'MO_URI_PATH' : 11,
                                 'MO_URI_PORT' : 7, 'MO_URI_QUERY' : 15,
                                 'MO_ACCEPT' : 16, 'MO_IF_NONE_MATCH' : 5,
                                 'MO_IF_MATCH' : 1, 'MO_SIZE1' : 60 })

    optdesc = Enum('optdesc', ['OF_NONE', 'OF_OPAQUE', 'OF_STRING',
                               'OF_EMPTY', 'OF_UINT'])

    # Dictionary of 3-tuples
    # optdesc_[OPTCODE] = (OPT_FORMAT, OPT_MINLEN, OPT_MAXLEN)
    optdesc_ = {}

    def is_critical(self):
        '''
        Returns True if the option is critical (see CoAP draft 5.4.1)
        '''
        return self.optcode & 1

    def is_unsafe(self):
        '''
        Returns True if the option is unsafe to forward (see CoAP draft 5.4.2)
        '''
        return self.optcode & 2

    def is_nocachekey(self):
        '''
        Returns True if the option is nocachekey (see CoAP draft 5.4.2)
        '''
        return (self.optcode & 0x1E) == 0x1C

    def __init__(self, code = None, optval = None, optlen = None):
        '''
        Constructor for the option class.
        Default constructor constructs empty option.
        If you specify both optval and optlen, constructs an option with
        opaque value.
        If you specify optval but omit optlen, it is assumed that optval is an
        unsigned integer. If optval is not an unsigned integer, a 
        TypeError is raised.

        Raises : ValueError if code is invalid or optlen is specified and
                 optval is a negative integer.
                 TypeError if optlen if specified and optval is not 
                 an integer.
                 RuntimeError is the parameter combination is invalid
        '''
        # A couple of sanity checks
        if optval == None and optlen != None:
            raise RuntimeError('Invalid parameters')
        if code == None and (optval != None or optlen != None):
            raise RuntimeError('Invalid parameters')
        if optlen == None:
            if type(optval) != int:
                raise TypeError('optval is of type ' + type(optval).__name__ + 
                                ' (expected int)')
            elif optval < 0:
                raise ValueError('optval must be positive')

        if code == None or code == self.optcodes.MO_NONE:
            self.optcode = self.optcodes.MO_NONE

        self.optcode_check(code)
        self.optcode = code
        if optval == None:
            self.optlen = 0
            self.optval = None
        elif optlen == None: # Integer value
            self.optval = self.int_to_bytes(optval)
            # I'm not quite sure of this one, c++ code is kinda funky
            self.optlen = len(self.optval) 
        else: # Opaque value
            self.optlen_check(code, optlen)
            self.optval = bytearray(optval)
            self.optlen = optlen

    def __lt__(self, other):
        '''
        Comparison operator '<'. Implemented for the sole purpose of 
        sorting an option list.
        Options are sorted by option code.
        '''
        return self.optcode < other.optcode

    @staticmethod
    def optcode_check(code):
        '''
        Checks the validity of an optcode.
        Will raise a ValueError exception if it is invalid.
        '''
        if code not in Option.optdesc or code == Option.optdesc.MO_NONE:
            raise ValueError()

    @staticmethod
    def optlen_check(code, len_):
        '''
        Checks the validity of an option length. This check does not assume
        that the option code is valid, so it will raise a ValueError
        exception if the option is invalid, or if the option is valid
        but its length is not.
        '''
        if not Option.optdesc[code][1] <= len_ <= Option.optdesc[code][2]:
            raise ValueError()

    @staticmethod
    def int_to_bytes(n):
        '''
        Converts an integer into a sequence of bytes in network byte order,
        without leading null bytes.
        '''
        bytes_ = bytearray()
        while n != 0:
            b = n & 0xFF
            n = n >> 8
            bytes_.append(b)
        bytes_.reverse()
        while bytes_[0] == 0:
            del bytes_[0]
        return bytes_


# Static initialization goes here
Option.optdesc_[Option.optcodes.MO_NONE] = (OF_NONE, 0, 0)
Option.optdesc_[Option.optcodes.MO_CONTENT_FORMAT] = (OF_OPAQUE, 0, 8)
Option.optdesc_[Option.optcodes.MO_ETAG] = (OF_OPAQUE, 1, 8)
Option.optdesc_[Option.optcodes.MO_LOCATION_PATH] = (OF_STRING, 0, 255)
Option.optdesc_[Option.optcodes.MO_LOCATION_QUERY] = (OF_STRING, 0, 255)
Option.optdesc_[Option.optcodes.MO_MAX_AGE] = (OF_UINT, 0, 4)
Option.optdesc_[Option.optcodes.MO_PROXY_URI] = (OF_STRING, 1, 1034)
Option.optdesc_[Option.optcodes.MO_PROXY_SCHEME] = (OF_STRING, 1, 255)
Option.optdesc_[Option.optcodes.MO_URI_HOST] = (OF_STRING, 1, 255)
Option.optdesc_[Option.optcodes.MO_URI_PATH] = (OF_STRING, 1, 255)
Option.optdesc_[Option.optcodes.MO_URI_PORT] = (OF_UINT, 0, 2)
Option.optdesc_[Option.optcodes.MO_URI_QUERY] = (OF_STRING, 0, 255)
Option.optdesc_[Option.optcodes.MO_ACCEPT] = (OF_UINT, 0, 2)
Option.optdesc_[Option.optcodes.MO_IF_NONE_MATCH] = (OF_EMPTY, 0, 0)
Option.optdesc_[Option.optcodes.MO_IF_MATCH] = (OF_OPAQUE, 0, 8)
Option.optdesc_[Option.optcodes.MO_SIZE1] = (OF_UINT, 0, 4)
