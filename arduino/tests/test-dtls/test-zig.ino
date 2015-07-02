#include <ZigMsg.h>
#include <stdarg.h>

#define	CHANNEL	25		// use "c" to change it while running

#define	PANID		CONST16 (0xbe, 0xef)
#define	SENDADDR	CONST16 (0x01, 0x02)
#define	RECVADDR	CONST16 (0x10, 0x11)

#define	MSGBUF_SIZE	10	// each msgbuf consumes ~130 bytes


#ifdef __GNUC__
#define UNUSED_PARAM __attribute__((unused))
#else
#define UNUSED_PARAM
#endif /* __GNUC__ */

#define	PERIODIC	100000

int channel = CHANNEL ;

extern "C" {
//#define MSG_DEBUG

#include "free_memory.h"
#include <tinydtls.h>
#include <dtls.h>
};

//#define PSK_DEFAULT_IDENTITY "Client_identity"
#define PSK_DEFAULT_KEY      "secretPSK"
#define LOGLVL          DTLS_LOG_WARN

/******************************************************************************
Utilities
*******************************************************************************/

#define	HEXCHAR(c)	((c) < 10 ? (c) + '0' : (c) - 10 + 'a')

void print_free_mem()
{
    int memory = freeMemory();
    Serial.print(F("mémoire disponible : "));
    Serial.println(memory);
    //delay(1000);
}

unsigned long get_random (unsigned int max) {
    return random(max);
}

unsigned long get_the_time (void) {
    return millis();
}

void instant_print (char * format, ...)
{
    va_list args;
    va_start(args, format);
    char msg[100];
    vsnprintf(msg, sizeof msg, format, args);
    va_end(args);
    Serial.print(msg);
}

void something_to_say (log_t loglvl, char * format, ...)
{
    if(loglvl < LOGLVL)
        return;

    va_list args;
    va_start(args, format);
    char msg[100];
    vsnprintf(msg, sizeof msg, format, args);
    va_end(args);

    Serial.print(msg);

    if(strlen(msg) > 4) {
        if(msg[0] == 'W' && msg[1] == 'A' && msg[2] == 'I' && msg[3] == 'T')
            delay(2000);
    }
}

static inline size_t
print_timestamp_(char *s, size_t len, clock_time_t t)
{
  return snprintf(s, len, "%u.%03u"
		  , (unsigned int)(t / CLOCK_SECOND)
		  , (unsigned int)(t % CLOCK_SECOND));
}

void 
something_to_hexdump(log_t loglvl, char *name, unsigned char *buf
        , size_t length, int extend)
{

    if(loglvl < LOGLVL)
        return;

    static char timebuf[32];
    int n = 0;

    if (print_timestamp_(timebuf,sizeof(timebuf), get_the_time()))
        instant_print(const_cast<char *>("%s "), timebuf);

    if (extend) {
        instant_print(const_cast<char *>("%s: (%d bytes):\n\r"), name, length);

        while (length--) {
            if (n % 16 == 0)
                instant_print(const_cast<char *>("%08X "), n);

            instant_print(const_cast<char *>("%02X "), *buf++);

            n++;
            if (n % 8 == 0) {
                if (n % 16 == 0)
                    instant_print(const_cast<char *>("\n\r"));
                else
                    instant_print(const_cast<char *>(" "));
            }
        }
    } else {
        instant_print(const_cast<char *>("%s: (%d bytes): "), name, length);
        while (length--) 
            instant_print(const_cast<char *>("%02X"), *buf++);
    }
    instant_print(const_cast<char *>("\n\r"));
}

void say (char * txt_to_say)
{
    Serial.print(txt_to_say);
    //delay(1000);
}

void phex (uint8_t c)
{
    char x ;
    x = (c >> 4) & 0xf ; x = HEXCHAR (x) ; Serial.print (x) ;
    x = (c     ) & 0xf ; x = HEXCHAR (x) ; Serial.print (x) ;
}

void pdec (int c, int ndigits, char fill)
{
    int c2 ;
    int n ;

    /*
     * compute number of digits of c
     */

    c2 = c ;
    n = 1 ;
    while (c2 > 0)
    {
	n++ ;
	c2 /= 10 ;
    }

    /*
     * Complete with fill characters
     */

    for ( ; n <= ndigits ; n++)
	Serial.print (fill) ;
    Serial.print (c) ;
}

void phexn (uint8_t *c, int len, char sep)
{
    int i ;

    for (i = 0 ; i < len ; i++)
    {
	if (i > 0 && sep != '\0')
	    Serial.print (sep) ;
	phex (c [i]) ;
    }
}

/* flen = field length = nominal number of chars in a "normal" row */
void phexascii (uint8_t buf [], int len, int flen)
{
    int i ;

    phexn (buf, len, ' ') ;
    for (i = 0 ; i < (flen - len) * 3 + 4 ; i++)
	Serial.print (' ') ;
    for (i = 0 ; i < len ; i++)
	Serial.print ((char) (buf [i] >= ' ' && buf [i] < 127 ? buf [i] : '.')) ;
}

/******************************************************************************
CoAP decoding
*******************************************************************************/

#define	COAP_VERSION(b)	(((b) [0] >> 6) & 0x3)
#define	COAP_TYPE(b)	(((b) [0] >> 4) & 0x3)
#define	COAP_TOKLEN(b)	(((b) [0]     ) & 0xf)
#define	COAP_CODE(b)	(((b) [1]))
#define	COAP_ID(b)	(((b) [2] << 8) | (b) [3])

#define	NTAB(t)		((int) (sizeof (t) / sizeof (t)[0]))

const char *type_string [] = { "CON", "NON", "ACK", "RST" } ;
const char *code_string [] = { "Empty msg", "GET", "POST", "PUT", "DELETE" } ;


/******************************************************************************
Raw Sniffer
*******************************************************************************/

const char *desc_frametype [] =
{
    "Beacon", "Data", "Ack", "MAC-cmd",
    "Reserved", "Reserved", "Reserved", "Reserved", 
} ;

uint8_t *padrpan (uint8_t *p, int len, int ispan, int disp)
{
    uint8_t *adr = p ;

    if (ispan)
	adr += 2 ;
    if (disp)
	phexn (adr, len, ':') ;
    adr += len ;
    if (disp && ispan)
    {
	Serial.print ('/') ;
	phexn (p, 2, ':') ;
    }
    return adr ;
}

uint8_t *padr (int mode, uint8_t *p, int ispan, int disp)
{
    switch (mode)
    {
	case Z_ADDRMODE_NOADDR :
	    if (disp)
		Serial.print ("()") ;
	    break ;
	case Z_ADDRMODE_RESERVED :
	    if (disp)
		Serial.print ("?reserved") ;
	    break ;
	case Z_ADDRMODE_ADDR2 :
	    p = padrpan (p, 2, ispan, disp) ;
	    break ;
	case Z_ADDRMODE_ADDR8 :
	    p = padrpan (p, 6, ispan, disp) ;
	    break ;
    }
    return p ;
}

int lastseq = -1 ;

void print_frame (ZigMsg::ZigReceivedFrame *z, bool casan_decode)
{
    uint8_t *p, *dstp ;
    int intrapan ;
    int dstmode, srcmode ;
    int seq ;

    dstmode = Z_GET_DST_ADDR_MODE (z->fcf) ;
    srcmode = Z_GET_SRC_ADDR_MODE (z->fcf) ;
    intrapan = Z_GET_INTRA_PAN (z->fcf) ;

    dstp = z->rawframe + 3 ;
    p = padr (dstmode, dstp, 1, 0) ;		// do not print dst addr
    p = padr (srcmode, p, ! intrapan, 1) ;	// p points past addresses
    Serial.print ("->") ;
	(void) padr (dstmode, dstp, 1, 1) ;

    Serial.print (" ") ;
    Serial.print (desc_frametype [Z_GET_FRAMETYPE (z->fcf)]) ;
    Serial.print (" Sec=") ;   Serial.print (Z_GET_SEC_ENABLED (z->fcf)) ;
    Serial.print (" Pendg=") ; Serial.print (Z_GET_FRAME_PENDING (z->fcf)) ;
    Serial.print (" AckRq=") ; Serial.print (Z_GET_ACK_REQUEST (z->fcf)) ;
    Serial.print (" V=") ;     Serial.print (Z_GET_FRAME_VERSION (z->fcf)) ;

    Serial.print (" Seq=") ;
    seq = z->rawframe [2] ;
    phex (seq) ;

    Serial.print (" Len=") ;
    Serial.print (z->paylen) ;

    Serial.print (" [") ;
    phexn (p, z->paylen, ',') ;
    Serial.print ("] LQI=") ;	Serial.print (z->lqi) ;
    Serial.println () ;

    //if (casan_decode && Z_GET_FRAMETYPE (z->fcf) == Z_FT_DATA && lastseq != seq)
	//coap_decode (p, z->paylen) ;
    lastseq = seq ;
}

void print_stat (void)
{
    ZigMsg::ZigStat *zs = zigmsg.getstat () ;
    Serial.print ("RX overrun=") ;
    Serial.print (zs->rx_overrun) ;
    Serial.print (", crcfail=") ;
    Serial.print (zs->rx_crcfail) ;
    Serial.print (", TX sent=") ;
    Serial.print (zs->tx_sent) ;
    Serial.print (", e_cca=") ;
    Serial.print (zs->tx_error_cca) ;
    Serial.print (", e_noack=") ;
    Serial.print (zs->tx_error_noack) ;
    Serial.print (", e_fail=") ;
    Serial.print (zs->tx_error_fail) ;
    Serial.println () ;
}

void snif (bool casan_decode)
{
    static int n = 0 ;
    ZigMsg::ZigReceivedFrame *z ;

    if (++n % PERIODIC == 0)
    {
	print_stat () ;
	n = 0 ;
    }

    while ((z = zigmsg.get_received ()) != NULL)
    {
	print_frame (z, casan_decode) ;
	zigmsg.skip_received () ;
    }
}


/******************************************************************************
CoAP and raw sniffer
*******************************************************************************/

void init_snif (char line [])
{
    zigmsg.channel (channel) ;
    zigmsg.promiscuous (true) ;
    zigmsg.start () ;
    Serial.println (F("Starting sniffer")) ;
}

void stop_snif (void)
{
    Serial.println (F("Stopping sniffer")) ;
}

void do_snif (void)
{
    snif (false) ;
}

/******************************************************************************
Sender
*******************************************************************************/

void init_send (char line [])
{
    zigmsg.channel (channel) ;
    zigmsg.panid (PANID) ;
    zigmsg.addr2 (SENDADDR) ;
    zigmsg.promiscuous (false) ;
    zigmsg.start () ;
    Serial.println("Starting send") ;
}

void stop_send (void)
{
    Serial.println(F("Stopping send")) ;
}

void do_send (void)
{
    static int n = 0 ;
    ZigMsg::ZigStat *zs ;

    if (++n % PERIODIC == 0)
    {
	zs = zigmsg.getstat () ;
	Serial.print (F("RX overrun=")) ;
	Serial.print (zs->rx_overrun) ;
	Serial.print (F(", crcfail=")) ;
	Serial.print (zs->rx_crcfail) ;
	Serial.print (F(", TX sent=")) ;
	Serial.print (zs->tx_sent) ;
	Serial.print (F(", e_cca=")) ;
	Serial.print (zs->tx_error_cca) ;
	Serial.print (F(", e_noack=")) ;
	Serial.print (zs->tx_error_noack) ;
	Serial.print (F(", e_fail=")) ;
	Serial.print (zs->tx_error_fail) ;
	Serial.println () ;
	n = 0 ;

	uint32_t time ;
	time = millis () ;
	if (zigmsg.sendto (RECVADDR, 4, (uint8_t *) &time))
	    Serial.println (F("Sent")) ;
	else
	    Serial.println (F("Sent error")) ;
    }
}

/******************************************************************************
Receiver
*******************************************************************************/

void init_recv (char line [])
{
    zigmsg.channel (channel) ;
    zigmsg.panid (PANID) ;
    zigmsg.addr2 (RECVADDR) ;
    zigmsg.promiscuous (false) ;
    zigmsg.start () ;
    Serial.println(F("Starting receiver")) ;
}

void stop_recv (void)
{
    Serial.println(F("Stopping receiver")) ;
}

void do_recv (void)
{
    do_snif () ;
}

/******************************************************************************
Channel
*******************************************************************************/

void init_chan (char line [])
{
    char *p ;
    int ch = 0 ;

    p = line ;
    while (*p == ' ' || *p == '\t')
	p++ ;

    while (*p >= '0' && *p <= '9')
    {
	ch = ch * 10 + (*p - '0') ;
	p++ ;
    }

    if (ch < 11 || ch > 26)
	Serial.println (F("Invalid channel")) ;
    else
    {
	channel = ch ;
	Serial.print (F("Channel set to ")) ;
	Serial.println (channel) ;
    }

    Serial.println (F("Entering idle mode")) ;
}

void stop_chan (void)
{
}

void do_chan (void)
{
}

//  DTLS common

static unsigned char buf[DTLS_MAX_BUF];
static size_t len = 0;
session_t session;
dtls_context_t *the_context = NULL;

static int
send_to_peer(struct dtls_context_t *ctx, 
        session_t *session, uint8 *data, size_t len)
{
    int ret = zigmsg.sendto(session->addr, len, data);
    return ret ? len : -1;
}

static void
try_send(struct dtls_context_t *ctx)
{
    int res;
    res = dtls_write(ctx, &session, (uint8 *)buf, len);
    if (res >= 0) {
        memmove(buf, buf + res, len - res);
        len -= res;
    }
}

// DTLS server

// get_psk_info became useless, we already know the key

static int
read_from_peer(struct dtls_context_t *ctx, 
        session_t *session, uint8 *data, size_t len)
{
    //Serial.println(F("READ FROM PEER"));
    return 1;
}

static int
dtls_handle_read(void)
{

    ZigMsg::ZigReceivedFrame *z ;
    size_t i = 0;
    while ((z = zigmsg.get_received ()) != NULL)
    {
        memcpy(buf, z->payload, z->paylen);
        len += z->paylen; // TODO FIXME : pas sûr de += ou =
        zigmsg.skip_received () ;

        dtls_debug_hexdump("RECEIVED MSG", buf, len);

        int ret = dtls_handle_message(the_context, &session, (uint8_t*)buf, len);
        len = 0; // TODO do we have to put the length of the received pkt to 0 ?

        if(ret) {
            Serial.print(F("\r\nerr d_hdl_msg > d_h_read : "));
            Serial.println(ret);
            return ret;
        }

        i++;
    }

#if 0
        // TODO
        if (len >= strlen(DTLS_CLIENT_CMD_CLOSE) &&
                !memcmp(buf, DTLS_CLIENT_CMD_CLOSE
                    , strlen(DTLS_CLIENT_CMD_CLOSE)))
        {
            Serial.println(F("client: closing connection"));
            dtls_close(the_context, &session);
            len = 0;
        } 
        else if (len >= strlen(DTLS_CLIENT_CMD_RENEGOTIATE) &&
                !memcmp(buf, DTLS_CLIENT_CMD_RENEGOTIATE
                    , strlen(DTLS_CLIENT_CMD_RENEGOTIATE))) 
        {
            Serial.println(F("client: renegotiate connection"));
            dtls_renegotiate(the_context, &session);
            len = 0;
        } else {
            try_send(the_context);
        }
#endif

    dtls_peer_t * peer = dtls_get_peer(the_context, &session);
    // if no received message and handshake complete
    if(i == 0 && peer && peer->state == DTLS_STATE_CONNECTED)
    {
        int res;
        buf[0] = 'C';
        buf[1] = 'O';
        buf[2] = 'U';
        buf[3] = 'C';
        buf[4] = 'O';
        buf[5] = 'U';
        buf[6] = '\0';
        len = 7;
        try_send(the_context);
    }

    return 1;
}

static dtls_handler_t cb_server = {
    .write = send_to_peer,
    .read  = read_from_peer,
    .event = NULL,
    .get_psk_info = NULL,
};

void init_dtls_server (char line [])
{
    zigmsg.channel (channel) ;
    zigmsg.panid (PANID) ;
    zigmsg.addr2 (RECVADDR) ;
    zigmsg.promiscuous (false) ;
    zigmsg.start () ;

    memset(&session, 0, sizeof(session_t));
    session.addr = SENDADDR;
    session.size = sizeof(session.addr);

    //log_t log_level = DTLS_LOG_WARN;
    //dtls_set_log_level(log_level);

    randomSeed(get_the_time());
    dtls_init(get_the_time);

    the_context = dtls_new_context(get_random);
    the_context->say = say;
    the_context->smth_to_say = something_to_say;
    the_context->smth_to_hexdump = something_to_hexdump;

    memcpy(the_context->psk.key, PSK_DEFAULT_KEY, strlen(PSK_DEFAULT_KEY));
    the_context->psk.len = strlen(PSK_DEFAULT_KEY);

    dtls_set_handler(the_context, &cb_server);
}

void stop_dtls_server (void)
{
    dtls_free_context(the_context);
    Serial.println(F("stop dtls server"));
}

void do_dtls_server (void)
{

#ifdef MSG_DEBUG
    print_free_mem();
#endif

    // on vérifie qu'on n'a pas reçu de message
    dtls_handle_read();
}

// DTLS client

// get_psk_info became useless, we already know the key

static dtls_handler_t cb_cli = {
    .write = send_to_peer,
    .read  = read_from_peer,
    .event = NULL,
    .get_psk_info = NULL,
};

void init_dtls_client (char line [])
{
    zigmsg.channel (channel) ;
    zigmsg.panid (PANID) ;
    zigmsg.addr2 (SENDADDR) ;
    zigmsg.promiscuous (false) ;
    zigmsg.start () ;

#ifdef MSG_DEBUG
    Serial.println(F("Start client")) ;
#endif

    memset(&session, 0, sizeof(session_t));
    session.addr = RECVADDR;
    session.size = sizeof(session.addr);

    randomSeed(get_the_time());
    dtls_init(get_the_time);

    the_context = dtls_new_context(get_random);
    the_context->say = say;
    the_context->smth_to_say = something_to_say;
    the_context->smth_to_hexdump = something_to_hexdump;

    memcpy(the_context->psk.key, PSK_DEFAULT_KEY, strlen(PSK_DEFAULT_KEY));
    the_context->psk.len = strlen(PSK_DEFAULT_KEY);

    if (!the_context) {
        while(1) { Serial.println(F("can't create ctxt")); delay(1000); }
    }

    dtls_set_handler(the_context, &cb_cli);
    dtls_connect(the_context, &session);
}

void stop_dtls_client (void)
{
    dtls_free_context(the_context);
    Serial.println(F("stop dtls client"));
}

void do_dtls_client (void)
{

#ifdef MSG_DEBUG
    print_free_mem();
#endif

    dtls_handle_read();
}

// GUI ;-)

void init_idle (char line [])
{
}

void stop_idle (void)
{
}

void do_idle (void)
{
}

struct gui
{
    char start_key ;			// lowercase
    const char *desc ;
    void (*f_init) (char line []) ;
    void (*f_stop) (void) ;
    void (*f_do) (void) ;
} ;

struct gui gui [] = {
    { 'i', "idle", init_idle, stop_idle, do_idle },
    { 'n', "raw sniffer", init_snif, stop_snif, do_snif },
    { 's', "sender", init_send, stop_send, do_send },
    { 'r', "receiver", init_recv, stop_recv, do_recv },
    { 'c', "channel (n)", init_chan, stop_chan, do_chan },
    { 'd', "dtls server", init_dtls_server, stop_dtls_server, do_dtls_server },
    { 't', "dtls client", init_dtls_client, stop_dtls_client, do_dtls_client },
} ;
#define	IDLE_MODE (& gui [0])

struct gui *parse_and_init_or_stop (char line [], struct gui *oldmode)
{
    char *p = line ;
    struct gui *newmode ;

    while (*p == ' ' || *p == '\t')
	p++ ;

    newmode = oldmode ;
    for (int i = 0 ; i < NTAB (gui) ; i++)
    {
	if (*p == gui [i].start_key)
	{
	    newmode = &gui [i] ;
	    break ;
	}
    }

    if (newmode != oldmode)
    {
	(* oldmode->f_stop) () ;
	(* newmode->f_init) (p + 1) ;
    }

    return newmode ;
}

void help (void)
{
    for (int i = 0 ; i < NTAB (gui) ; i++)
    {
	if (i > 0)
	    Serial.print (", ") ;
	Serial.print (gui [i].start_key) ;
	Serial.print (':') ;
	Serial.print (gui [i].desc) ;
    }
    Serial.println () ;
}

/******************************************************************************
Classic Arduino functions
*******************************************************************************/

void setup ()
{
    Serial.begin (38400);	// don't introduce spaces here
    Serial.println (F("Starting...")) ; // signal the start with a new line
    zigmsg.msgbufsize (MSGBUF_SIZE) ;	// space for 10 received messages
    help () ;
}

void loop()
{
    static char line [100], *p = line ;
    static struct gui *curmode = IDLE_MODE ;
    int n ;

    n = Serial.available () ;
    if (n > 0)
    {
	for (int i = 0 ; i < n ; i++)
	{
	    *p = Serial.read () ;
	    Serial.print (*p) ;
	    if (*p == '\r')
	    {
		struct gui *oldmode ;

		Serial.print ('\n') ;
		p = '\0' ;
		oldmode = curmode ;
		curmode = parse_and_init_or_stop (line, curmode) ;
		p = line ;
		if (curmode == oldmode)
		    help () ;
	    }
	    else p++ ;
	}
    }

    (*curmode->f_do) () ;
}

void errHandle(radio_error_t err)
{
    Serial.println();
    Serial.print(F("Error: "));
    Serial.print((uint8_t)err, 10);
    Serial.println();
}
