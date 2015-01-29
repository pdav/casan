"""
This module contains the main class of the HTTP server.
"""
__author__ = 'chavignat'

import asyncio  # Requires python 3.4
from itertools import takewhile
import re
from urllib.parse import parse_qsl
from sys import stderr

from sos_http_server.request import Request
from util.threads import ThreadBase
from util.debug import print_debug, dbg_levels
from .request_handler import SOSRequestHandler
from util.exceptions import ServerShutdownRequestException
from .reply import Reply, HTTPCodes


class HTTPServer(ThreadBase):
    """
    This class implements a HTTP server. It relies on asyncio module for parallel request handling, so, many
    of it's methods are coroutines. See asyncio documentation.
    """

    def __init__(self, master, host=None, port=None, connection_timeout=None):
        """
        Default constructor.
        :param master: reference to the instance of the Master class.
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
        self.request_handler = SOSRequestHandler(master)
        # TODO : wait for the sender to notify when it's ready and serve HTTP 503 while not ready.
        self.ready = False

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
        except ServerShutdownRequestException:
            pass
        except Exception as e:
            stderr.write('Error : unhandled exception in HTTP server.\n'
                         'Reason : ' + str(e))
            raise
        finally:
            loop.close()

        print_debug(dbg_levels.HTTP, 'HTTP server dying.')

    @asyncio.coroutine
    def server_control(self):
        """
        This function starts the HTTP server, and then sleeps and wakes up periodically to check if
        the server has been requested to shut down.
        This function is a coroutine.
        """
        yield from asyncio.start_server(self.client_connected, self.host, self.port)
        while True:
            yield from asyncio.sleep(1)
            if not self.keep_running:
                raise ServerShutdownRequestException()  # Interrupt execution.

    @asyncio.coroutine
    def parse_request(self, reader):
        """
        Parses a received request into a Request object. This function is a coroutine.
        :param reader: StreamReader object.
        :return: Request object, or None if a problem was encountered when parsing the request.
        """
        req = Request()

        # Parse the request line (method, URI, and http version)
        first_line = yield from asyncio.wait_for(reader.readline(), self.timeout)

        def take_until_space(b):
            """
            Returns a substring from the input stopping at the first space encountered.
            :param b: bytes object
            :return: substring of b up to it's first space.
            """
            return takewhile(lambda c: c != b' '[0], b)
        req.method = bytes(take_until_space(first_line))
        req.uri = bytes(take_until_space(first_line[len(req.method)+1:]))
        http_ver = bytes(first_line[len(req.method)+len(req.uri)+2:])
        # Note : python re module keeps a cache of the last used regular expressions
        # so this regex should be compiled only once.
        # If you want to be sure, it could be made a class attribute.
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
            if req.http_ver_minor not in [0, 1]:
                return None

        # Parse header fields
        # Boost's server2 allows multiline header fields, which was deprecated by RFC7230, so I didn't
        # implement that behaviour.
        def take_until_colon(b):
            """
            Returns a substring from the input stopping at the first space encountered.
            :param b: bytes object
            :return: substring of b up to it's first space.
            """
            return takewhile(lambda c: c != b':'[0], b)
        http_header_re = re.compile(b'^[a-zA-Z0-9-]+: ?.+ ?\r$')
        while True:
            line = yield from asyncio.wait_for(reader.readline(), self.timeout)
            if line == b'\r\n':  # Catch header terminator
                break
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
        # Let's assume content-length is present.
        if req.method == b'POST':
            for h in req.headers:
                if h[0] == b'content-length':
                    line = yield from asyncio.wait_for(reader.readexactly(int(h[1].decode())), self.timeout)
                    req.raw_post_args += line
                    reader.feed_eof()
                    break

        return req

    @staticmethod
    def decode_request(req):
        """
        Decodes the payload string of POST requests. Only requests using 'application/x-www-form-urlencoded' encoding
        to pass parameters are supported.
        :param req: Request object to decode
        :return: True if success, False either.
        """
        r = True
        try:
            req.post_args = parse_qsl(req.raw_post_args.decode())
        except ValueError:
            r = False
        return r

    @asyncio.coroutine
    def client_connected(self, reader, writer):
        """
        Callback function automatically called when a new client connects to the server. Parses client request
        and forwards it to a RequestHandler for processing.
        This function is a coroutine.
        :param reader: StreamReader object.
        :param writer: StreamWriter object.
        """
        try:
            req = yield from self.parse_request(reader)
            rep = Reply()
            print_debug(dbg_levels.HTTP, 'Incoming connection!')
            if self.decode_request(req):
                yield from self.request_handler(req, rep)
            else:
                rep.code = HTTPCodes.HTTP_BAD_REQUEST
            yield from rep.send(writer)
        except asyncio.futures.TimeoutError:
            # Request read timed out, drop it.
            print_debug(dbg_levels.HTTP, 'Incoming request timed out.')
