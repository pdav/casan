from sos import sos
from conf import Conf
from sos.l2_154 import l2net_154
from sos.slave import Slave
from util.debug import *


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
        # TODO : start HTTP servers

        # Initialize SOS engine last
        self.engine.init()

        return r

    def stop(self):
        self.engine.stop()

    def parse_path(self, path):
        class dummy:
            def __init__(self):
                self.type, self.base, self.slave, self.resource, self.str_ = (None,) * 5
        res = dummy()
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
                        print_debug(dbg_levels.HTTP, 'HTTP request for admin namespace: {}, remainder='.format(res.base, res.str_))
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
            return path.split()[1:]  # Discard first empty string

    def handle_http(self, request_path, req, rep):
        pass
