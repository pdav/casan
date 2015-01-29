"""
This module contains the cache related functionality of CASAN.
"""
__author__ = 'chavignat'

from datetime import datetime, timedelta
from threading import Lock

from util.debug import print_debug, dbg_levels
from .option import Option


class Cache:
    """
    This class manages the cache of CASAN.
    """
    class Entry:
        """
        This class represents an entry in the CASAN cache.
        """
        def __init__(self, req):
            """
            Entry constructor.
            :param req: Request to add in the cache.
            """
            self.expire = datetime.now()
            self.req = req

    def __init__(self):
        """
        Cache constructor, constructs an empty cache.
        """
        self.cache = []
        self.lock = Lock()

    def add(self, req):
        """
        Add a request to the cache
        :param req: Msg object to add to the cache
        """
        rep = req.req_rep
        if rep is not None:
            max_age = Option.int_from_bytes(rep.max_age())
            if max_age is not None:
                with self.lock:
                    e = self.Entry(req)
                    print_debug(dbg_levels.CACHE, 'Adding ' + str(req) + 'for '
                                + str(max_age) + 'seconds')
                    e.expire = datetime.now() + timedelta(seconds=max_age)
                    e.req = req
                    self.cache.append(e)

    def get(self, req):
        """
        This method looks the cache for a reply to a given request.
        The request must have been linked with the reply.
        Cache cleanup (removing obsolete entries) is performed if needed.
        :param req: request of type Msg
        :return: cached reply if any, else None
        """
        r = None
        with self.lock:
            do_cleanup = False
            now = datetime.now()
            for entry in self.cache:
                if entry.expire < now:
                    do_cleanup = True
                    print_debug(dbg_levels.CACHE, 'Expiring ' + str(entry.req))
                else:
                    if req.cache_match(entry.req):
                        print_debug(dbg_levels.CACHE, 'found ' + str(entry.req))
                        r = entry.req
            if do_cleanup:
                self.cache = [e for e in self.cache if e.expire > now]
            return r
