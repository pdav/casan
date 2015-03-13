#!/usr/bin/env python3

# Must be run as root
# Test Ethernet and 802.15.4 receive methods with asyncio

import asyncio

import aiohttp
import aiohttp.web

from casan import l2
from casan import l2_154
from casan import l2_eth

@asyncio.coroutine
def handle(request):
    name = request.match_info.get('name', "Anonymous")
    text = "Hello, " + name
    return aiohttp.web.Response(body=text.encode('utf-8'))


@asyncio.coroutine
def init(loop):
    app = aiohttp.web.Application(loop=loop)
    app.router.add_route('GET', '/{name}', handle)
    srv = yield from loop.create_server(app.make_handler(), '0.0.0.0', 8080)
    print("Server started at http://0.0.0.0:8080")
    return srv

x = l2_154.l2net_154 ()
l = x.init ('/dev/digi', 'xbee', 0, '12:34', 'ca:fe', 25, asyncio=True)
if l is False:
    print ('Error in 154 init')
    sys.exit (1)

e = l2_eth.l2net_eth ()
l = e.init ('eth0', 0, 0x0806)
if l is False:
    print ('Error in eth init')
    sys.exit (1)

def rdr (l2n):
    """
    :param l2n: l2net_* objet
    :type  l2n: l2net_* (l2net_eth or l2net_154)
    """
    try:
        r = l2n.recv ()
        if ptype is not None:
            (ptype, saddr, data) = r
            print ('Received from: ', saddr)
            print ('Received type: ', data[:10])
    except Exception(e):
        print (e)

loop = asyncio.get_event_loop()
print("Starting asyncio HTTP + ETH + 802.15.4 server")

loop.add_reader (x.handle (), lambda: rdr (x))
loop.add_reader (e.handle (), lambda: rdr (e))

loop.run_until_complete(init(loop))

try:
    loop.run_forever()
except KeyboardInterrupt:
    pass


try:
    loop.run_forever()
except KeyboardInterrupt:
    pass

loop.close()
x.term ()
e.term ()
