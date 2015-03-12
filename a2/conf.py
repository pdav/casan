import configparser
import urllib.parse
import io


class Conf:
    """
    Configuration file handling
    The conf class is used to read the configuration file and
    store information into instance variables, which are
    directly used by the class Master
    """

    #
    # Default values
    #

    defval_ = {
        'firsthello': 3,
        'hello': 10,
        'slavettl': 3600,
        'port:http': 80,
        'port:https': 443,
        '802154:channel': 12,
    }

    #
    # Object attributes
    #
    # timers: dictionnary indexed by timer name
    # namespaces: dictionnary indexed by namespace type
    # http: list of tuples (scheme, addr, port)
    #

    timers = {}
    namespaces = {}
    http = []

    #
    # Internal variables
    #

    config_ = {}

    def __str__ (self):
        output = io.StringIO ()
        self.config_.write (output)
        return output.getvalue ()

    def parse (self, file):
        """
        Parse a configuration file and store information in object
        """

        self.config_ = configparser.ConfigParser ()
        if not self.config_.read (file):
            raise RuntimeError ('Cannot read ' + file)

        for sect in self.config_.sections ():
            w = sect.split ()
            l = len (w)
            if sect == 'timers':
                self.parse_timers (self.config_ [sect])
            elif w [0] == 'namespace' and l == 2:
                self.parse_namespace (self.config_ [sect], w [1])
            elif w [0] == 'http' and l == 2:
                self.parse_http (self.config_ [sect], w [1])
            elif w [0] == 'network' and l == 2:
                self.parse_network (self.config_ [sect], w [1])
            elif w [0] == 'slave' and l == 2:
                self.parse_slave (self.config_ [sect], w [1])
            else:
                raise RuntimeError ("Unknown section '" + sect + "' in "
                                    + file)

    def parse_timers (self, sectab):
        """
        Parse a "timers" section.
        Section contents are:
        - firsthello: time before first hello is sent (in sec)
        - hello: time between hello packets (in sec)
        - slavettl: default slave TTL (in sec)
        """

        for k in ['firsthello', 'hello', 'slavettl']:
            self.timers [k] = sectab [k] if k in sectab else self.defval [k]

    def parse_namespace (self, sectab, name):
        """
        Parse a "namespace" section.
        'name' is the namespace type ('admin', 'casan', or 'well-known')
        Section contents are:
        - uri: URI of namespace
        """

        if name not in ['admin', 'casan', 'well-known']:
            raise RuntimeError ("Unknown namespace type '" + name + "'")

        if 'uri' not in sectab:
            raise RuntimeError ("No URI for namespace " + name)
        uri = sectab ['uri']
        if uri in self.namespaces:
            raise RuntimeError ("Duplicate URI '" + uri + '"')
        self.namespaces [name] = uri

    def parse_http (self, sectab, name):
        """
        Parse a "http" section to specify an HTTP server instance
        'name' is just an (unused) name
        Section contents are:
        - url: base URL of HTTP server
        """

        if 'url' not in sectab:
            raise RuntimeError ("No URL for http " + name)

        url = sectab ['url']
        p = urllib.parse.urlparse (url)
        if p.scheme not in ['http', 'https']:
            raise RuntimeError ("Invalid URL scheme for http " + name)

        port = p.port
        if p.port is None:
            port = self.defval ['port:' + p.scheme]
        port = int (port)
        spec = (p.scheme, p.hostname, port)
        self.http.append (spec)

    def parse_network (self, sectab, name):
        pass

    def parse_slave (self, sectab, name):
        pass
