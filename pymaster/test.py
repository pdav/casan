import conf
c = conf.Conf()
c.parse()
import sys
c.write_to_stream(sys.stdout)
