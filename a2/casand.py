#!/usr/bin/env python3

import sys
import argparse

assert sys.version >= '3.4', 'Please use Python 3.4 or higher.'

import conf
import master

def run ():
    """
    Main CASAN program
    """

    #
    # Parse arguments
    #

    parser = argparse.ArgumentParser (description='CASAN master')
    parser.add_argument ('-d', '--debug', default=None,
                         help='Set debugging level', metavar='[[+|-]spec]...]')
    parser.add_argument ('-c', '--config', default='./casand.conf',
                         help='Set debugging level', metavar='file')
    args = parser.parse_args()

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
