"""
This module contains the main class of the HTTP server.
"""
__author__ = 'chavignat'

import asyncio  # Requires python 3.4

from sos_http_server.request import Request
from util.threads import ThreadBase
from util.debug import *


class HTTPServer(ThreadBase):
    """
    This class implements a HTTP server. It relies it asyncio module for parallel request handling, so, many
    of it's methods are coroutines. See asyncio documentation.
    """

    def __init__(self, host=None, port=None, connection_timeout=None):
        """
        Default constructor.
        :param host: Defaults to localhost ('127.0.0.1')
        :param port: Port to listen on. Defaults to port 80.
        :param connection_timeout: time in seconds to wait for the client request to arrive once HTTP
                                   connection is initiated. Defaults to 10 seconds.
        """
        super().__init__()
        self.server = None
        if port is None:
            port = 80
        self.port = port
        if host is None:
            host = '127.0.0.1'
        self.host = host
        if connection_timeout is None:
            self.timeout = 10

    def run(self):
        """
        Entry point for the HTTP server.
        """
        print_debug(dbg_levels.HTTP, 'Starting HTTP server, listening on {}:{}.'.format(self.host, self.port))

        # Create a new event loop for the thread and register it.
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)

        try:
            loop.run_until_complete(self.server_control())
        except:
            # TODO : create a specific exception for server shutdown, handle other exceptions gracefully.
            pass
        finally:
            loop.close()

        print_debug(dbg_levels.HTTP, 'HTTP server dying.')

    @asyncio.coroutine
    def server_control(self):
        """
        This function starts the HTTP server, and then sleeps an wakes up periodically to check if
        the server has been requested to shut down.
        This function is a coroutine.
        """
        yield from asyncio.start_server(self.client_connected, self.host, self.port)
        while True:
            yield from asyncio.sleep(1)
            if not self.keepRunning:
                raise Exception()  # Interrupt execution.

    @asyncio.coroutine
    def client_connected(self, reader, writer):
        """
        Callback function automatically called when a new client connects to the server. Parses client request
        and forwards it to a RequestHandler for processing.
        This function is a coroutine.
        """
        @asyncio.coroutine
        def parse_request():
            """
            Parses a received request into a Request object. This function is a coroutine.
            :return: Request object.
            """
            nonlocal reader  # Capture client_connected reader parameter.
            req = Request()
            while True:
                line = yield from asyncio.wait_for(reader.readline(), self.timeout)
                print(line.decode())
                if line == bytes():
                    break
            return req

        req = yield from parse_request()
        print_debug(dbg_levels.HTTP, 'Incoming connection!')

        # Send reply
