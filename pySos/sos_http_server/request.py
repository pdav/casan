"""
This module contains the Request class.
"""
__author__ = 'chavignat'


class Request:
    """
    This class represents a HTTP request. All fields are internally stored as bytes, so
    decode them first if you need to work with them as strings.
    """
    def __init__(self):
        """
        Default constructor, initializes all fields to empty values.
        """
        self.method = None
        self.uri = None
        self.http_ver_major, self.http_ver_minor = None, None
        self.headers = []
        self.post_args = []
        self.raw_post_args = bytes()
