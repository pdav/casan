"""
This module provides debugging facilities.
To abbreviate typing, import this module with:
    from debug import d
then you can use the various methods of the class Debug
    d.m ('cache', 'Cache hit')
"""

import logging


class Debug (object):
    """
    Debug class. One instance, called d, is created in order to use it.
    """

    _CATEGORIES = ['master', 'conf', 'cache', 'http',
                   'slave', 'msg', 'opt', 'asyncio']

    def __init__ (self):
        """
        Initialize the debug class
        """

        self._cat = []
        self._level = logging.WARNING

    def _set_level (self, level):
        """
        Set the logging level such as we always use the lowest
        possible level
        :param level: one of logging.DEBUG, INFO, etc.
        """

        if level < self._level:
            self._level = level

    def _set_category (self, cat):
        """
        Private method to append a category in the list of displayed
        categories. Special case for the 'asyncio' category which is
        enabled (see asyncio documentation) by setting the log level
        to DEBUG, which means that other debug information must use
        the INFO level.
        :param cat: one item from the _CATEGORIES list
        """

        self._cat.append (cat)
        if cat == 'asyncio':
            self._set_level (logging.DEBUG)
        else:
            self._set_level (logging.INFO)

    def add_category (self, catlist):
        """
        Add a category to the list of displayed categories.
        The category may be a comma-separated list (ex: 'msg,opt,slave')
        or just 'all'
        :param catlist: comma-separated list of categories
        """

        for cat in catlist.split (','):
            if cat == 'all':
                for c in self._CATEGORIES:
                    self._set_category (c)
            elif cat in self._CATEGORIES:
                self._set_category (cat)
            else:
                raise RuntimeError ('Unknown debug category ' + cat)

    def all_categories (self):
        """
        Return the list of call supported categories
        """

        return ['all'] + self._CATEGORIES

    # pylint: disable=no-self-use
    def set_file (self, logfile):
        """
        Set log file
        """

        if logfile is not None and logfile != '-':
            logging.basicConfig (filename=logfile)

    def start (self):
        """
        Start the debug message system. In fact, initialize level
        (it must apparently be performed only once)
        """

        logging.basicConfig (level=self._level)

    # pylint: disable=invalid-name
    def m (self, cat, msg, *args, **kwargs):
        """
        Output a debug message.
        This method has a very short name to ease writing debug messages.
        """

        if cat not in self._CATEGORIES:
            raise RuntimeError ('Invalid debug category ' + cat)
        if cat in self._cat:
            logging.info (msg, *args, **kwargs)


# pylint: disable=invalid-name
d = Debug ()
