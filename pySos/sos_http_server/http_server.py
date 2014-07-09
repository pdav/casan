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

        # Store the raw payload if there is any. RFC 7230 says:
        # The presence of a message body in a request is signaled by a Content-Length or Transfer-Encoding header field.
        if req.method == b'POST':
            for h in req.headers:
                if h[0] in [b'content-length', b'transfer-encoding']:
                    req.raw_post_args = yield from asyncio.wait_for(reader.read(), self.timeout)
                    break

        return req

    def decode_request(self, req):
        """
        Decodes the payload string of POST requests. Only requests using 'application/x-www-form-urlencoded' encoding
        to pass parameters are supported.
        The algorithm used for decoding is inspired by the one described at:
        http://www.w3.org/html/wg/drafts/html/CR/forms.html#application/x-www-form-urlencoded-decoding-algorithm
        The differences are:
            - We don't expect wide-char unicode characters in the request body, so step 4.4 is skipped.
            - For the same reason, steps 5 and 6 are skipped.
        :param req: Request object to decode
        :return: True if success, False either.
        """
        r = True

        # Look for Content-Type header
        x_www_form_encoding = False
        for h in req.headers:
            if h[0] == b'content-type':
                if h[1].startswith(b'application/x-www-form-urlencoded'):
                    x_www_form_encoding = True
                    break

        if x_www_form_encoding:
            try:
                strings = req.raw_post_args.split(b'&')
                for s in strings:
                    name, value = (bytearray(x) for x in s.split(b'='))
                    name, value = (x.replace(b'+', b' ') for x in [name, value])
                    while b'%' in name:
                        i = name.find(b'%')
                        name[i:i+3] = int(name[i:i+1].decode(), 16)*16 + int(name[i+1:i+2].decode(), 16)
                    while b'%' in value:
                        i = value.find(b'%')
                        v = bytes([int(value[i+1:i+2].decode(), 16)*16 + int(value[i+2:i+3].decode(), 16)])
                        value[i:i+3] = v
                    req.post_args.append((bytes(name), bytes(value)))
            except Exception as e:
                r = False
            if not r:  # In case of failure, restore req to it's initial state
                req.raw_post_args = []
        return r

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
            if self.decode_request(req):
                pass
            else:
                # TODO : send HTTP 400 : Bad Request
                pass
        except asyncio.futures.TimeoutError:
            # Request read timed out, drop it.
            print_debug(dbg_levels.HTTP, 'Incoming request timed out.')

        # Send reply
