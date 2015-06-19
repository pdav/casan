"""
This module contains the Resource class.
"""

import html

import option


class Resource (object):
    """
    An object of class Resource represents a resource provided
    by an CASAN slave. A resource has:
    - an URL
    - some attributes: name, title, etc. which are stored as a
        list of tuples (key, vallist) where:
        - key is the attribute name ('name', 'title', 'ct', etc.)
        - vallist is a list of associated values ('temp', '0', etc.)
    """

    def __init__ (self, path):
        """
        Resource constructor.
        """

        self._path = path
        self._attr = []

    def __str__ (self):
        """
        Return the string representation of a resource.
        """

        s = '<' + self._path + '>'
        for (key, vallist) in self._attr:
            for val in vallist:
                if val is None:
                    s += ';' + key
                else:
                    s += ';' + key + '=' + val
        return s

    def html (self):
        """
        Return the string representation of a resource.
        """

        s = html.escape ('<' + self._path + '>')
        for (key, vallist) in self._attr:
            for val in vallist:
                if val is None:
                    s += ';' + html.escape (key)
                else:
                    s += ';' + html.escape (key) + '=' + html.escape (val)
        return s

    # XXX
    def __eq__ (self, something):
        """
        Test equality between a Resource object and a path
            - a string containing a path.
            - a NONEMPTY list of path components
            - a NONEMPTY list of options
        The not empty condition is necessary, because we need an element in
        the list to infer what type the list holds and therefore which
        comparison to perform.

        Raises : TypeError if 'something' is not a valid type to compare to
                 a Resource.
                 RuntimeError if 'something' is an empty list.
        Exception safety : strong
        """

        if isinstance (something, str):
            return self._path == something
        elif isinstance (something, list):
            return self._path.split ('/') == something
        elif something is None:
            return False
        else:
            raise TypeError

    def __ne__(self, something):
        """
        Difference test operator.
        """

        return not self == something

    def add_attribute (self, name, value):
        """
        Add a value to an attribute
        """

        found = False
        for (key, vallist) in self._attr:
            if key == name:
                found = True
                vallist.append (value)

        if not found:
            na = (name, [value])
            self._attr.append (na)

    def add_to_message (self, m):
        """
        Add the resource path to a message
        :param m: message
        :type  m: class Msg
        """

        up = option.Option.Codes.URI_PATH
        for comp in self._path.split ('/'):
            o = option.Option (up, optval=comp)
            m.optlist.append (o)

    def get_attribute (self, name):
        """
        Return the values associated with an attribute name.
        """
        for (key, vallist) in self._attr:
            if key == name:
                return vallist
