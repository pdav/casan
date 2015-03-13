#!/usr/bin/env python3

# Must be run as root
# Test Ethernet receive method

import l2_eth

e = l2_eth.l2net_eth ()
e.init ('eth0', 0, 0x0806)
n = 0
b = 0
try:
    while True:
        r = e.recv ()
        if r is None:
            n += 1
        else:
            (ptype, addr, data) = r
            print (ptype, addr, data [0:10], '...')
            b += 1
except KeyboardInterrupt:
    pass

print ('Calls to recv: None = {}, dest bcast or me = {}'.format (n, b))
