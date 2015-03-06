#!/usr/bin/env python3

# Must be run as root
# Test Ethernet receive method

from casan import l2
from casan import l2_eth

e = l2_eth.l2net_eth ()
e.init ('eth0', 0, 0x0806)
n = 0
b = 0
try:
    while True:
        (ptype, addr, data) = e.recv ()
        if ptype == l2.PkType.PK_NONE:
            n += 1
        else:
            print (ptype, addr, data [0:10], '...')
            b += 1
except KeyboardInterrupt:
    pass

print ('Calls to recv: PK_NONE = {}, PK_BCAST = {}'.format (n, b))
