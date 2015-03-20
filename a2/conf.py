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
        'firsthello': '3',
        'hello': '10',
        'slavettl': '3600',
        'port:http': '80',
        'port:https': '443',
        '802154:channel': '12',
        'ethertype': '0x88b5',
        'mtu': '0',
    }

    def __init__ (self):
        """
        Initialize instance variables for the newly created object
        Public variables
        - timers: dictionnary indexed by timer name
        - namespaces: dictionnary indexed by namespace type
        - networks: list of tuples (type, dev, mtu, net-specific-subinfo)
        - http: list of tuples (scheme, addr, port)
        Private variables:
        - _config: configparser.ConfigParser object
        """
        self.timers = {}
        self.namespaces = {}
        self.networks = []
        self.http = []
        self.slaves = []

        self._config = {}

    def __str__ (self):
        output = io.StringIO ()
        self._config.write (output)
        return output.getvalue ()

    def parse (self, file):
        """
        Parse a configuration file and store information in object
        """

        self._config = configparser.ConfigParser ()
        if not self._config.read (file):
            raise RuntimeError ('Cannot read ' + file)

        for sect in self._config.sections ():
            w = sect.split ()
            l = len (w)
            if sect == 'timers':
                self._parse_timers (self._config [sect])
            elif w [0] == 'namespace' and l == 2:
                self._parse_namespace (self._config [sect], w [1])
            elif w [0] == 'http' and l == 2:
                self._parse_http (self._config [sect], w [1])
            elif w [0] == 'network' and l == 2:
                self._parse_network (self._config [sect], w [1])
            elif w [0] == 'slave' and l == 2:
                self._parse_slave (self._config [sect], w [1])
            else:
                raise RuntimeError ("Unknown section '" + sect + "' in "
                                    + file)

    def _parse_timers (self, sectab):
        """
        Parse a "timers" section.
        Section contents are:
        - firsthello: time before first hello is sent (in sec)
        - hello: time between hello packets (in sec)
        - slavettl: default slave TTL (in sec)
        """

        for k in ['firsthello', 'hello', 'slavettl']:
            v = self._getdefault (sectab, k, k, None)
            self.timers [k] = int (v, 0)

    def _parse_namespace (self, sectab, name):
        """
        Parse a "namespace" section.
        'name' is the namespace type ('admin', 'casan', or 'well-known')
        Section contents are:
        - uri: URI of namespace
        """

        e = "namespace " + name
        if name not in ['admin', 'casan', 'well-known']:
            raise RuntimeError ("Unknown namespace type '" + name + "'")

        uri = self._getdefault (sectab, 'uri', None, e)
        if uri in self.namespaces:
            raise RuntimeError ("Duplicate URI '" + uri + "' for " + e)
        self.namespaces [name] = uri

    def _parse_http (self, sectab, name):
        """
        Parse a "http" section to specify an HTTP server instance
        'name' is just an (unused) name
        Section contents are:
        - url: base URL of HTTP server
        """

        e = "http " + name
        url = self._getdefault (sectab, 'url', None, e)
        p = urllib.parse.urlparse (url)
        if p.scheme not in ['http', 'https']:
            raise RuntimeError ("Invalid URL scheme for " + e)

        port = p.port
        if p.port is None:
            port = int (self.defval ['port:' + p.scheme], 0)
        spec = (p.scheme, p.hostname, port)
        self.http.append (spec)

    def _parse_network (self, sectab, name):
        """
        Parse a "network" section to specify a CASAN network
        'name' is just an (unused) name
        Section contents are:
        - type: '802.15.4' or 'ethernet'
        - subtype: 'xbee' (only for 802.15.4 type)
        - dev: device name
        - mtu: desired MTU or 0 for default MTU
        - addr: only for 802.15.4 / xbee
        - panid: only for 802.15.4
        - channel: only for 802.15.4
        - ethertype: only for ethernet
        """

        e = "network " + name

        type = self._getdefault (sectab, 'type', None, e)
        dev = self._getdefault (sectab, 'dev', None, e)
        mtu = int (self._getdefault (sectab, 'mtu', 'mtu', e), 0)

        if type == 'ethernet':
            ethertype = self._getdefault (sectab, 'ethertype', 'ethertype', e)
            ethertype = int (ethertype, 0)
            sub = (ethertype, )
        elif type == '802.15.4':
            subtype = self._getdefault (sectab, 'subtype', None, e)
            addr = self._getdefault (sectab, 'addr', None, e)
            panid = self._getdefault (sectab, 'panid', None, e)
            channel = self._getdefault (sectab, 'channel', '802154:channel', e)
            channel = int (channel, 0)
            sub = (subtype, addr, panid, channel)
        else:
            raise RuntimeError ("Invalid type" + e)
        spec = (type, dev, mtu, sub)
        self.networks.append (spec)

    def _parse_slave (self, sectab, name):
        """
        Parse a "slave" section to specify a CASAN slave
        'name' is just an (unused) name
        Section contents are:
        - sid = 169
        - ttl = 600 (optional)
        - mtu = 0 (optional)
        """

        e = "slave " + name

        sid = int (self._getdefault (sectab, 'id', None, e), 0)
        ttl = int (self._getdefault (sectab, 'ttl', 'slavettl', e), 0)
        mtu = int (self._getdefault (sectab, 'mtu', 'mtu', e), 0)

        spec = (sid, ttl, mtu)
        self.slaves.append (spec)

    def _getdefault (self, sectab, key, defkey, ectx):
        """
        Get a configuration parameter value from the current section
        or a default value if none is given. Raise an exception if
        not found.
        :param sectab: current section dictionnary
        :type  sectab: dictionnary
        :param key: key to look for in sectab[]
        :type  key: str
        :param defkey: key to look for in _defval[]
        :type  defkey: str
        :param ectx: error context for error message
        :type  ectx: str
        :return: value or exception
        :rtype: str
        """

        if key in sectab:
            v = sectab [key]
        elif defkey is None:
            raise RuntimeError ("No '{}' value for {}".format (key, ectx))
        else:
            v = self.defval_ [defkey]

        return v
