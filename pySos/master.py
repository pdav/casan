import sos.sos
import pdb

class Master:
    def start(self, cf):
        r = True
        sos.sos.engine.init()
        for i in range(3):
           sos.sos.engine.start_net(None)

    def stop(self):
        sos.sos.engine.stop()
    def handle_http(self, request_path, req, rep):
        pass
