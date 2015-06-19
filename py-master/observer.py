"""
This module contains the Observer class.
"""

import datetime
import html

import option
import msg


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

    class Status (object):
        INACTIVE = 0                # observer not started
        ACTIVE = 1                  # confirmation received

    def __init__ (self, sl, res, token):
        """
        Resource constructor.
        :param sl: slave
        :type  sl: class Slave
        :param res: resource path
        :type  res: list
        :param token: observer token
        :type  token: bytes
        """

        self.status = self.Status.INACTIVE
        self.res = res

        self._slave = sl
        self._token = token
        self._lastobs = 0            # last counter (observe option value)
        self._value = None
        self._contenttype = None
        self._expire = None

    def __str__ (self):
        """
        Return the string representation of an observer
        """

        s = str (self._token) + '/' + str (self._slave.sid) + '/' + '/'.join (self.res)
        return s

    def html (self):
        """
        Return the HTML representation of an observer
        """

        s = html.escape ('<' + str (self) + '>')
        return s

    def match (self, token, res=None):
        """
        Check if a given observer matches the token and optionally
        the resource (iterable object)
        :param res: resource path or None
        :type  res: list
        :param token: token
        :type  token: bytearray
        :return: True or False
        """

        return self._token == token and (res is None or self.res == res)

    def set_value (self, m):
        """
        Set the observed value according to the received message
        :param m: received message
        :type  m: class Msg
        """

        obsval = m.observe ()
        if self.status == self.Status.INACTIVE or obsval > self._lastobs:
            self.status = self.Status.ACTIVE

            # counter of last observe packet
            self._lastobs = obsval

            # last value encountered...
            self._value = m.payload

            # ... and associated content format
            cf = m.content_format ()
            self._contenttype = option.ContentFormat.coap2http (cf)

            # expiration
            ma = m.max_age ()
            expire = datetime.datetime.now ()
            expire += datetime.timedelta (seconds=ma)
            self._expire = expire

    def get_value (self):
        """
        Return the current observation value, if still valid.
        We don't check the observer status since the slave may
        be off while the last reported value is still valid.
        :return: couple (value, http content-type)
        """

        r = (None, None)
        now = datetime.datetime.now ()
        if self._expire is not None and now < self._expire:
            r = (self._value.decode (), self._contenttype)

        return r

    def start (self):
        """
        Start an observer
        """

        self.status = self.Status.INACTIVE

        mreq = msg.Msg ()
        mreq.peer = self._slave.addr
        mreq.l2n = self._slave.l2n
        mreq.msgtype = msg.Msg.Types.CON
        mreq.msgcode = msg.Msg.Codes.GET
        mreq.token = self._token

        obs = option.Option (option.Option.Codes.OBSERVE, optval=0)
        mreq.optlist.append (obs)

        up = option.Option.Codes.URI_PATH
        for p in self.res:
            mreq.optlist.append (option.Option (up, optval=p))

        mreq.send ()
