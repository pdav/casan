import l2

class l2addr_154(l2.l2addr):
    '''
    This class represents a 802.15.4 network address.
    '''
    def __init__(self, s):
        '''
        Initializes the object from a string representation such as 'ca:fe'
        '''
        sl = s.split(':')
        self.addr = bytes([int(ss, 16) for ss in sl])

    def __eq__(self, other):
        return self.addr == other.addr

    def __ne__(self, other):
        return self.addr != other.addr

    def __repr__(self):
        rep = ''
        for b in self.addr:
            rep = rep + hex(b)[2:]
        return rep
        
    def __str__(self):
        rep = []
        for b in self.addr:
            atom = hex(b)[2:]
            if len(atom) == 1 : atom = '0' + atom # Prefix single-digits with 0
            rep.append(atom)
        return ':'.join(rep)

