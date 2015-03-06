#!/usr/bin/env python3

# May be run as a normal user
# Test 802.15.4 async receive method

from casan import l2
from casan import l2_154

x = l2_154.l2net_154 ()
x.init ('/dev/digi', 'xbee', 0, '12:34', 'ca:fe', 25, asyncio=True)
n = 0
b = 0
try:
    while True:
        (ptype, addr, data) = x.recv ()
        if ptype == l2.PkType.PK_NONE:
            n += 1
        else:
            print (ptype, addr, data)
            b += 1
except KeyboardInterrupt:
    pass

print ('Calls to recv: PK_NONE = {}, PK_BCAST = {}'.format (n, b))
