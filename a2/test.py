#!/usr/bin/env python3

# Must be run as root
# Test Ethernet and 802.15.4 receive methods with asyncio

import asyncio

from casan import l2
from casan import l2_154
from casan import l2_eth

x = l2_154.l2net_154 ()
x.init ('/dev/digi', 'xbee', 0, '12:34', 'ca:fe', 25, asyncio=True)

e = l2_eth.l2net_eth ()
e.init ('eth0', 0, 0x0806)

loop = asyncio.get_event_loop()
print("Starting asyncio ETH + 802.15.4 server")

def rdr (l2n):
    """
    :param l2n: l2net_* objet
    :type  l2n: l2net_* (l2net_eth or l2net_154)
    """
    try:
        (ptype, saddr, data) = l2n.recv ()
        if ptype != l2.PkType.PK_NONE:
            print ('Received from: ', saddr)
            print ('Received type: ', data[:10])
    except Exception(e):
        print (e)

loop.add_reader (x.handle (), lambda: rdr (x))
loop.add_reader (e.handle (), lambda: rdr (e))

try:
    loop.run_forever()
except KeyboardInterrupt:
    pass

loop.close()
x.term ()
e.term ()
