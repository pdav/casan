"""
This module contains the main class of the HTTP server.
"""
__author__ = 'chavignat'

import asyncio  # Requires python 3.4

from util.threads import ThreadBase
from util.debug import *


class HTTPServer(ThreadBase):
    """
    This class implements a HTTP server.
    """

    def __init__(self, host=None, port=None):
        """
        Default constructor.
        :param host: Defaults to localhost ('127.0.0.1')
        :param port: Port to listen on. Defaults to port 80.
        """
        super().__init__()
        self.server = None
        if port is None:
            port = 80
        self.port = port
        if host is None:
            host = '127.0.0.1'
        self.host = host

    def run(self):
        """
        Entry point for the HTTP server.
        """
        print_debug(dbg_levels.HTTP, 'Starting HTTP server, listening on {}:{}.'.format(self.host, self.port))

        # Create a new event loop for the thread and register it.
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)

        try:
            loop.run_until_complete(self.http_server())
        except:
            pass
        finally:
            loop.close()

        print_debug(dbg_levels.HTTP, 'HTTP server dying.')

    @asyncio.coroutine
    def http_server(self):
        """
        This function starts the HTTP server, and then sleeps an wakes up periodically to check if
        the server has been requested to shut down.
        """
        yield from asyncio.start_server(self.client_connected, self.host, self.port)
        while True:
            yield from asyncio.sleep(1)
            if not self.keepRunning:
                raise Exception()  # Interrupt execution.

    @asyncio.coroutine
    def client_connected(self, reader, writer):
        """
        Callback function automatically called when a new client connects to the server.
        This function is a coroutine.
        """
        def parse_request(reader):
            pass
        print_debug(dbg_levels.HTTP, 'Incoming connection!')

        # Send reply
