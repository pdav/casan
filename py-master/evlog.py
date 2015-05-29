"""
This module contains the EventLogger class
"""

import time
import json

class EventLogger (object):
    """
    The EventLogger abstracts event storage, access and retrieval.
    Events are signalled by the master or by slaves. An event is
    made of:
    - a source (master or slave number)
    - a date (time_t)
    - a message
    """

    def __init__(self):
        """
        Default constructor for the EventLogger class.
        Note: the _log list is always sorted with the last events first
        """

        self._maxsize = 0
        self._log = []

    def set_size (self, sz=None):
        """
        Set the maximum size of the event log (and truncate it if needed)
        :param sz: desired size, or 0 for an infinite event log
        :type  sz: integer
        """

        if sz is None:
            self._maxsize = 0
        else:
            self._maxsize = sz
            # truncate the list if needed
            if len (self._log) > self._maxsize:
                self._log = self._log [-self._maxsize:]

    def add (self, src, msg):
        """
        Add an event (with source and message) to the event log
        """

        dt = int (time.time ())
        self._log.append ((dt, src, msg))
        if self._maxsize > 0 and len (self._log) >= self._maxsize:
            self._log = self._log [-self._maxsize:]

    def get (self, since=None):
        """
        Returns the list of all events since the given date (time_t)
        :param since: starting date
        :type  since: number of seconds since Epoch (time_t)
        :return: a list of (src, date, msg)
        """

        if since is None:
            r = self._log
        else:
            r = []
            for (dt, src, msg) in self._log:
                if dt >= since:
                    r.append ((dt, src, msg))

        return r

    def get_json (self, since=None):
        """
        Returns the list of all events since the given date (time_t)
        in JSON format
        :param since: starting date
        :type  since: number of seconds since Epoch (time_t)
        :return: a JSON-formatted text
        """

        r = [{"date": dt, "src": src, "msg": msg}
                    for (dt, src, msg) in self.get (since)]
        return json.dumps (r)
