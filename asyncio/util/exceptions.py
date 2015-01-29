"""
This module contains specific exceptions used by SOS and the HTTP server.
"""

class ServerShutdownRequestException(Exception):
    def _init__(self, *args, **kwargs):
        """
        Default constructor, just forwards the call to Exception's default constructor.
        """
        super().__init__(self)