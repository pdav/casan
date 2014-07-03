from enum import Enum, IntEnum
from io import StringIO

class Conf:
    """
    Configuration file handling
    The conf class is used to read the configuration file and
    represent the configuration through an intermediate form,
     which is composed of instance variables that the master
     class can use.
    """
    # Define the constants defined in conf.h and help strings
    DEFAULT_FIRST_HELLO = 3
    DEFAULT_INTERVAL_HELLO = 10
    DEFAULT_SLAVE_TTL = 3600
    DEFAULT_HTTP_PORT = 'http'
    DEFAULT_HTTP_LISTEN = '*'
    DEFAULT_HTTP_THREADS = 5
    DEFAULT_154_CHANNEL = 12
    syntax_help = {'ALL': 'http-server, namespace, timer, network, or slave',
                   'HTTP': 'http-server [listen <addr>] [port <num>] [threads <num>]',
                   'NAMESPACE': 'namespace <admin|sos|well-known> <path>',
                   'TIMER': 'timer <firsthello|hello|slavettl|http> <value in s>',
                   'NETWORK': 'network <ethernet|802.15.4> ...',
                   'SLAVE': 'slave id <id> [ttl <timeout in s>] [mtu <bytes>]',
                   'NETETH': 'network ethernet <iface> [mtu <bytes>] [ethertype [0x]<val>]',
                   'NET154': 'network 802.15.4 <iface> type <xbee> addr <addr> panid <id> [channel <chan>] [mtu <bytes>]'}

    # Define the enums defined in conf.h
    class CfNsType(Enum):
        NS_NONE = 0
        NS_ADMIN = 1
        NS_SOS = 2
        NS_WELL_KNOWN = 3

    class CfTimerIndex(IntEnum):
        I_FIRST_HELLO = 0
        I_INTERVAL_HELLO = 1
        I_SLAVE_TTL = 2
        I_LAST = 3

    class NetType(Enum):
        NET_NONE = 0
        NET_ETH = 1
        NET_154 = 2

    class Net154Type(Enum):
        NET_154_NONE = 0
        NET_154_XBEE = 1

    # Define the structs defined in conf.h
    class CfHttp:
        def __init__(self):
            self.listen = ''
            self.port = ''
            self.threads = 0

    class CfNamespace:
        def __init__(self):
            self.type = Conf.CfNsType.NS_NONE

    class CfNetEth:
        def __init__(self):
            self.ethertype = 0

    class CfNet154:
        def __init__(self):
            self.type = Conf.Net154Type.NET_154_NONE
            self.addr, self.panid, self.iface = '', '', ''
            self.channel = 0

    class CfNetwork:
        def __init__(self):
            self.type, self.mtu = Conf.NetType.NET_NONE, 0

    class CfSlave:
        def __init__(self):
            self.id, self.ttl, self.mtu = 0, 0, 0

    # And finally the actual configuration functions

    def __init__(self):
        self.done = False
        self.httplist, self.nslist, self.netlist, self.slavelist = [], [], [], []
        self.timers = [0] * 3
        self.lineno = 0
        self.file = None

    def parse(self, file_="./sosd.conf"):
        return self.parse_file(file_)

    def parse_file(self, file_):
        self.file = file_
        r = False
        try:
            self.file = file_
            f = open(self.file, 'r')
            self.lineno = 0
            for line in f:
                self.lineno += 1
                r = self.parse_line(line)
                if not r:
                    break
        except OSError:
            return False
        if r:
            self.done = True
        self.file = None
        return r

    def parse_line(self, l):
        # First pass : remove comments
        if '#' in l:
            l = l[:l.find('#')]

        # Second pass : split line into tokens
        tokens = l.split()
        r = True

        # Third pass : quick & dirty line parser
        a = len(tokens)
        if a != 0:
            if tokens[0] == 'http-server':
                c = self.CfHttp()
                for i in range(1, a - 1, 2):
                    if tokens[i] == 'listen':
                        if c.listen != '':
                            self.parse_error_dup_token(tokens[i], 'HTTP')
                            r = False
                        else:
                            c.listen = tokens[i+1]
                    elif tokens[i] == 'port':
                        if c.port != '':
                            self.parse_error_dup_token(tokens[i], 'HTTP')
                            r = False
                        else:
                            c.port = tokens[i+1]
                    elif tokens[i] == 'threads':
                        if c.threads != 0:
                            self.parse_error_dup_token(tokens[i], 'HTTP')
                            r = False
                        else:
                            c.threads = int(tokens[i+1])
                    else:
                        self.parse_error_unk_token(tokens[i], 'HTTP')
                        r = False
                if r:
                    if a % 2 != 1:  # Odd number of parameters
                        self.parse_error_num_token(a, 'HTTP')
                        r = False
                    else:
                        self.httplist.append(c)
            elif tokens[0] == 'namespace':
                c = self.CfNamespace()
                if a != 3:
                    self.parse_error_num_token(a, 'NAMESPACE')
                    r = False
                else:
                    c.prefix = tokens[2].split('/')[1:]  # Remove leading empty string
                    try:
                        c.type = {'admin': self.CfNsType.NS_ADMIN,
                                  'sos': self.CfNsType.NS_SOS,
                                  'well-known': self.CfNsType.NS_WELL_KNOWN}[tokens[1]]
                    except KeyError:
                        self.parse_error_unk_token(tokens[1], 'NAMESPACE')
                        r = False
                    if r:
                        self.nslist.append(c)
            elif tokens[0] == 'timer':
                if a != 3:
                    self.parse_error_num_token(a, 'TIMER')
                    r = False
                else:
                    try:
                        idx = self.CfTimerIndex[{'firsthello': 'I_FIRST_HELLO',
                                                 'hello': 'I_INTERVAL_HELLO',
                                                 'slavettl': 'I_SLAVE_TTL'}[tokens[1]]]
                        self.timers[idx] = int(tokens[2])
                    except KeyError:
                        self.parse_error_unk_token(tokens[1], 'TIMER')
                        r = False
            elif tokens[0] == 'network':
                if a < 3:  # 3 Mandatory arguments according to sample sosd.conf
                    self.parse_error_mis_token('NETWORK')
                else:
                    c = self.CfNetwork()
                    if tokens[1] == 'ethernet':
                        # I think ethernet support is on in python, so
                        # I removed the test, for now.
                        c.type = self.NetType.NET_ETH
                        i = 2
                        if i >= a:
                            self.parse_error_num_token(a, 'NETETH')
                            r = False
                        else:
                            c.net_eth = self.CfNetEth()
                            c.net_eth.iface = tokens[i]
                            for i in range(i, a-1, 2):
                                if tokens[i] == 'mtu':
                                    if c.mtu != 0:
                                        self.parse_error_dup_token(tokens[i], 'NETETH')
                                        r = False
                                        break
                                    else:
                                        c.mtu = int(tokens[i+1])
                                elif tokens[i] == 'ethertype':
                                    if c.net_eth.ethertype != 0:
                                        self.parse_error_dup_token(tokens[i], 'NETETH')
                                        r = False
                                        break
                                    else:
                                        c.net_eth.ethertype = tokens[i+1]
                                else:
                                    self.parse_error_unk_token(tokens[i], 'NETETH')
                                    r = False
                                    break
                            if r and i+2 != a:  # Odd number of parameters
                                self.parse_error_num_token(a, 'NETETH')
                                # self.parse_error('No ethernet support')
                    elif tokens[1] == '802.15.4':
                        c.type = self.NetType.NET_154
                        c.net_154 = self.CfNet154()
                        i = 3
                        if i >= a:
                            self.parse_error_num_token(a, 'NET154')
                        else:
                            c.net_154.iface = tokens[2]
                            for i in range(i, a-1, 2):
                                if tokens[i] == 'mtu':
                                    if c.mtu != 0:
                                        self.parse_error_dup_token(tokens[i], 'NET154')
                                        r = False
                                        break
                                    else:
                                        c.mtu = int(tokens[i])
                                elif tokens[i] == 'type':
                                    if c.net_154.type is not self.Net154Type.NET_154_NONE:
                                        self.parse_error_dup_token(tokens[i], 'NET154')
                                        r = False
                                        break
                                    else:
                                        if tokens[i+1] == 'xbee':
                                            c.net_154.type = self.Net154Type.NET_154_XBEE
                                        else:
                                            self.parse_error_unk_token(tokens[i+1], 'NET154')
                                            r = False
                                            break
                                elif tokens[i] == 'addr':
                                    if c.net_154.addr != '':
                                        self.parse_error_dup_token(tokens[i], 'NET154')
                                        r = False
                                        break
                                    else:
                                        c.net_154.addr = tokens[i+1]
                                elif tokens[i] == 'panid':
                                    if c.net_154.panid != '':
                                        self.parse_error_dup_token(tokens[i], 'NET154')
                                        r = False
                                        break
                                    else:
                                        c.net_154.panid = tokens[i+1]
                                elif tokens[i] == 'channel':
                                    if c.net_154.channel != 0:
                                        self.parse_error_dup_token(tokens[i], 'NET154')
                                        r = False
                                        break
                                    else:
                                        c.net_154.channel = int(tokens[i+1])
                                else:
                                    self.parse_error_unk_token(tokens[i], 'NET154')
                                    r = False
                                    break
                        if r and i != a-2:
                            self.parse_error_num_token(a, 'NET154')
                            r = False
                    else:
                        self.parse_error_unk_token(tokens[1], 'NETWORK')
                        r = False
                    if r:
                        self.netlist.append(c)
            elif tokens[0] == 'slave':
                c = self.CfSlave()
                for i in range(1, a-1, 2):
                    if tokens[i] == 'id':
                        if c.id != 0:
                            self.parse_error_dup_token(tokens[i], 'SLAVE')
                            r = False
                            break
                        else:
                            c.id = int(tokens[i+1])
                    elif tokens[i] == 'ttl':
                        if c.ttl != 0:
                            self.parse_error_dup_token(tokens[i], 'SLAVE')
                            r = False
                            break
                        else:
                            c.ttl = int(tokens[i+1])
                    elif tokens[i] == 'mtu':
                        if c.mtu != 0:
                            self.parse_error_dup_token(tokens[i], 'SLAVE')
                            r = False
                            break
                        else:
                            c.mtu = int(tokens[i+1])
                    else:
                        self.parse_error_unk_token(tokens[i], 'SLAVE')
                        r = False
                        break
                if r:
                    if i != a-2:
                        self.parse_error_num_token(a, 'SLAVE')
                    else:
                        self.slavelist.append(c)
            else:
                self.parse_error_unk_token(tokens[0], 'ALL')
                r = False

        # 4th pass : set default values
        for h in self.httplist:
            if h.port == '':
                h.port = self.DEFAULT_HTTP_PORT
            if h.listen == '':
                h.listen = self.DEFAULT_HTTP_LISTEN
            if h.threads == 0:
                h.threads = self.DEFAULT_HTTP_THREADS

        if self.timers[self.CfTimerIndex.I_FIRST_HELLO] == 0:
            self.timers[self.CfTimerIndex.I_FIRST_HELLO] = 0
        if self.timers[self.CfTimerIndex.I_INTERVAL_HELLO] == 0:
            self.timers[self.CfTimerIndex.I_INTERVAL_HELLO] = 0
        if self.timers[self.CfTimerIndex.I_SLAVE_TTL] == 0:
            self.timers[self.CfTimerIndex.I_SLAVE_TTL] = 0
                    
        for slave in self.slavelist:
            if slave.ttl == 0:
                slave.ttl = self.timers[self.CfTimerIndex.I_SLAVE_TTL]

        for n in self.netlist:
            if n.type is self.NetType.NET_ETH and n.net_eth.ethertype == 0:
                n.net_eth.ethertype = 0
            if n.type is self.NetType.NET_154 and n.net_154.channel == 0:
                n.net_154.channel = self.DEFAULT_154_CHANNEL

        if r:
            self.done = True
        return r

    def write_to_stream(self, stream):
        if self.done:
            for h in self.httplist:
                stream.write('http-server listen ' + h.listen +
                             ' port ' + h.port + 
                             ' threads ' + str(h.threads) + '\n')
            for n in self.nslist:
                s = ''
                try:
                    s = {self.CfNsType.NS_ADMIN: 'admin',
                         self.CfNsType.NS_SOS: 'sos',
                         self.CfNsType.NS_WELL_KNOWN: 'well-known'}[n.type]
                except KeyError:
                    s = '(unknown)'
                stream.write('namespace ' + s + ' ')
                if len(n.prefix) == 0:
                    stream.write('/')
                else:
                    for p in n.prefix:
                        stream.write('/' + p)
                    stream.write('\n')
            for i in range(0, len(self.timers)):
                p = {self.CfTimerIndex.I_FIRST_HELLO: 'firsthello',
                     self.CfTimerIndex.I_INTERVAL_HELLO: 'hello',
                     self.CfTimerIndex.I_SLAVE_TTL: 'slavettl' }[i]
                stream.write(p + ' ' + str(self.timers[i]) + '\n')
            for n in self.netlist:
                if n.type is self.NetType.NET_ETH:
                    stream.write('network ' + 'ethernet ' + n.net_eth.iface + 
                                 'ethertype ' + n.net_eth.ethertype)
                elif n.type is self.NetType.NET_154:
                    stream.write('network ' + '802.15.4 ' + n.net_154.iface +
                                 ' type ' + ('xbee' if n.net_154.type is self.Net154Type.NET_154_XBEE else '(none)') +
                                 ' addr ' + n.net_154.addr +
                                 ' panid ' + n.net_154.panid +
                                 ' channel ' + str(n.net_154.channel))
                else:
                    stream.write('(unrecognised network)\n')
                stream.write(' mtu ' + str(n.mtu) + '\n')
            for s in self.slavelist:
                stream.write('slave id ' + str(s.id) +
                             ' ttl ' + str(s.ttl) +
                             ' mtu ' + str(s.mtu) + '\n')

    def to_string(self):
        ss = StringIO()
        self.write_to_stream(ss)
        return ss.getvalue()

    # Error handlers
    def parse_error(self, msg, help_=None):
        import sys
        sys.stderr.write(self.file + '(' + str(self.lineno) + '):' + msg + '\n')
        if help_ is not None:
            sys.stderr.write('    usage: ' + self.syntax_help[help_]  + '\n')

    def parse_error_num_token(self, n, help_):
        self.parse_error('invalid number of tokens (' + str(n) + ')', help_)

    def parse_error_unk_token(self, tok, help_):
        self.parse_error('unrecognised token \'' + tok + '\'', help_)

    def parse_error_dup_token(self, tok, help_):
        self.parse_error('duplicate token \'' + tok + '\'', help_)

    def parse_error_mis_token(self, help_):
        self.parse_error('missing token/value', help_)

