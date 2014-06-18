import signal
import argparse
from sys import stderr
from util.debug import *
import conf
from master import Master

import time
#import debug

def run():
    # Read arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', type=str, help='Set the level of debugging '
                        'verbosity', default='', metavar = '[[+|-]spec]...]')
    parser.add_argument('-c', type=str, help='Specify the configuration file '
                        'to use with pySos', default = './sosd.conf',
                        metavar = 'file')

    args = parser.parse_args()

    cf = conf.Conf() 
    if not cf.parse(args.c):
        stderr.write('Cannot parse configuration file ' + args.c + 
                     '\nAborting.\n')
    print_debug(dbg_levels.CONF, 'Read configuration :\n' + cf.to_string())

    #block_all_signals()
    # Start the master
    master = Master()
    master.start(cf)

    print('Waiting 3 seconds')
    time.sleep(3)
    print('Done waiting! Killing all.')

    master.stop()

    #undo_block_all_signals()
    #wait_for_signal()

# Signal related functions
def block_all_signals():
    global sigmask
    sigmask = signal.pthread_sigmask(signal.SIG_SETMASK, {sig for sig in range(1, signal.NSIG)})

def undo_block_all_signals():
    global sigmask
    signal.pthread_sigmask(signal.SIG_UNBLOCK, sigmask)

def wait_for_signal():
    sigset = {signal.SIGINT,signal.SIGQUIT, signal.SIGTERM}
    signal.pthread_sigmask(signal.SIG_BLOCK, sigset)
    signal.sigwait(sigset)


if __name__ == '__main__':
    run()

