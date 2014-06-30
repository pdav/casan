'''
This module contains the Resource class.
'''

from .option import Option
from .msg import Msg

class Resource:
    '''
    An object of class Resource represents a resource which
  	is provided by an SOS slave.
  
    This class represents a resource. A resource has:
    - an URL (vector of path components)
    - some attributes: name, title, etc. which are stored as a
  	list of (key, val) where the key is the name of the
  	attribute (e.g. `name`, `title`, `ct`) and val is a list
  	of associated values (`temp`, `Temperature`, `0`).
    '''

    class Attribute:
        '''
        Class representing attributes
        '''
        def __init__(self, name = None, values = None):
            '''
            Default constructor for attribute type
            '''
            # This construct is not important in this case, but it can be
            # See explanation here :
            # http://stackoverflow.com/questions/101268/hidden-features-of-python#113198
            if values is None:
                values = []
            self.name = name
            self.values = values

    def __init__(self, path):
        '''
        Resource constructor.
        Splits the path into components and stores it in the class.
        '''
        self.pathopt = []
        self.attributes = []
        self.vpath = path.split('/')
        for i in range(self.vpath.count('')):
            self.vpath.remove('')
        for comp in self.vpath:
            self.pathopt.append(Option(Option.optcodes.MO_URI_PATH, comp,
                                       len(comp)))

    def __str__(self):
        '''
        Returns the string representation of a resource.
        '''
        s = '/'.join(self.vpath)
        for attr in self.attributes:
            for val in attr.values:
                s = s + ';' + attr.name + '="' + val + '"'
        return s

    def __eq__(self, something):
        '''
        Equality test operator. Tests equality between a Resource object and :
            - A string containing a path.
            - A NONEMPTY list of path components
            - A NONEMPTY list of options
        The not empty condition is necessary, because we need an element in
        the list to infer what type the list holds and therefore which
        comparison to perform.

        Raises : TypeError if 'something' is not a valid type to compare to
                 a Resource.
                 RuntimeError if 'something' is an empty list.
        Exception safety : strong
        '''
        if something.type() == str:
            something = something.split('/')
            for i in range(something.count('')):
                something.remove('')
        if something.type() == list:
            if len(something) == 0:
                raise RuntimeError('''Cannot perform comparison with an empty 
                                    list.''')
            if type(something[0]) == str:
                return something == self.vpath
            elif type(something[0]) == Option:
                return something == self.pathopt
            else:
                raise TypeError
        elif something is None:
            return False
        else:
            raise TypeError
            
    def __ne__(self, something):
        '''
        Difference test operator.
        See documentation for equality test operator
        '''
        return (not self == something)

    def add_attribute(self, name, value):
        '''
        Adds a value to an attribute
        '''
        found = False
        for attr in self.attributes:
            if attr.name == name:
                found = True
                attr.values.append(value)
        if not found:
            self.attributes.append(self.Attribute(name, value))


    def add_to_message(self, msg):
        '''
        Add the resource path to a message
        '''
        for opt in self.pathopt:
            msg.optlist.append(opt)


    def get_attribute(self, name):
        '''
        Returns the values associated with an attribute name.
        '''
        for attr in self.attributes:
            if name == attr.name:
                return attr.values
