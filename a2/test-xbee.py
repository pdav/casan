#!/usr/bin/env python3

# May be run as a normal user
# Test 802.15.4 async receive method

import l2_154

x = l2_154.L2net_154 ()
x.init ('/dev/digi', 'xbee', 0, '12:34', 'ca:fe', 25, asyncio=True)
n = 0
b = 0
try:
    while True:
        r = x.recv ()
        if r is None:
            n += 1
        else:
            (ptype, addr, data) = r
            print (ptype, addr, data)
            b += 1
except KeyboardInterrupt:
    pass

print ('Calls to recv: None = {}, dest bcast or me = {}'.format (n, b))
