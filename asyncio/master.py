"""
This module contains the Master class.
"""
import asyncio

from sos import sos
from conf import Conf
from sos.l2_154 import l2net_154
from sos.slave import Slave
from util.debug import print_debug, dbg_levels
from sos_http_server.http_server import HTTPServer
from sos_http_server.reply import HTTPCodes
from sos import msg
from sos.cache import Cache
from sos.option import Option


class Master:
    """
    This class drives the SOS system and handles the HTTP servers, acting like a bridge between them.
    """

    def __init__(self, cf):
        self.cf = cf
        self.engine = sos.SOS()
        self.cache = Cache()

    def start(self):
        """
        Initialize the SOS engine from the configuration file, but does not start it.
        """
        r = True
        self.engine.timer_first_hello = self.cf.timers[self.cf.CfTimerIndex.I_FIRST_HELLO]
        self.engine.timer_interval_hello = self.cf.timers[self.cf.CfTimerIndex.I_INTERVAL_HELLO]
        self.engine.timer_slave_ttl = self.cf.timers[self.cf.CfTimerIndex.I_SLAVE_TTL]
        for net in self.cf.netlist:
            if net.type is Conf.NetType.NET_154:
                l2n = l2net_154()
                # The last parameter of the l2n.init method is the read timeout for the serial link.
                # If none is set, the receiver thread in charge of the network won't be able to exit
                # gracefully and the process will have to be killed. Don't touch that.
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

        return r

    def stop(self):
        """
        Stops SOS engine.
        """
        self.engine.stop()

    def parse_path(self, path):
        """
        Parse a PATH to extract namespace type, slave and resource on this slave
        :param path: path to parse
        :return: object of ParseResult type if parsing was successful, else None.
        """

        class ParseResult:
            """
            Dummy class to represent the result of the parse_path method.
            """

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
                        print_debug(dbg_levels.HTTP, ('HTTP request for admin namespace: {}, '
                                                      'remainder={}'.format(res.base, res.str_)))
                    elif ns.type is Conf.CfNsType.NS_SOS:
                        if len(path_list) <= len(ns.prefix):
                            raise Exception()
                        else:
                            sid = path_list[len(ns.prefix)]
                            res.base += '/' + sid
                            sid = int(sid)
                            res.slave = self.engine.find_slave(sid)
                            if res.slave is None or res.slave.status is not Slave.StatusCodes.SL_RUNNING:
                                raise Exception()
                            res.resource = res.slave.find_resource(path_list[len(ns.prefix) + 1:])
                            if res.resource is None:
                                raise Exception()
                            print_debug(dbg_levels.HTTP, ('HTTP request for SOS namespace : {}, '
                                                          'slave id= {}'.format(res.base, res.slave.id)))
                    elif ns.type is Conf.CfNsType.NS_WELL_KNOWN:
                        # No specific handling
                        print_debug(dbg_levels.HTTP, 'HTTP request for .well-known namespace.')
                    else:
                        raise Exception()
                    res.type = ns.type
        except Exception:
            res = None

        return res

    @staticmethod
    def split_path(path):
        """
        Splits a string containing a path in a list of the path components.
        :param path: path to split
        :return: list of path's components
        """
        if not path.startswith('/'):
            return []
        else:
            return path.split('/')[1:]  # Discard first empty string

    @asyncio.coroutine
    def handle_http(self, request_path, req, rep):
        """
        Handles HTTP requests. This method is a coroutine.
        :param request_path: RequestPath object returned by parse_path method
        :param req: received Request to process
        :param rep: Reply object
        """
        path_res = self.parse_path(request_path)
        if path_res is None:
            rep.code = HTTPCodes.HTTP_NOT_FOUND
        elif path_res.type in [Conf.CfNsType.NS_ADMIN, Conf.CfNsType.NS_SOS, Conf.CfNsType.NS_WELL_KNOWN]:
            if path_res.type is Conf.CfNsType.NS_ADMIN:
                self.http_admin(path_res, req, rep)
            elif path_res.type is Conf.CfNsType.NS_SOS:
                yield from self.http_sos(path_res, req, rep)
            elif path_res.type is Conf.CfNsType.NS_WELL_KNOWN:
                self.http_well_known(path_res, req, rep)
        else:
            rep.code = HTTPCodes.HTTP_NOT_FOUND

    def http_admin(self, res, req, rep):
        """
        Handle a HTTP request for admin namespace.
        :param res: RequestPath object returned by parse_path method
        :param req: received Request to process
        :param rep: Reply object
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

    @asyncio.coroutine
    def http_sos(self, res, req, rep):
        """
        Handle a HTTP request for SOS namespace. This function is a coroutine.
        :param res: RequestPath object returned by parse_path method
        :param req: received Request to process
        :param rep: Reply object
        """
        m = msg.Msg()
        code = {b'GET': msg.Msg.MsgCodes.MC_GET,
                b'HEAD': msg.Msg.MsgCodes.MC_GET,
                b'PUT': msg.Msg.MsgCodes.MC_PUT,
                b'POST': msg.Msg.MsgCodes.MC_POST,
                b'DELETE': msg.Msg.MsgCodes.MC_DELETE}[req.method]
        m.peer = res.slave
        m.msg_type = msg.Msg.MsgTypes.MT_CON
        m.msg_code = code
        res.resource.add_to_message(m)

        # Return 'Bad Request' if request is too large for slave MTU
        # TODO : find request length
        if m.paylen > res.slave.curmtu:
            rep.code = HTTPCodes.HTTP_BAD_REQUEST
            return
        # Is the request in the cache?
        mc = self.cache.get(m)
        if mc is not None:
            # Request is in the cache! Don't forward it.
            m = mc
            print_debug(dbg_levels.CACHE, 'Found request ' + str(m) + ' in cache.')
            print_debug(dbg_levels.CACHE, 'reply = ' + str(m.req_rep))
        else:
            # Request not found in cache : send it and wait for a reply.
            max_ = msg.exchange_lifetime(res.slave.l2_net.max_latency)
            # timeout = datetime.now() + timedelta(seconds=max)
            print_debug(dbg_levels.HTTP, 'HTTP request, timeout = ' + str(max_) + 's')
            self.engine.add_request(m)
            for i in range(int(max_) + 1):
                yield from asyncio.sleep(1)
                if m.req_rep is not None:
                    break
        r = m.req_rep
        if r is None:
            # No reply to our request
            rep.code = HTTPCodes.HTTP_SERVICE_UNAVAILABLE
        else:
            if mc is None:
                # Request wasn't in the cache, let's add it
                self.cache.add(m)
            # Get content format
            content_format = None
            for opt in r.optlist:
                if opt.optcode is Option.OptCodes.MO_CONTENT_FORMAT:
                    content_format = opt.optval
            content_format = content_format if content_format is not None else 0  # Default to text/plain
            # Get announced MTU
            for opt in r.optlist:
                if opt.optcode is Option.OptCodes.MO_SIZE1:
                    res.slave.curmtu = opt.optval
            rep.code = HTTPCodes.HTTP_OK

            rep.body = bytes()
            if content_format == 0:
                rep.body = r.payload
                rep.headers.append((b'Content-Type', b'text/plain'))
            elif content_format == 50:
                rep.body = r.payload
                rep.headers.append((b'Content-Type', b'application/json'))
            rep.headers.append((b'Content-Length', str(len(rep.body)).encode()))

    def http_well_known(self, res, req, rep):
        """
        Handle a HTTP request for '.well-known' namespace.
        :param res: RequestPath object returned by parse_path method
        :param req: received Request to process
        :param rep: Reply object
        """
        rep.code = HTTPCodes.HTTP_OK
        rep.body = self.engine.resource_list().encode()
        rep.headers = [(b'Content-Length', str(len(rep.body)).encode()),
                       (b'Content-Type', b'text/html')]
