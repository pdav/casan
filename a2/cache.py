"""
This module contains the cache related functionality of CASAN.
"""

import datetime

import option


class Cache (object):
    """
    This class manages the cache of CASAN.
    Each entry is a tuple (expiration date, message)
    """

    def __init__ (self, loop, timer):
        """
        Cache constructor, constructs an empty cache.
        :param loop: asyncio loop for cache cleanup
        :param timer: interval between cache cleanups
        """

        self._entries = []
        loop.call_later (timer, lambda: self._cleanup (loop, timer))

    def __str__ (self):
        """
        Cast the entire cache into a string
        """

        return '\n'.join (['<' + str (e) + ', ' + str (m) + '>'
                                      for (e, m) in self._entries])

    def html (self):
        """
        Return an HTML representation of all cache contents
        """

        if self._entries == []:
            h = 'Cache is empty'
        else:
            l = ['<li>expire=' + str (e) + ', msg=(' + m.html () + ')</li>'
                 for (e, m) in self._entries]
            h = '<ul>' + '\n'.join (l) + '</ul>'
        return h

    def _cleanup (self, loop, timer):
        """
        Remove expired entries from cache and reschedule cleanup
        :param loop: asyncio loop for cache cleanup
        :param timer: interval between cache cleanups
        """

        print ('Cache cleanup')
        now = datetime.datetime.now ()
        self._entries = [(exp, m) for (exp, m) in self._entries if exp > now]

        loop.call_later (timer, lambda: self._cleanup (loop, timer))

    def add (self, req):
        """
        Add a request to the cache
        :param req: Msg object to add to the cache
        """

        rep = req.req_rep
        if rep is not None:
            max_age = rep.max_age ()
            if max_age is not None:
                expire = datetime.datetime.now ()
                expire += datetime.timedelta (seconds=max_age)
                entry = (expire, req)
                self._entries.append (entry)
                print ('Adding ' + str (req) + ' for ' + str (max_age) + 'seconds')

    def get (self, req):
        """
        This method searches the cache for a given request.
        The request must have been linked with the reply.
        Cache cleanup (removing obsolete entries) is performed if needed.
        :param req: request of type Msg
        :return: cached reply if any, else None
        """

        r = None
        now = datetime.datetime.now ()
        for (exp, m) in self._entries:
            if now <= exp and req.cache_match (m):
                print ('Cache hit ' + str (m))
                r = m
                break

        if r is None:
            print ('Cache miss')

        return r
