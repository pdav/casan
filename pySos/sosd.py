def run():
    # Read arguments
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', type=str, help='Set the level of debugging '
                        'verbosity', default='', metavar = '[[+|-]spec]...]')
    parser.add_argument('-c', type=str, help='Specify the configuration file '
                        'to use with pySos', default = './sosd.conf',
                        metavar = 'file')

    args = parser.parse_args()
    print('d : {}\nc : {}'.format(args.d, args.c))


if __name__ == '__main__':
    run()

