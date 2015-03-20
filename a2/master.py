"""
This module contains the Casan master class
"""

import html

try:
    import ssl
except ImportError:
    ssl = None

import asyncio
import aiohttp
import aiohttp.web

import engine
import slave


class Master (object):
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
                uri += '/{name:[^{}]+}'
                app.router.add_route ('GET', uri, self.handle_casan)
            elif ns == 'well-known':
                app.router.add_route ('GET', uri, self.handle_well_known)
            else:
                raise RuntimeError ('Unknown configured namespace ' + ns)

        loop = asyncio.get_event_loop ()

        #
        # Start HTTP servers
        #

        for (scheme, addr, port, sslcert, sslkey) in self._conf.http:
            sct = None
            if scheme == 'https':
                if not ssl:
                    raise RuntimeError ('ssl module not available for https')
                sct = ssl.SSLContext (ssl.PROTOCOL_SSLv23)
                sct.load_cert_chain (sslcert, sslkey)

            f = loop.create_server (app.make_handler (), addr, port, ssl=sct)
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
            raise

    ######################################################################
    # HTTP handle routines
    ######################################################################

    def handle_admin (self, request):
        """
        Handle HTTP requests for the admin namespace
        :param request: an HTTP request
        :type  request: aiohttp.web.Request object
        :return: a HTTP response
        :rtype: aiohttp.web.Response object
        """

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
        """
        Handle HTTP requests for the well-known namespace
        :param request: an HTTP request
        :type  request: aiohttp.web.Request object
        :return: a HTTP response
        :rtype: aiohttp.web.Response object
        """
        return aiohttp.web.Response (body=b"WELL")

    @asyncio.coroutine
    def handle_casan (self, request):
        """
        Handle HTTP requests for a slave (casan namespace)
        :param request: an HTTP request
        :type  request: aiohttp.web.Request object
        :return: a HTTP response
        :rtype: aiohttp.web.Response object
        """

        name = request.match_info ['name']
        vpath = name.split ('/')

        try:
            sid = int (vpath [0])
        except:
            raise aiohttp.web.HTTPNotFound ()

        sl = self._engine.find_slave (sid)
        if sl is None or sl.status != slave.Slave.Status.RUNNING:
            raise aiohttp.web.HTTPNotFound ()

        del (vpath [0])
        res = sl.find_resource (vpath)
        if res is None:
            raise aiohttp.web.HTTPNotFound ()


        # XXX

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
