#!/usr/bin/env python3

"""
Main CASAN program
"""

import sys
import argparse

assert sys.version >= '3.4', 'Please use Python 3.4 or higher.'

import conf
import master
import g


def run ():
    """
    Parse arguments, read the configuration file, and run the
    CASAN master
    """

    #
    # Parse arguments
    #

    mcat = '|'.join (g.d.all_categories ())
    parser = argparse.ArgumentParser (description='CASAN master')
    parser.add_argument ('-l', '--log-file', action='store',
                         help='Log file or -', metavar='logfile')
    parser.add_argument ('-d', '--debug', action='append', default=[],
                         help='Set debug modules', metavar=mcat)
    parser.add_argument ('-c', '--config', default='./casand.conf',
                         help='Set config file', metavar='file')
    args = parser.parse_args()

    #
    # Initialize event log and debug level
    #

    g.e.set_size (1000)

    g.d.set_file (args.log_file)
    if args.debug is not []:
        for c in args.debug:
            g.d.add_category (c)
    g.d.start ()

    #
    # Parse the configuration file
    #

    cf = conf.Conf ()
    cf.parse (args.config)

    #
    # Run the master with the just parsed configuration
    #

    m = master.Master (cf)
    m.run ()


if __name__ == '__main__':
    try:
        run ()
    except RuntimeError as e:
        print (e)
    except KeyboardInterrupt:
        pass
