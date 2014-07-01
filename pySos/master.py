from sos import sos
from conf import Conf
from sos.l2_154 import l2net_154
from sos.slave import Slave
import pdb

class Master:
    def start(self, cf):
        r = True
        sos.engine.timer_first_hello = cf.timers[cf.CfTimerIndex.I_FIRST_HELLO]
        sos.engine.timer_interval_hello = cf.timers[cf.CfTimerIndex.I_INTERVAL_HELLO]
        sos.engine.timer_slave_ttl = cf.timers[cf.CfTimerIndex.I_SLAVE_TTL]
        sos.engine.init()
        for net in cf.netlist:
            if net.type is Conf.NetType.NET_154:
                l2n = l2net_154()
                l2n.init(net.net_154.iface, 'xbee' if net.net_154.type is Conf.Net154Type.NET_154_XBEE else None,
                         net.mtu, net.net_154.addr, net.net_154.panid, net.net_154.channel)
                sos.engine.start_net(l2n)
        for s in cf.slavelist:
            sl = Slave()
            sl.id = s.id
            sl.defmtu = s.mtu
            sl.ttl = s.ttl if s.ttl != 0 else sos.engine.timer_slave_ttl
            sos.engine.slist.append(sl)
        # TODO : start HTTP servers

    def stop(self):
        sos.engine.stop()
    def handle_http(self, request_path, req, rep):
        pass
