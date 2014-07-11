from sos import sos
from conf import Conf
from sos.l2_154 import l2net_154
from sos.slave import Slave
from util.debug import print_debug, dbg_levels
from sos_http_server.http_server import HTTPServer
from sos_http_server.reply import HTTPCodes


class Master:
    def __init__(self, cf):
        self.cf = cf
        self.engine = sos.SOS()

    def start(self):
        r = True
        self.engine.timer_first_hello = self.cf.timers[self.cf.CfTimerIndex.I_FIRST_HELLO]
        self.engine.timer_interval_hello = self.cf.timers[self.cf.CfTimerIndex.I_INTERVAL_HELLO]
        self.engine.timer_slave_ttl = self.cf.timers[self.cf.CfTimerIndex.I_SLAVE_TTL]
        for net in self.cf.netlist:
            if net.type is Conf.NetType.NET_154:
                l2n = l2net_154()
                l2n.init(net.net_154.iface, 'xbee' if net.net_154.type is Conf.Net154Type.NET_154_XBEE else None,
                         net.mtu, net.net_154.addr, net.net_154.panid, net.net_154.channel, 2)
                self.engine.start_net(l2n)
        for s in self.cf.slavelist:
            sl = Slave()
            sl.id = s.id
            sl.def_mtu = s.mtu
            sl.init_ttl = s.ttl if s.ttl != 0 else self.engine.timer_slave_ttl
            self.engine.slist.append(sl)

        for h in self.cf.httplist:
            self.engine.httplist.append(HTTPServer(self, h.listen, h.port))

        # Initialize SOS engine last
        self.engine.init()

        return r

    def stop(self):
        self.engine.stop()

    def parse_path(self, path):
        """
        Parse a PATH to extract namespace type, slave and resource on this slave
        :param path: path to parse
        :return: object of ParseResult type if parsing was succesful, None either.
        """
        class ParseResult:
            def __init__(self):
                self.type, self.base, self.slave, self.resource, self.str_ = (None,) * 5
        res = ParseResult()
        try:
            path_list = self.split_path(path)
            for ns in self.cf.nslist:
                if path_list[:len(ns.prefix)] == ns.prefix:  # Does the path start with the prefix?
                    res.base = '/' + '/'.join(ns.prefix)
                    if ns.type is Conf.CfNsType.NS_ADMIN:
                        if len(path_list) == len(ns.prefix):
                            res.str_ = '/'
                        else:
                            res.str_ = '/' + '/'.join(path_list[len(ns.prefix):])
                        print_debug(dbg_levels.HTTP, 'HTTP request for admin namespace: {}, remainder={}'.format(res.base, res.str_))
                    elif ns.type is Conf.CfNsType.NS_SOS:
                        if len(path_list) == len(ns.prefix):
                            raise Exception()
                        else:
                            sid = path_list[len(ns.prefix)]
                            res.base += '/' + sid
                            sid = int(sid)
                            res.slave = self.engine.find_slave(sid)
                            if res.slave is None or res.slave.status is not Slave.StatusCodes.SL_RUNNING:
                                raise Exception()
                            res.resource = res.slave.find_resource(path_list[len(ns.prefix)+1:])
                            if res.resource is None:
                                raise Exception()
                            print_debug(dbg_levels.HTTP, 'HTTP request for SOS namespace : {}, slave id= {}'.format(res.base, res.slave.id))
                    elif ns.type is Conf.CfNsType.NS_WELL_KNOWN:
                        pass  # No specific handling
                    else:
                        raise Exception()
                    res.type = ns.type
        except Exception as e:
            res = None

        return res

    @staticmethod
    def split_path(path):
        if not path.startswith('/'):
            return []
        else:
            return path.split('/')[1:]  # Discard first empty string

    def handle_http(self, request_path, req, rep):
        """
        :param request_path: string
        :param req:
        :param rep:
        :return:
        """
        path_res = self.parse_path(request_path)
        if path_res is None:
            rep.code = HTTPCodes.HTTP_NOT_FOUND
        if path_res.type in [Conf.CfNsType.NS_ADMIN, Conf.CfNsType.NS_SOS, Conf.CfNsType.NS_WELL_KNOWN]:
            {Conf.CfNsType.NS_ADMIN: self.http_admin,
             Conf.CfNsType.NS_SOS: self.http_sos,
             Conf.CfNsType.NS_WELL_KNOWN: self.http_well_known}[path_res.type](path_res, req, rep)
        else:
            rep.code = HTTPCodes.HTTP_NOT_FOUND

    def http_admin(self, res, req, rep):
        """
        Handle a HTTP request for admin namespace.
        :param res:
        :param req:
        :param rep:
        :return:
        """
        if res.str_ == '/':
            rep.code = HTTPCodes.HTTP_OK
            rep.body = ('<html><body><ul>'
                        '<li><a href=\'{base}/conf\'>configuration</a>'
                        '<li><a href=\'{base}/run\'>running status</a>'
                        '<li><a href=\'{base}/slave\'>slave status</a>'
                        '</ul></body></html>'.format(base=res.base).encode())
            rep.headers = [(b'Content-Length', str(len(rep.body)).encode()),
                           (b'Content-Type', b'text/html')]
        elif res.str_ == '/conf':
            rep.code = HTTPCodes.HTTP_OK
            rep.body = ('<html><body><pre>' + str(self.cf) + '</pre></body></html>').encode()
            rep.headers = [(b'Content-Length', str(len(rep.body)).encode()),
                           (b'Content-Type', b'text/html')]
        elif res.str_ == '/run':
            rep.code = HTTPCodes.HTTP_OK
            rep.body = ('<html><body><pre>' + str(self.engine) + '</pre></body></html>').encode()
            rep.headers = [(b'Content-Length', str(len(rep.body)).encode()),
                           (b'Content-Type', b'text/html')]
        elif res.str_ == '/slave':
            if req.method == b'POST':
                sid = -1
                new_status = None
                for arg in req.post_args:
                    if arg[0] == 'slaveid':
                        sid = int(arg[1])
                    elif arg[0] == 'status':
                        if arg[1] == 'inactive':
                            new_status = Slave.StatusCodes.SL_INACTIVE
                        elif arg[1] == 'running':
                            new_status = Slave.StatusCodes.SL_RUNNING
                if sid != -1:
                    r = True
                    if r:
                        rep.code = HTTPCodes.HTTP_OK
                        rep.body = ('<html><body><pre>set to {} (note: not implemented)'
                                    '</pre></body></html>').format('inactive' if new_status is Slave.StatusCodes.SL_INACTIVE else 'running').encode()
                else:
                    rep.code = HTTPCodes.HTTP_BAD_REQUEST
            else:
                rep.code = HTTPCodes.HTTP_OK
                rep.body = ('<html><body>'
                            '<form method=\'post\' action=\'{}/slave\'>'
                            'slave id <input type=text name=slaveid><p>'
                            'status <select size=1 name=status>'
                            '<option value=\'inactive\'>INACTIVE'
                            '<option value=\'running\'>RUNNING'
                            '</select>'
                            '<input type=submit value=set>' '</pre></body></html>').format(res.base).encode()
            rep.headers = [(b'Content-Length', str(len(rep.body)).encode()),
                           (b'Content-Type', b'text/html')]
        else:
            rep.code = HTTPCodes.HTTP_NOT_FOUND

    def http_sos(self, res, req, rep):
        """
        Handle a HTTP request for SOS namespace.
        :param res:
        :param req:
        :param rep:
        :return:
        """
        pass

    def http_well_known(self, res, req, rep):
        """
        Handle a HTTP request for '.well-known' namespace.
        :param res:
        :param req:
        :param rep:
        :return:
        """
        pass