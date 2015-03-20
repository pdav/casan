import html

import asyncio
import aiohttp
import aiohttp.web

import engine


class Master:
    """
    Main casand class
    """

    def __init__ (self, conf):
        """
        Initialize a master object with a parsed configuration file
        :param conf: parsed configuration
        :type  conf: class conf.Conf
        """
        self._conf = conf
        self._engine = None

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
        # Start CASAN engine (i.e. initialize events for the event loop)
        #

        self._engine = engine.Engine ()
        self._engine.start (self._conf, loop)

        print ('Server ready')

        #
        # Main loop
        #

        try:
            loop.run_forever ()
        except KeyboardInterrupt:
            pass

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
            r = '<html><body><pre>'
            r += html.escape (str (self._conf))
            r += '</pre></body></html>'

        elif name == 'run':
            r = """<html><title>Current status</title>
                    <body>
                        <h1>Current CASAN status</h1>
                        """
            r += self._engine.html ()
            r += '</pre></body></html>'

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
            r = yield from asyncio.shield (asyncio.wait_for (self.machin(), 3))
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
