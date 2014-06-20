'''
This module contains the Slave class
'''

from datetime import datetime, timedelta
from util.enum import Enum
from util.debug import *
from sos.sos import engine
from sos.msg import Msg
from .resource import Resource

class Slave:
    '''
    This class describes a slave in the SOS system.
    '''
    status_code = Enum('status_code', {'SL_INACTIVE' : 0,
                                       'SL_ASSOCIATING' : 0,
                                       'SL_RUNNING' : 1})
    res_status = Enum('res_status', ['S_START', 'S_RESOURCE', 'S_ENDRES',
                       'S_ATTRNAME', 'S_ATTRVAL_START', 'S_ATTRVAL_NQUOTED',
                       'S_ATTRVAL_QUOTED', 'S_ERROR'])

    def __init__(self):
        '''
        Default constructor
        '''
        self.slaveid = 0
        self.defmtu = 0
        self.curmtu = 0
        self.l2n = None
        self.addr = None
        self.init_ttl = 0
        self.status = self.status_code.SL_INACTIVE
        self.reslist = []
        self.next_timeout = datetime.now()

    def __str__(self):
        '''
        Returns a string describing the slave status and resources.
        '''
        s1 = 'slave ' + str(self.slaveid) + ' '
        s2 = ('INACTIVE' if self.status == self.status_code.SL_INACTIVE else
              'RUNNING (curmtu=' + str(self.curmtu) + ', ttl= ' +
                self.next_timeout.isoformat() + ')' 
                if self.status == self.statuc_code.SL_RUNNING else
              '(unknown state)')
        s3 = ' mac=' + str(self.addr) + '\n'
        return s1 + s2 + s3

    def reset(self):
        '''
        Resets the slave to it's default state
        '''
        self.addr = None
        self.l2n = None
        self.reslist.clear()
        self.curmtu = 0
        self.status = status_codes.SL_INACTIVE
        self.next_timeout = datetime.now()
        print_debug(debug_levels.STATE, 'Slave' + str(self.slaveid) + 
                    ' status set to INACTIVE.')

    def find_resource(self, res):
        '''
        Looks for a given resource on the slave.
        This functions expects the resource to be an iterable object
        containing the resource path, e.g for resource '/a/b/c' : ['a','b','c']
        If the resource is not found, returns None
        '''
        for r in self.reslist:
            if r == res:
                return r
        return None

    def resource_list(self):
        '''
        Returns the list of all the resources managed by the slave
        '''
        return self.reslist

    def process_sos(self, msg):
        '''
        This method is called by a receiver thread when the received message
        is a control message originated from this slave.
        The method implements the SOS control protocol for this slave, 
        and maintains the state associated to this slave.
        '''
        tp = msg.sos_type()
        if tp == Msg.sostype.SOS_DISCOVER:
            print_debug(debug_levels.STATE, '''Received DISCOVER, sending 
                                               ASSOCIATE''')
            answer = Msg()
            answer.peer = msg.peer
            answer.type_ = Msg.msgtype.MT_CON
            answer.code = Msg.msgcode.MC_POST
            answer.mk_cntl_assoc(self.init_ttl, self.curmtu)

            engine.add_request(answer)
        elif tp == Msg.sostype.SOS_ASSOC_REQUEST:
            print_debug(debug_levels.STATE, '''Received ASSOC_REQUEST from
                                               another master''')
        elif tp == Msg.sostype.SOS_ASSOC_ANSWER:
            if msg.reqrep is not None:
                print_debug(debug_levels.STATE, '''Received ASSOC ANSWER for
                                                   slave.''')
                print_debug(debug_levels.STATE, 'payload length=' + 
                                                str(len(msg.payload)))
                reslist = []
                if parse_resource_list(reslist, msg.payload):
                    print_debug(debug_levels.STATE, 'Slave ' + str(self.slaveid) + 
                                                    ' status set to RUNNING')
                    self.status = self.status_codes.SL_RUNNING
                    self.reslist = reslist
                    self.next_timeout = datetime.now() + timedelta(seconds = self.init_ttl)
                else:
                    print_debug(debug_levels.STATE, 'Slave ' + str(self.slaveid) + 
                                                    'cannot parse resource list.')
        elif tp == Msg.sostype.SOS_HELLO:
            print_debug(debug_levels.STATE, '''Received HELLO from another 
                                               master''')
        else:
            print_debug(debug_levels.STATE, '''Received an unrecognized 
                                               message''')

    def parse_resource_list(self, rlist, payload):
        attrname = ''
        cur_res = bytearray()
        def parse_single_byte(state, b):
            nonlocal rlist, attrname, cur_res
            if state == res_status.S_START:
                if b == b'<':
                    cur_res = bytearray()
                    return res_status.S_RESOURCE
                else:
                    return res_status.S_ERROR
            elif state == res_status.S_RESOURCE:
                if b == b'>':
                    rlist.append(Resource(cur_res.decode()))
                    return res_status.S_ENDRES
                else:
                    cur_res.append(b)
                    return state
            elif state == res_status.S_ENDRES:
                attrname = ''
                if b == b';':
                    return res_status.S_ATTRNAME
                elif b == b',':
                    return res_status.S_START
                else:
                    return res_status.S_ERROR
            elif state == res_status.S_ATTRNAME:
                if b == b'=':
                    if attrname == '':
                        return res_status.S_ERROR
                    else:
                        return res_status.S_ATTRVAL_START
                elif b == b';':
                    rlist[len(rlist)-1].add_attribute(attrname, '')
                    attrname = ''
                    return state
                elif b == b',':
                    rlist[len(rlist)-1].add_attribute(attrname, '')
                    return res_status.S_START
                else:
                    attrname = attrname + b.decode()
                    return state
            elif state == res_status.S_ATTRVAL_START:
                cur_res = bytearray()
                if b == b'"':
                    return res_status.S_ATTRVAL_QUOTED
                elif b == b',':
                    rlist[len(rlist)-1].add_attribute(attrname, '')
                    return res_status.S_START
                elif b == b';':
                    rlist[len(rlist)-1].add_attribute(attrname, '')
                    attrname = ''
                    return res_status.S_ATTRNAME
                else:
                    cur_res.append(b)
                    return res_status.S_ATTRVAL_NQUOTED
            elif state == res_status.S_ATTRVAL_QUOTED:
                if b == b'"':
                    rlist[len(rlist)-1].add_attribute(attrname, cur_res.decode())
                    return res_status.S_ENDRES
                else:
                    cur_res.append(b)
                    return state
            elif state == res_status.S_ATTRVAL_NQUOTED:
                if b == b',':
                    rlist[len(rlist)-1].add_attribute(attrname, cur_res.decode())
                    return res_status.S_START
                elif b == b';':
                    rlist[len(rlist)-1].add_attribute(attrname, cur_res.decode())
                    attrname = ''
                    return res_status.S_ATTRNAME
                else:
                    cur_res.append(b)
            else:
                res_status.S_ERROR

            state = res_status.S_START
            for b in payload:
                state = parse_single_byte(state, b)
                if state == res_status.S_ERROR:
                    break
        # Handle terminale states
        if state == res_status.S_ATTRNAME:
            if attrname == '':
                rlist[len(rlist)-1].add_attribute(attrname, '')
        elif state == res_status.S_ATTRVAL_START:
            rlist[len(rlist)-1].add_attribute(attrname, '')
        elif state == res_status.S_NQUOTED:
            rlist[len(rlist)-1].add_attribute(attrname, cur_res.decode())
        else:
            state = res_status.S_ERROR
        return state != res_status.S_ERROR
