#ifndef __ENUM_H__
#define __ENUM_H__

typedef enum coap_code {
	COAP_CODE_EMPTY = 0,
	COAP_CODE_GET,
	COAP_CODE_POST,
	COAP_CODE_PUT,
	COAP_CODE_DELETE
} coap_code_t;

typedef enum coap_type {
	COAP_TYPE_CON = 0,
	COAP_TYPE_NON,
	COAP_TYPE_ACK,
	COAP_TYPE_RST
} coap_type_t;

typedef enum pktype { 
	PK_ME, 
	PK_BCAST, 
	PK_NONE 
} pktype_t ;

typedef enum eth_recv {
	ETH_RECV_EMPTY, 
	ETH_RECV_WRONG_SENDER, 
	ETH_RECV_WRONG_ETHTYPE, 
	ETH_RECV_WRONG_DEST,
	ETH_RECV_WRONG_SIZE,
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
