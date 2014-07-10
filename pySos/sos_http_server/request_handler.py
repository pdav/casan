"""
This module contains the SOSRequestHandler class.
"""
__author__ = 'chavignat'

from urllib.parse import unquote_plus

from .reply import Reply, HTTPCodes

class SOSRequestHandler:
    """
    This class is a function object. It is in charge of handling a request received by the HTTP server,
    and to forward it to SOS if needed.
    """
    def __init__(self, master):
        """
        Constructs a SOSRequestHandler object.
        :param master: reference to the instance of the Master class.
        """
        self.master = master

    def __call__(self, req, rep):
        """
        Call operator.
        :param req: request to handle
        :return:
        """
        # First, attempt to decode URI
        try:
            # Unfortunately, unquote_plus doesn't accept bytes
            uri = unquote_plus(req.uri.decode()).encode()

            # Make sure path name is absolute and does not represent a directory.
            if len(uri) == 0 or uri[0] != b'/' or b'..' in uri:
                rep = Reply(HTTPCodes.HTTP_BAD_REQUEST)
                pass
            else:
                self.master.handle_http(uri, req)

        except Exception as e:
            pass