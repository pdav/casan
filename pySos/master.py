import sos.sos
from conf import Conf
from sos.l2_154 import l2net_154
import pdb

class Master:
    def start(self, cf):
        r = True
        sos.sos.engine.init()
        for net in cf.netlist:
            if net.type is Conf.net_type.NET_154:
                l2n = l2net_154()
                l2n.init(net.net_154.iface,
                  'xbee' if net.net_154.type is Conf.net_154_type.NET_154_XBEE else None,
                  net.mtu, net.net_154.addr, net.net_154.panid, net.net_154.channel)
                sos.sos.engine.start_net(l2n)


    def stop(self):
        sos.sos.engine.stop()
    def handle_http(self, request_path, req, rep):
        pass
