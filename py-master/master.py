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

import observer
import engine
import cache
import slave
import msg
import option
import g


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
        self._cache = None
        self._addevlog = False

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
                uri += '/{action}'
                app.router.add_route ('GET', uri, self.handle_admin)
            elif ns == 'casan':
                uri += '/{sid:\d+}/{respath:[^{}]+}'
                for meth in ['POST', 'DELETE', 'GET', 'PUT']:
                    app.router.add_route (meth, uri, self.handle_casan)
            elif ns == 'observe':
                uri += '/{token}/{sid:\d+}/{respath:[^{}]+}'
                for meth in ['POST', 'DELETE', 'GET']:
                    app.router.add_route (meth, uri, self.handle_observe)
            elif ns == 'evlog':
                uri += '/{op}'
                app.router.add_route ('GET', uri, self.handle_evlog)
            elif ns == 'well-known':
                app.router.add_route ('GET', uri, self.handle_well_known)
            else:
                raise RuntimeError ('Unknown configured namespace ' + ns)

        loop = asyncio.get_event_loop ()

        #
        # Initialize event logger
        #

        if self._conf.namespaces ['evlog']:
            self._addevlog = self._conf.evlog ['add']
            g.e.set_size (self._conf.evlog ['size'])
        g.e.add ('master', 'starting')

        #
        # Initialize cache
        #

        self._cache = cache.Cache (loop, self._conf.timers ['cacheclean'])

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

        g.d.m ('master', 'Server ready')

        #
        # Main loop
        #

        try:
            loop.run_forever ()
        except KeyboardInterrupt:
            g.d.m ('master', 'Interrupt')
            raise

    ######################################################################
    # HTTP handler for admin namespace
    ######################################################################

    def handle_admin (self, request):
        """
        Handle HTTP requests for the admin namespace
        :param request: an HTTP request
        :type  request: aiohttp.web.Request object
        :return: a HTTP response
        :rtype: aiohttp.web.Response object
        """

        g.d.m ('http', 'HTTP admin request {}'.format (request))
        action = request.match_info ['action']

        if action == 'index':
            r = """<html><title>Welcome to CASAN</title>
                    <body>
                        <h1>Welcome to CASAN</h1>
                        <ul>
                            <li><a href="conf">configuration</a></li>
                            <li><a href="run">running status</a></li>
                            <li><a href="slave">slave status</a></li>
                            <li><a href="cache">message cache contents</a></li>
                        </ul>
                    </body></html>"""

        elif action == 'conf':
            r = '<html><body><pre>'
            r += html.escape (str (self._conf))
            r += '</pre></body></html>'

        elif action == 'run':
            r = """<html><title>Current status</title>
                    <body>
                        <h1>Current CASAN status</h1>
                        """
            r += self._engine.html ()
            r += '</body></html>'

        elif action == 'cache':
            r = """<html><title>Cache contents</title>
                    <body>
                        <h1>CASAN cache contents</h1>
                        """
            r += self._cache.html ()
            r += '</body></html>'

        elif action == 'slave':
            r = '<html><body><pre>' + str(self._conf) + '</pre></body></html>'

        else:
            raise aiohttp.web.HTTPNotFound ()

        return aiohttp.web.Response (body=r.encode ('utf-8'))

    ######################################################################
    # HTTP handler for evlog namespace
    ######################################################################

    def _process_get_evlog (self, param):
        """
        Process an event log read request
        :param param: HTTP GET parameters
        :type  param: aiohttp MultiDictProxy
        """

        since = 0
        if 'since' in param:
            try:
                since = int (param ['since'])
            except ValueError:
                raise aiohttp.web.HTTPBadRequest ()
        r = g.e.get_json (since)
        ct = "application/json"
        return aiohttp.web.Response (body=r.encode ('utf-8'), content_type=ct)

    def _process_add_evlog (self, param):
        """
        Process an event log write request
        :param param: HTTP GET parameters
        :type  param: aiohttp MultiDictProxy
        """

        if not self._addevlog:
            raise aiohttp.web.HTTPUnauthorized ()

        if 'date' in param:
            try:
                dt = int (param ['date'])
            except ValueError:
                raise aiohttp.web.HTTPBadRequest ()
        else:
            dt = None

        if 'src' not in param:
            raise aiohttp.web.HTTPBadRequest ()
        src = param ['src']

        if 'msg' not in param:
            raise aiohttp.web.HTTPBadRequest ()
        m = param ['msg']

        g.e.add (src, m, dt=dt)

        r = '<html><body><pre>done</pre></body></html>'

        return aiohttp.web.Response (body=r.encode ('utf-8'))

    def handle_evlog (self, request):
        """
        Handle HTTP requests for the evlog namespace
        :param request: an HTTP request
        :type  request: aiohttp.web.Request object
        :return: a HTTP response
        :rtype: aiohttp.web.Response object
        """

        g.d.m ('http', 'HTTP evlog request {}'.format (request))
        op = request.match_info ['op']
        param = request.GET

        if op == 'get':
            r = self._process_get_evlog (param)
        elif op == 'add':
            r = self._process_add_evlog (param)
        else:
            raise aiohttp.web.HTTPNotFound ()

        return r

    ######################################################################
    # HTTP handler for well-known namespace
    ######################################################################

    def handle_well_known (self, request):
        """
        Handle HTTP requests for the well-known namespace
        :param request: an HTTP request
        :type  request: aiohttp.web.Request object
        :return: HTTP response
        :rtype: aiohttp.web.Response object
        """

        g.d.m ('http', 'HTTP well-known request {}'.format (request))
        rl = self._engine.resource_list ().encode ()
        return aiohttp.web.Response (body=rl)

    ######################################################################
    # HTTP handler for casan namespace
    ######################################################################

    @asyncio.coroutine
    def handle_casan (self, request):
        """
        Handle HTTP requests for a slave (casan namespace)
        :param request: an HTTP request
        :type  request: aiohttp.web.Request object
        :return: a HTTP response
        :rtype: aiohttp.web.Response object
        """

        g.d.m ('http', 'HTTP casan request {}'.format (request))
        meth = request.method
        sid = request.match_info ['sid']
        vpath = request.match_info ['respath'].split ('/')

        #
        # Find slave and resource
        #

        sl = self._engine.find_slave (sid)
        if sl is None:
            raise aiohttp.web.HTTPNotFound ()

        if sl is None or not sl.isrunning ():
            raise aiohttp.web.HTTPNotFound ()

        res = sl.find_resource (vpath)
        if res is None:
            return None

        #
        # Build request
        #

        mreq = msg.Msg ()
        mreq.peer = sl.addr
        mreq.l2n = sl.l2n
        mreq.msgtype = msg.Msg.Types.CON

        if meth == 'GET':
            mreq.msgcode = msg.Msg.Codes.GET
        elif meth == 'POST':
            mreq.msgcode = msg.Msg.Codes.POST
        elif meth == 'DELETE':
            mreq.msgcode = msg.Msg.Codes.DELETE
        elif meth == 'PUT':
            mreq.msgcode = msg.Msg.Codes.PUT
        else:
            raise aiohttp.web.HTTPNotFound ()

        up = option.Option.Codes.URI_PATH
        for p in vpath:
            mreq.optlist.append (option.Option (up, optval=p))

        #
        # Is the request already present in the cache?
        #

        mc = self._cache.get (mreq)
        if mc is not None:
            # Request found in the cache
            mreq = mc
            mrep = mc.req_rep

        else:
            # Request not found in the cache: send it and wait for a result
            mrep = yield from mreq.send_request ()

            if mrep is not None:
                # Add the request (and the linked answer) to the cache
                self._cache.add (mreq)
            else:
                return aiohttp.web.HTTPRequestTimeout ()

        # Python black magic: aiohttp.web.Response expects a
        # bytes argument, but mrep.payload is a bytearray
        pl = mrep.payload.decode ().encode ()
        r = aiohttp.web.Response (body=pl)

        return r

    ######################################################################
    # HTTP handler for observe namespace
    ######################################################################

    @asyncio.coroutine
    def handle_observe (self, request):
        """
        Handle HTTP request to observe a resource. Parameter are:
        - HTTP method:
            - POST: add an observer
            - DELETE: remove an observer
            - GET: get the current resource value
        - path: includes /token/path/to/slave/and/resource
            (e.g.: /foo/169/temp)
        - query string: to send to the resource to be observed (for POST)
        The parameters may be given with the 'curl' command. E.g.:
            curl -X POST -d "" http://.../token/res
            curl -X DELETE http://..../token/res
            curl -X GET http://.../token/res
        :param request: an HTTP request
        :type  request: aiohttp.web.Request object
        :return: a HTTP response
        """

        g.d.m ('http', 'HTTP observe request {}'.format (request))
        meth = request.method
        token = request.match_info ['token'].encode ()
        sid = request.match_info ['sid']
        vpath = request.match_info ['respath'].split ('/')
        qs = request.query_string

        #
        # Find slave
        #

        sl = self._engine.find_slave (sid)
        if sl is None:
            raise aiohttp.web.HTTPNotFound ()

        obs = sl.find_observer (vpath, token)

        #
        # Find slave and resource
        #

        ct = 'text/plain;; charset=utf-8'
        r = None
        if obs is None:
            if meth == 'POST':
                obs = observer.Observer (sl, vpath, token)
                sl.add_observer (obs)
                r = 'Observer created'
        else:
            if meth == 'POST':
                r = 'Observer already created'
            elif meth == 'DELETE':
                sl.del_observer (obs)
                r = 'Observer deleted'
            elif meth == 'GET':
                (r, ct) = obs.get_value ()
                if r is None:
                    raise aiohttp.web.HTTPNoContent ()

        if r is None:
            raise aiohttp.web.HTTPNotFound ()
            
        return aiohttp.web.Response (body=r.encode ('utf-8'), content_type=ct)
