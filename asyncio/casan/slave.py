"""
This module contains the Slave class
"""

from datetime import datetime, timedelta
from enum import Enum
from functools import reduce

from util.debug import print_debug, dbg_levels
from casan import msg
from .resource import Resource


class Slave:
    """
    This class describes a slave in the CASAN system.
    """
    class StatusCodes(Enum):
        """
        Possible status codes for slaves.
        """
        SL_INACTIVE = 0
        SL_ASSOCIATING = 0
        SL_RUNNING = 1

    class ResStatus(Enum):
        """
        Possible resource status codes.
        """
        S_START = 0
        S_RESOURCE = 1
        S_ENDRES = 2
        S_ATTRNAME = 3
        S_ATTRVAL_START = 4
        S_ATTRVAL_NQUOTED = 5
        S_ATTRVAL_QUOTED = 6
        S_ERROR = 7

    def __init__(self):
        """
        Default constructor
        """
        self.id = 0
        self.def_mtu = 0
        self.curmtu = 0
        self.l2_net = None
        self.addr = None
        self.init_ttl = 0
        self.status = self.StatusCodes.SL_INACTIVE
        self.res_list = []
        self.next_timeout = datetime.now()

    def __str__(self):
        """
        Returns a string describing the slave status and resources.
        """
        l1 = 'slave ' + str(self.id) + ' '
        l2 = ('INACTIVE' if self.status is self.StatusCodes.SL_INACTIVE else
              'RUNNING (curmtu=' + str(self.curmtu) + ', ttl= ' +
              self.next_timeout.isoformat() + ')'
              if self.status is self.StatusCodes.SL_RUNNING else
              '(unknown state)')
        l3 = ' mac=' + str(self.addr) + '\n'
        res = '\t' + reduce(lambda s1, s2: s1 + s2, (str(w) for w in self.res_list), '') + '\n'
        return l1 + l2 + l3 + res

    def reset(self):
        """
        Resets the slave to it's default state
        """
        self.addr = None
        self.l2_net = None
        self.res_list.clear()
        self.curmtu = 0
        self.status = self.StatusCodes.SL_INACTIVE
        self.next_timeout = datetime.now()
        print_debug(dbg_levels.STATE, 'Slave' + str(self.id) + 
                    ' status set to INACTIVE.')

    def find_resource(self, res):
        """
        Looks for a given resource on the slave.
        This function expects the resource to be an iterable object
        containing the resource path, e.g for resource '/a/b/c' : ['a','b','c']
        If the resource is not found, returns None
        :param res: iterable object containing the resource path.
        """
        for r in self.res_list:
            if r == res:
                return r
        return None

    def resource_list(self):
        """
        Returns the list of all the resources managed by the slave
        """
        return self.res_list

    def process_casan(self, casan_instance, mess):
        """
        This method is called by a receiver thread when the received message
        is a control message originated from this slave.
        The method implements the CASAN control protocol for this slave, 
        and maintains the state associated to this slave.
        :param casan_instance: CASAN engine instance.
        :param mess: message to process.
        """
        tp = mess.casan_type
        if tp is msg.Msg.CasanTypes.CASAN_DISCOVER:
            print_debug(dbg_levels.STATE, 'Received DISCOVER, sending ASSOCIATE')
            answer = msg.Msg()
            answer.peer = mess.peer
            answer.msg_type = msg.Msg.MsgTypes.MT_CON
            answer.msg_code = msg.Msg.MsgCodes.MC_POST
            answer.mk_ctrl_assoc(self.init_ttl, self.curmtu)

            casan_instance.add_request(answer)

        elif tp is msg.Msg.CasanTypes.CASAN_ASSOC_REQUEST:
            print_debug(dbg_levels.STATE, 'Received ASSOC_REQUEST from another master')
        elif tp is msg.Msg.CasanTypes.CASAN_ASSOC_ANSWER:
            if mess.req_rep is not None:
                print_debug(dbg_levels.STATE, 'Received ASSOC ANSWER for slave.')
                print_debug(dbg_levels.STATE, 'payload length=' + str(len(mess.payload)))
                reslist = []
                if self.parse_resource_list(reslist, mess.payload):
                    print_debug(dbg_levels.STATE, 'Slave ' + str(self.id) + ' status set to RUNNING')
                    self.status = self.StatusCodes.SL_RUNNING
                    self.res_list = reslist
                    self.next_timeout = datetime.now() + timedelta(seconds=self.init_ttl)
                else:
                    print_debug(dbg_levels.STATE, 'Slave ' + str(self.id) + ' cannot parse resource list.')
        elif tp is msg.Msg.CasanTypes.CASAN_HELLO:
            print_debug(dbg_levels.STATE, 'Received HELLO from another master')
        else:
            print_debug(dbg_levels.STATE, 'Received an unrecognized message')

    def parse_resource_list(self, rlist, payload):
        """
        Parses the resource list for a slave.
        :param rlist: list used to store the parsed resource list.
        :param payload: payload of a CASAN association message.
        """
        attrname = ''
        cur_res = bytearray()

        def parse_single_byte(state, b):
            """
            Parses a single byte from the resource list.
            :param state: current state of the parser
            :param b: byte to parse
            :return: new state of the parser
            """
            nonlocal rlist, attrname, cur_res
            if state is self.ResStatus.S_START:
                if b == b'<':
                    cur_res = bytearray()
                    return self.ResStatus.S_RESOURCE
                else:
                    return self.ResStatus.S_ERROR
            elif state is self.ResStatus.S_RESOURCE:
                if b == b'>':
                    rlist.append(Resource(cur_res.decode()))
                    return self.ResStatus.S_ENDRES
                else:
                    cur_res += b
                    return state
            elif state is self.ResStatus.S_ENDRES:
                attrname = ''
                if b == b';':
                    return self.ResStatus.S_ATTRNAME
                elif b == b',':
                    return self.ResStatus.S_START
                else:
                    return self.ResStatus.S_ERROR
            elif state is self.ResStatus.S_ATTRNAME:
                if b == b'=':
                    if attrname == '':
                        return self.ResStatus.S_ERROR
                    else:
                        return self.ResStatus.S_ATTRVAL_START
                elif b == b';':
                    rlist[len(rlist)-1].add_attribute(attrname, '')
                    attrname = ''
                    return state
                elif b == b',':
                    rlist[len(rlist)-1].add_attribute(attrname, '')
                    return self.ResStatus.S_START
                else:
                    attrname = attrname + b.decode()
                    return state
            elif state is self.ResStatus.S_ATTRVAL_START:
                cur_res = bytearray()
                if b == b'"':
                    return self.ResStatus.S_ATTRVAL_QUOTED
                elif b == b',':
                    rlist[len(rlist)-1].add_attribute(attrname, '')
                    return self.ResStatus.S_START
                elif b == b';':
                    rlist[len(rlist)-1].add_attribute(attrname, '')
                    attrname = ''
                    return self.ResStatus.S_ATTRNAME
                else:
                    cur_res += b
                    return self.ResStatus.S_ATTRVAL_NQUOTED
            elif state is self.ResStatus.S_ATTRVAL_QUOTED:
                if b == b'"':
                    rlist[len(rlist)-1].add_attribute(attrname, cur_res.decode())
                    return self.ResStatus.S_ENDRES
                else:
                    cur_res += b
                    return state
            elif state is self.ResStatus.S_ATTRVAL_NQUOTED:
                if b == b',':
                    rlist[len(rlist)-1].add_attribute(attrname, cur_res.decode())
                    return self.ResStatus.S_START
                elif b == b';':
                    rlist[len(rlist)-1].add_attribute(attrname, cur_res.decode())
                    attrname = ''
                    return self.ResStatus.S_ATTRNAME
                else:
                    cur_res += b
            else:
                return self.ResStatus.S_ERROR

        state_ = self.ResStatus.S_START
        # Iterating on a bytes object yields integers but parse_single_byte takes bytes.
        # So, iterate on indices and give it slices of length 1.
        for i in range(len(payload)):
            state_ = parse_single_byte(state_, payload[i:i+1])
            if state_ is self.ResStatus.S_ERROR:
                break

        # Handle terminal states
        if state_ is self.ResStatus.S_ATTRNAME:
            if attrname == '':
                rlist[len(rlist)-1].add_attribute(attrname, '')
        elif state_ is self.ResStatus.S_ATTRVAL_START:
            rlist[len(rlist)-1].add_attribute(attrname, '')
        elif state_ is self.ResStatus.S_ATTRVAL_NQUOTED:
            rlist[len(rlist)-1].add_attribute(attrname, cur_res.decode())
        elif state_ is self.ResStatus.S_ENDRES:
            pass
        else:
            state_ = self.ResStatus.S_ERROR
        return state_ is not self.ResStatus.S_ERROR