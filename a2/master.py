# import conf

import asyncio
import aiohttp
import aiohttp.web


class Master:
    """
    """

    def __init__ (self, conf):
        """
        Initialize a master object with a parsed configuration file
        :param conf:parsed configuration
        :type  conf:class conf.Conf
        """
        self.conf_ = conf

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

        for ns in self.conf_.namespaces:
            uri = self.conf_.namespaces [ns]

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

        for (scheme, addr, port) in self.conf_.http:
            ############# XXX : we should use the SCHEME
            f = loop.create_server (app.make_handler (), addr, port)
            loop.run_until_complete (f)

        try:
            loop.run_forever ()
        except KeyboardInterrupt ():
            pass

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
            r = '<html><body><pre>' + str(self.conf_) + '</pre></body></html>'

        elif name == 'run':
            r = '<html><body><pre>' + str(self.conf_) + '</pre></body></html>'

        elif name == 'slave':
            r = '<html><body><pre>' + str(self.conf_) + '</pre></body></html>'

        else:
            # r = "404, c'était une belle voiture"
            raise aiohttp.web.HTTPNotFound ()

        return aiohttp.web.Response (body=r.encode (('utf-8'))

    def handle_well_known (self, request):
        return aiohttp.web.Response (body=b"WELL")

    @asyncio.coroutine
    def handle_casan (self, request):
        return aiohttp.web.Response (body=b"CASAN")
