"""
This module contains the reply class as well as stock replies used by the server
for error codes.
"""
__author__ = 'chavignat'

from enum import Enum
from sys import stderr
import asyncio


class HTTPCodes(Enum):
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_ACCEPTED = 202,
    HTTP_NO_CONTENT = 204,
    HTTP_MULTIPLE_CHOICES = 300,
    HTTP_MOVED_PERMANENTLY = 301,
    HTTP_MOVED_TEMPORARILY = 302,
    HTTP_NOT_MODIFIED = 304,
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVICE_UNAVAILABLE = 503


class Reply():
    """
    This class represents a HTTP reply to a HTTP request.
    """
    def __init__(self, status_code=None):
        """
        Reply constructor. Constructs an empty reply or a stock reply from a
        status code.
        :param status_code: enumerated type from the HTTPCodes enumeration.
        """
        if status_code is None:
            self.headers = list()
            self.body = None
            self._code = None
        else:
            if not type(status_code) == HTTPCodes:
                raise TypeError
            self._code = status_code
            self.body = stock_replies[self.code]
            self.headers = [(b'Content-Length', str(len(self.body)).encode()),
                            (b'Content-Type', b'text/html')]

    @property
    def code(self):
        return self._code

    @code.setter
    def code(self, new_code):
        """
        Setter for the code property.
        Besides changing the HTTP status code, this method loads the response
        body for stock replies.
        :param new_code: a member of HTTPCodes enumeration, or ValueError will
            be raised.
        """
        if not type(new_code) == HTTPCodes:
            raise ValueError
        self._code = new_code
        if new_code is not HTTPCodes.HTTP_OK:
            self.body = stock_replies[new_code]

    @asyncio.coroutine
    def send(self, writer):
        """
        Sends the reply to the client. This method is a coroutine.
        :param writer: StreamWriter object received by HttpServer.client_connected
        """
        try:
            writer.write(reply_lines[self.code])
            for name, val in self.headers:
                writer.write(name + b': ' + val + b'\r\n')
            writer.write(b'\r\n')
            if self.body is not None:
                writer.write(self.body)
            writer.write_eof()
            yield from writer.drain()
        except Exception as e:
            stderr.write('Error sending reply.\nReason : ' + str(e) + '\n')
            raise

stock_replies, reply_lines = dict(), dict()
stock_replies[HTTPCodes.HTTP_OK] = None
stock_replies[HTTPCodes.HTTP_CREATED] = (b'<html>'
                                         b'<head><title>Created</title></head>'
                                         b'<body><h1>201 Created</h1></body>'
                                         b'</html>')
stock_replies[HTTPCodes.HTTP_ACCEPTED] = (b'<html>'
                                          b'<head><title>Accepted</title></head>'
                                          b'<body><h1>202 Accepted</h1></body>'
                                          b'</html>')
stock_replies[HTTPCodes.HTTP_NO_CONTENT] = (b'<html>'
                                            b'<head><title>No Content</title></head>'
                                            b'<body><h1>204 No Content</h1></body>'
                                            b'</html>')
stock_replies[HTTPCodes.HTTP_MULTIPLE_CHOICES] = (b'<html>'
                                                  b'<head><title>Multiple Choices</title></head>'
                                                  b'<body><h1>300 Multiple Choices</h1></body>'
                                                  b'</html>')
stock_replies[HTTPCodes.HTTP_MOVED_PERMANENTLY] = (b'<html>'
                                                   b'<head><title>Moved Permanently</title></head>'
                                                   b'<body><h1>301 Moved Permanently</h1></body>'
                                                   b'</html>')
stock_replies[HTTPCodes.HTTP_MOVED_TEMPORARILY] = (b'<html>'
                                                   b'<head><title>Moved Temporarily</title></head>'
                                                   b'<body><h1>302 Moved Temporarily</h1></body>'
                                                   b'</html>')
stock_replies[HTTPCodes.HTTP_NOT_MODIFIED] = (b'<html>'
                                              b'<head><title>Not Modified</title></head>'
                                              b'<body><h1>304 Not Modified</h1></body>'
                                              b'</html>')
stock_replies[HTTPCodes.HTTP_BAD_REQUEST] = (b'<html>'
                                             b'<head><title>Bad Request</title></head>'
                                             b'<body><h1>400 Bad Request</h1></body>'
                                             b'</html>')
stock_replies[HTTPCodes.HTTP_UNAUTHORIZED] = (b'<html>'
                                              b'<head><title>Unauthorized</title></head>'
                                              b'<body><h1>401 Unauthorized</h1></body>'
                                              b'</html>')
stock_replies[HTTPCodes.HTTP_FORBIDDEN] = (b'<html>'
                                           b'<head><title>Forbidden</title></head>'
                                           b'<body><h1>403 Forbidden</h1></body>'
                                           b'</html>')
stock_replies[HTTPCodes.HTTP_NOT_FOUND] = (b'<html>'
                                           b'<head><title>Not Found</title></head>'
                                           b'<body><h1>404 Not Found</h1></body>'
                                           b'</html>')
stock_replies[HTTPCodes.HTTP_INTERNAL_SERVER_ERROR] = (b'<html>'
                                                       b'<head><title>Internal Server Error</title></head>'
                                                       b'<body><h1>500 Internal Server Error</h1></body>'
                                                       b'</html>')
stock_replies[HTTPCodes.HTTP_NOT_IMPLEMENTED] = (b'<html>'
                                                 b'<head><title>Not Implemented</title></head>'
                                                 b'<body><h1>501 Not Implemented</h1></body>'
                                                 b'</html>')
stock_replies[HTTPCodes.HTTP_BAD_GATEWAY] = (b'<html>'
                                             b'<head><title>Bad Gateway</title></head>'
                                             b'<body><h1>502 Bad Gateway</h1></body>'
                                             b'</html>')
stock_replies[HTTPCodes.HTTP_SERVICE_UNAVAILABLE] = (b'<html>'
                                                     b'<head><title>Service Unavailable</title></head>'
                                                     b'<body><h1>503 Service Unavailable</h1></body>'
                                                     b'</html>')

reply_lines[HTTPCodes.HTTP_OK] = b'HTTP/1.0 200 OK\r\n'
reply_lines[HTTPCodes.HTTP_CREATED] = b'HTTP/1.0 201 Created\r\n'
reply_lines[HTTPCodes.HTTP_ACCEPTED] = b'HTTP/1.0 202 Accepted\r\n'
reply_lines[HTTPCodes.HTTP_NO_CONTENT] = b'HTTP/1.0 204 No Content\r\n'
reply_lines[HTTPCodes.HTTP_MULTIPLE_CHOICES] = b'HTTP/1.0 300 Multiple Choices\r\n'
reply_lines[HTTPCodes.HTTP_MOVED_PERMANENTLY] = b'HTTP/1.0 301 Moved Permanently\r\n'
reply_lines[HTTPCodes.HTTP_MOVED_TEMPORARILY] = b'HTTP/1.0 302 Moved Temporarily\r\n'
reply_lines[HTTPCodes.HTTP_NOT_MODIFIED] = b'HTTP/1.0 304 Not Modified\r\n'
reply_lines[HTTPCodes.HTTP_BAD_REQUEST] = b'HTTP/1.0 400 Bad Request\r\n'
reply_lines[HTTPCodes.HTTP_UNAUTHORIZED] = b'HTTP/1.0 401 Unauthorized\r\n'
reply_lines[HTTPCodes.HTTP_FORBIDDEN] = b'HTTP/1.0 403 Forbidden\r\n'
reply_lines[HTTPCodes.HTTP_NOT_FOUND] = b'HTTP/1.0 404 Not Found\r\n'
reply_lines[HTTPCodes.HTTP_INTERNAL_SERVER_ERROR] = b'HTTP/1.0 500 Internal Server Error\r\n'
reply_lines[HTTPCodes.HTTP_NOT_IMPLEMENTED] = b'HTTP/1.0 501 Not Implemented\r\n'
reply_lines[HTTPCodes.HTTP_BAD_GATEWAY] = b'HTTP/1.0 502 Bad Gateway\r\n'
reply_lines[HTTPCodes.HTTP_SERVICE_UNAVAILABLE] = b'HTTP/1.0 503 Service Unavailable\r\n'
