"""
This module contains the Observer class.
"""

import html

import option


class Observer (object):
    """
    An object of class Observer represents a declared observer.
    An observer observes a resource on a slave with some parameters.
    Resource and slave may not always exist: an observer may be created
    when the slave is not up, or the slave may stop at any time (to
    preserve battery life for example) during the observation period.
    An Observer object keeps the following informations:
    - slave
    - resource name
    - query string
    - status of observed resource (avail, notav)
    - current value, if any
    - time of last access
    """

    def __init__ (self, sl, res, token):
        """
        Resource constructor.
        """

        self.slave = sl
        self.res = res
        self.token = token

    def __str__ (self):
        """
        Return the string representation of an observer
        """

        s = self.token + '/' + str (self.slave.sid) + '/' + '/'.join (self.res)
        return s

    def html (self):
        """
        Return the HTML representation of an observer
        """

        s = html.escape ('<' + str (self) + '>')
        return s
