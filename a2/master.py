# import conf

import asyncio
import aiohttp
import aiohttp.web

import l2_eth
import l2_154
import msg


class Master:
    """
    """

    def __init__ (self, conf):
        """
        Initialize a master object with a parsed configuration file
        :param conf:parsed configuration
        :type  conf:class conf.Conf
        """
        self._conf = conf
        self._waiters = []

    ######################################################################
    # Initialization
    ######################################################################

    def run (self):
        """
        Initialize devices and various sockets and run the master loop
        """

        #
        # Web application handlers for various namespaces
        #

        app = aiohttp.web.Application ()

        for ns in self._conf.namespaces:
            uri = self._conf.namespaces [ns]

            if ns == 'admin':
                uri += '/{name}'
                app.router.add_route ('GET', uri, self.handle_admin)
            elif ns == 'casan':
                uri += '/{name}'
                app.router.add_route ('GET', uri, self.handle_casan)
            elif ns == 'well-known':
                app.router.add_route ('GET', uri, self.handle_well_known)
            else:
                raise RuntimeError ('Unknown configured namespace ' + ns)

        loop = asyncio.get_event_loop ()

        #
        # Start HTTP servers
        #

        for (scheme, addr, port) in self._conf.http:
            ############# XXX : we should use the SCHEME
            f = loop.create_server (app.make_handler (), addr, port)
            loop.run_until_complete (f)

        #
        # Configure L2 networks
        #

        for (type, dev, mtu, sub) in self._conf.networks:
            if type == 'ethernet':
                (ethertype, ) = sub
                l2n = l2_eth.l2net_eth ()
                r = l2n.init (dev, mtu, ethertype)
            elif type == '802.15.4':
                (subtype, addr, panid, channel) = sub
                l2n = l2_154.l2net_154 ()
                r = l2n.init (dev, subtype, mtu, addr, panid, channel,
                            asyncio=True)
            else:
                raise RuntimeError ('Unknown network type ' + type)

            if r is False:
                raise RuntimeError ('Error in network ' + type + ' init')

#            loop.call_later (self._conf.timers ['firsthello'],
#                             lambda: self.send_hello (loop, l2n))
            loop.add_reader (l2n.handle (), lambda: self.l2reader (l2n))

        print ('Server ready')

        #
        # Main loop
        #

        try:
            loop.run_forever ()
        except KeyboardInterrupt:
            pass

    def send_hello (self, loop, l2n):
        m = msg.Msg ()
        m.peer = l2n.broadcast
        m.msgtype = msg.Msg.MsgTypes.MT_NON
        m.msgcode = msg.Msg.MsgCodes.MC_POST
        # XXX : set a better id
        m.mk_ctrl_hello (0)
        m.coap_encode ()
        l2n.send (m.peer, m.bmsg)
        print ('Sent Hello')
        loop.call_later (self._conf.timers ['hello'],
                         lambda: self.send_hello (loop, l2n))


    ######################################################################
    # HTTP handle routines
    ######################################################################

    def handle_admin (self, request):
        name = request.match_info ['name']

        if name == 'index':
            r = """<html><title>Welcome to CASAN</title>
                    <body>
                        <h1>Welcome to CASAN</h1>
                        <ul>
                            <li><a href="conf">configuration</a></li>
                            <li><a href="run">running status</a></li>
                            <li><a href="slave">slave status</a></li>
                        </ul>
                    </body></html>"""

        elif name == 'conf':
            r = '<html><body><pre>' + str(self._conf) + '</pre></body></html>'

        elif name == 'run':
            r = '<html><body><pre>' + str(self._conf) + '</pre></body></html>'

        elif name == 'slave':
            r = '<html><body><pre>' + str(self._conf) + '</pre></body></html>'

        else:
            raise aiohttp.web.HTTPNotFound ()

        return aiohttp.web.Response (body=r.encode ('utf-8'))

    def handle_well_known (self, request):
        return aiohttp.web.Response (body=b"WELL")

    @asyncio.coroutine
    def handle_casan (self, request):
        try:
            r = yield from asyncio.shield (asyncio.wait_for (self.machin (), 3))
        except asyncio.TimeoutError:
            print ('Got exception')
            r = aiohttp.web.Response (body=b"TIMEOUT")
        return r

    @asyncio.coroutine
    def machin (self):
        ev = asyncio.Event ()
        self._waiters.append (ev)
        yield from ev.wait ()
        print ('Awaken')
        return aiohttp.web.Response (body=b"CASAN")

    ######################################################################
    # L2 handle routines
    ######################################################################

    # @asyncio.coroutine
    def l2reader (self, l2n):
        """
        :param l2n: l2net_* objet
        :type  l2n: l2net_* (l2net_eth or l2net_154)
        """

        r = l2n.recv ()
        if r is not None:
            m = msg.Msg ()
            if m.decode (r):
                print ('Received ', str (m))
                if m.is_casan_ctrl_msg ():
                    print ('CTRL')

                # wake-up all waiters
                for e in self._waiters:
                    e.set ()
                self._waiters = []

            else:
                print ('DECODING FAILED')
