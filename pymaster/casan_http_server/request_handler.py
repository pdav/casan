"""
This module contains the CASANRequestHandler class.
"""
__author__ = 'chavignat'

import asyncio
from urllib.parse import unquote_plus
from sys import stderr

from .reply import HTTPCodes

class CASANRequestHandler:
    """
    This class is a function object. It is in charge of handling a request received by the HTTP server,
    and to forward it to CASAN if needed.
    """
    def __init__(self, master):
        """
        Constructs a CASANRequestHandler object.
        :param master: reference to the instance of the Master class.
        """
        self.master = master

    @asyncio.coroutine
    def __call__(self, req, rep):
        """
        Call operator. This method is a coroutine.
        :param req: request to handle
        """
        # First, attempt to decode URI
        try:
            # Unfortunately, unquote_plus doesn't accept bytes
            uri = unquote_plus(req.uri.decode())

            # Make sure path name is absolute and does not represent a directory.
            if len(uri) == 0 or uri[0] != '/' or '..' in uri or uri.endswith('/'):
                rep.code = HTTPCodes.HTTP_BAD_REQUEST
            else:
                yield from self.master.handle_http(uri, req, rep)

        except Exception as e:
            # Unhandled errors are caught here and result in code HTTP 500 being sent.
            stderr.write('Error while processing request :\n' + str(e) + '\n')
            rep.code = HTTPCodes.HTTP_INTERNAL_SERVER_ERROR
