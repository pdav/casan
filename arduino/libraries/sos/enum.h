#ifndef __ENUM_H__
#define __ENUM_H__

enum coap_request_method {
	COAP_METHOD_GET = 1,
	COAP_METHOD_PUT,
	COAP_METHOD_POST,
	COAP_METHOD_DELETE
} ;

enum coap_message_type {
	COAP_TYPE_CON = 1,
	COAP_TYPE_NON,
	COAP_TYPE_ACK,
	COAP_TYPE_RST
} ;

typedef enum pktype { PK_ME, PK_BCAST, PK_NONE } pktype_t ;

enum sos_slave_status {
	SL_COLDSTART = 1,
	SL_WAITING_UNKNOWN,
	SL_RUNNING,
	SL_RENEW,
	SL_WAITING_KNOWN,
} ;

#endif
