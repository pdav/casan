"""
This module provides debugging facilities.
To avoid long unnecessary typing, it is advised to import the module this way :
    from debug import *
"""
from enum import IntEnum


class dbg_levels(IntEnum):
    MESSAGE = 0x1
    OPTION = 0x2
    STATE = 0x4
    CACHE = 0x8
    CONF = 0x10
    HTTP = 0x20
    ALL = 0xFFFF

debugLevel = dbg_levels.ALL


def debug_title(level):
    for name in dbg_levels.__members__.keys():
        if level == dbg_levels[name]:
            return name
    return '(unknown level)'


def set_debug_level(sign, name):
    global debugLevel
    try:
        debugLevel = debugLevel | dbg_levels.__dict__[name]
    except KeyError:
        return False


def update_debug_level(levelStr):
    start, i, sign = 0, 0, 1
    for c in levelStr:
        sign = 1
        if c in {'+- '}:
            if c == '-':
                newSign = -1
            if set_:
                if not set_debug_level(sign, levelStr[start:i]):
                    return False
                start = i
            sign = newSign
        elif c.isalpha():
            set_ = True
        else:
            return False
        i += 1
    return set_debug_level(sign, levelStr[start:])


def print_debug(level, msg):
    global dbg_levels
    if debugLevel & level:
        print(debug_title(level) + ' : ' + msg)
