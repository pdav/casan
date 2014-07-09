"""
This module contains the main class of the HTTP server.
"""
__author__ = 'chavignat'

import asyncio  # Requires python 3.4
from itertools import takewhile
import re

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
    def parse_request(self, reader):
        """
        Parses a received request into a Request object. This function is a coroutine.
        :return: Request object, or None if a problem was encountered when parsing the request.
        """
        req = Request()

        # Parse the request line (method, URI, and http version)
        first_line = yield from asyncio.wait_for(reader.readline(), self.timeout)
        def take_until_space(b):
            return takewhile(lambda c: c != b' '[0], b)
        req.method = bytes(take_until_space(first_line))
        req.uri = bytes(take_until_space(first_line[len(req.method)+1:]))
        http_ver = bytes(first_line[len(req.method)+len(req.uri)+2:])
        http_ver_re = re.compile(b'^HTTP/[01]\.[019]\r$')
        if http_ver_re.match(http_ver):
            http_ver = http_ver[5:]
        else:
            return None
        # Catch wrong version numbers (0.1, 1.9, etc...) here
        req.http_ver_major = int(http_ver[0:1])
        req.http_ver_minor = int(http_ver[2:3])
        if req.http_ver_major == 0:
            if not req.http_ver_minor == 9:
                return None
        elif req.http_ver_major == 1:
            if req.http_ver_minor not in [0,1]:
                return None

        # Parse header fields
        # Boost's server2 allows multiline header fields, which was deprecated by RFC7230, so I didn't
        # implement that behaviour.
        def take_until_colon(b):
            return takewhile(lambda c: c != b':'[0], b)
        while True:
            line = yield from asyncio.wait_for(reader.readline(), self.timeout)
            if line == b'\r\n':  # Catch header terminator
                break
            http_header_re = re.compile(b'^[a-zA-Z0-9-]+: ?.+ ?\r$')
            if http_header_re.match(line):
                # RFC7230 : "Each header field consists of a case-insensitive field name"
                header_name = bytes(take_until_colon(line)).lower()
                header_value = line[len(header_name)+1:]
                header_value = header_value.lstrip(b' ')  # Remove optional leading whitespace
                header_value = header_value.rstrip(b' \r\n')  # Remove CRLF and optional trailing whitespace
                req.headers.append((header_name, header_value))
            else:
                return None

        # Store the raw payload if the method is POST.
        if req.method == 'POST':
            payload = yield from asyncio.wait_for(reader.read(), self.timeout)
            req.raw_post_args = payload

        return req

    @asyncio.coroutine
    def client_connected(self, reader, writer):
        """
        Callback function automatically called when a new client connects to the server. Parses client request
        and forwards it to a RequestHandler for processing.
        This function is a coroutine.
        """
        try:
            req = yield from self.parse_request(reader)
            print_debug(dbg_levels.HTTP, 'Incoming connection!')
            print_debug(dbg_levels.HTTP, 'Method : {req.method}\nURI : {req.uri}\nHTTP '
                                         'version : {req.http_ver_major}.{req.http_ver_minor}'.format(req=req))
            for h in req.headers:
                print_debug(dbg_levels.HTTP, 'Header name : {}\nHeader value : {}'.format(h[0], h[1]))
            print_debug(dbg_levels.HTTP, 'Raw payload :\n{}'.format(req.raw_post_args))
        except asyncio.futures.TimeoutError:
            # Request read timed out, drop it.
            print_debug(dbg_levels.HTTP, 'Incoming request timed out.')

        # Send reply
