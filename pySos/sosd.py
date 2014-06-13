import signal
import argparse

def run():
    # Read arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', type=str, help='Set the level of debugging '
                        'verbosity', default='', metavar = '[[+|-]spec]...]')
    parser.add_argument('-c', type=str, help='Specify the configuration file '
                        'to use with pySos', default = './sosd.conf',
                        metavar = 'file')

    args = parser.parse_args()
    print('d : {}\nc : {}'.format(args.d, args.c))

# Signal related functions
def block_all_signals():
    nonlocal sigmask
    sigmask = signal.pthread_sigmask(signal.SIG_BLOCK, range(1, signal.NSIG))

def undo_block_all_signals():
    nonlocal sigmask
    signal.pthread_sigmask(signal.SIG_UNBLOCK, sigmask)

def wait_for_signal():
    sigset = {signal.SIGINT,signal.SIGQUIT, signal.SIGTERM}
    signal.pthread_sigmask(signal.SIG_BLOCK, sigset)
    signal.sigwait(sigset)


if __name__ == '__main__':
    run()

