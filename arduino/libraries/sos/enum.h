#ifndef __ENUM_H__
#define __ENUM_H__

typedef enum coap_request_method {
	COAP_METHOD_GET = 1,
	COAP_METHOD_PUT,
	COAP_METHOD_POST,
	COAP_METHOD_DELETE
} coap_req_meth_t;

typedef enum coap_message_type {
	COAP_TYPE_CON = 0,
	COAP_TYPE_NON,
	COAP_TYPE_ACK,
	COAP_TYPE_RST
} coap_msg_t;

typedef enum pktype { PK_ME, PK_BCAST, PK_NONE } pktype_t ;

typedef enum eth_recv {
	ETH_RECV_EMPTY, 
	ETH_RECV_WRONG_SENDER, 
	ETH_RECV_WRONG_ETHTYPE, 
	ETH_RECV_WRONG_DEST,
	ETH_RECV_RECV_OK
} eth_recv_t;


enum sos_slave_status {
	SL_COLDSTART = 1,
	SL_WAITING_UNKNOWN,
	SL_RUNNING,
	SL_RENEW,
	SL_WAITING_KNOWN,
} ;

#endif
