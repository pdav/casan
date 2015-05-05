#include <ZigMsg.h>
//#include <Test.h>

extern "C" {
#include <tinydtls.h>
#include <dtls.h>
//#include <test.h>
}

#define	CHANNEL	25		// use "c" to change it while running

#define	PANID		CONST16 (0xbe, 0xef)
#define	SENDADDR	CONST16 (0x12, 0x34)
#define	RECVADDR	CONST16 (0x45, 0x67)

#define	CURRENT_ADDRESS SENDADDR

#if CURRENT_ADDRESS == SENDADDR
#define	OTHER_ADDRESS RECVADDR
#else
#define	OTHER_ADDRESS SENDADDR
#endif

#define	MSGBUF_SIZE	100

// TODO FIXME DEBUG
#define MSG_DEBUG 1

#define PSK_DEFAULT_IDENTITY "Client_identity"
#define PSK_DEFAULT_KEY      "secretPSK"

#ifdef __GNUC__
#define UNUSED_PARAM __attribute__((unused))
#else
#define UNUSED_PARAM
#endif /* __GNUC__ */

#define	PERIODIC	100000

session_t dst;

// TODO : envoyer les messages d'erreur DTLS au lieu de simplement
// afficher une erreur et boucler

static char buf[200];
static size_t len = 0;

int channel = CHANNEL ;

/******************************************************************************
  Utilities
 *******************************************************************************/

#define	HEXCHAR(c)	((c) < 10 ? (c) + '0' : (c) - 10 + 'a')

void say (char * txt_to_say)
{
    Serial.print(txt_to_say);
    delay(1000);
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

void coap_decode (uint8_t msg [], int msglen)
{
    int version, type, toklen, code, id ;
    int c, dd ;
    int i ;
    int success, opt_nb ;
    int paylen ;

    version = COAP_VERSION (msg) ;
    type =    COAP_TYPE (msg) ;
    toklen =  COAP_TOKLEN (msg) ;
    code =    COAP_CODE (msg) ;
    id =      COAP_ID (msg) ;

    Serial.print ("  CASAN Vers=") ;
    Serial.print (version) ;

    Serial.print (", Type=") ;
    Serial.print (type_string [type]) ;

    Serial.print (", Code=") ;
    c = (code >> 5) & 0x7 ;
    dd = code & 0x1f ;
    Serial.print (c) ;
    Serial.print ('.') ;
    if (dd < 10)
        Serial.print ('0') ;
    Serial.print (dd) ;
    if (c == 0 && dd < (int) NTAB (code_string))
    {
        Serial.print ('(') ;
        Serial.print (code_string [dd]) ;
        Serial.print (')') ;
    }

    Serial.print (", Id=") ;
    Serial.print (id) ;
    Serial.println () ;

    i = 4 ;

    if (toklen > 0)
    {
        Serial.print ("    Token=") ;
        phexascii (msg + i, toklen, 8) ;
        Serial.println () ;
        i += toklen ;
    }

    /*
     * Options analysis
     */

    opt_nb = 0 ;
    success = 1 ;

    while (success && i < msglen && msg [i] != 0xff)
    {
        int opt_delta, opt_len ;

        opt_delta = (msg [i] >> 4) & 0x0f ;
        opt_len   = (msg [i]     ) & 0x0f ;
        i++ ;
        switch (opt_delta)
        {
            case 13 :
                opt_delta = msg [i] + 13 ;
                i += 1 ;
                break ;
            case 14 :
                opt_delta = (msg [i] << 8) + msg [i+1] + 269 ;
                i += 2 ;
                break ;
            case 15 :
                success = 0 ;			// recv failed
                break ;
        }
        opt_nb += opt_delta ;

        switch (opt_len)
        {
            case 13 :
                opt_len = msg [i] + 13 ;
                i += 1 ;
                break ;
            case 14 :
                opt_len = (msg [i] << 8) + msg [i+1] + 269 ;
                i += 2 ;
                break ;
            case 15 :
                success = 0 ;			// recv failed
                break ;
        }

        /* option found */
        if (success)
        {
            Serial.print ("    opt ") ;
            pdec (opt_nb, 3, ' ') ;
            Serial.print (" (len=") ;
            pdec (opt_len, 2, ' ') ;
            Serial.print (")  ") ;
            phexascii (msg + i, opt_len, 14) ;
            Serial.println () ;
            i += opt_len ;
        }
        else Serial.print ("    option unrecognized\n") ;
    }

    paylen = msglen - i - 1 ;
    if (success && paylen > 0)
    {
        if (msg [i] != 0xff)
        {
            Serial.println ("  no FF marker") ;
            success = 0 ;
        }
        else
        {
            Serial.print ("  Payload (len=") ;
            Serial.print (paylen) ;
            Serial.println (")") ;

            i++ ;
            while (i < msglen)
            {
                Serial.print ("      ") ;
                phexascii (msg + i, paylen > 16 ? 16 : paylen, 16) ;
                Serial.println () ;
                paylen -= 16 ;
                i += 16 ;
            }
        }
    }
}

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
    p = padr (dstmode, dstp, 1, 0) ;	   // do not print dst addr
    p = padr (srcmode, p, ! intrapan, 1);  // p points past addresses
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

    if (casan_decode && Z_GET_FRAMETYPE (z->fcf) == Z_FT_DATA && lastseq != seq)
        coap_decode (p, z->paylen) ;
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
    Serial.println ("Start snif") ;
}

void stop_snif (void)
{
    Serial.println ("Stop snif") ;
}

void do_snif (void)
{
    snif (false) ;
}

/******************************************************************************
  Sender
 *******************************************************************************/

/*
void do_send (void)
{
    static int n = 0 ;

    if (++n % PERIODIC == 0)
    {
        print_stat ();
        n = 0 ;

        uint32_t time ;
        time = millis () ;
        if (zigmsg.sendto(RECVADDR, 4, (uint8_t *) &time))
            Serial.println ("Sent") ;
        else
            Serial.println ("Sent error") ;
    }
}
*/


// DTLS TODO


/******************************************************************************
  DTLS server
 *******************************************************************************/

/* This function is the "key store" for tinyDTLS. It is called to
 * retrieve a key for the given identity within this particular
 * session. */
static int
get_psk_info(struct dtls_context_t *ctx, const session_t *session,
        dtls_credentials_type_t type,
        const unsigned char *id, size_t id_len,
        unsigned char *result, size_t result_length)
{

    struct keymap_t {
        unsigned char *id;
        size_t id_length;
        unsigned char *key;
        size_t key_length;
    } psk[3] = {
        { (unsigned char *)"Client_identity", 15,
            (unsigned char *)"secretPSK", 9 },
        { (unsigned char *)"default identity", 16,
            (unsigned char *)"\x11\x22\x33", 3 },
        { (unsigned char *)"\0", 2,
            (unsigned char *)"", 1 }
    };

    if (type != DTLS_PSK_KEY) {
        return 0;
    }

    if (id) {
        unsigned int i;
        for (i = 0; i < sizeof(psk)/sizeof(struct keymap_t); i++) {
            if (id_len == psk[i].id_length && 
                    memcmp(id, psk[i].id, id_len) == 0) {
                if (result_length < psk[i].key_length) {
                    //dtls_warn("buffer too small for PSK");
                    Serial.println("buf 2 small 4 PSK");
                    return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
                }

                memcpy(result, psk[i].key, psk[i].key_length);
                return psk[i].key_length;
            }
        }
    }

    return dtls_alert_fatal_create(DTLS_ALERT_DECRYPT_ERROR);
}

#define DTLS_SERVER_CMD_CLOSE "server:close"
#define DTLS_SERVER_CMD_RENEGOTIATE "server:renegotiate"

dtls_context_t *the_context = NULL;

static int
read_from_peer_server(struct dtls_context_t *ctx, 
        session_t *session, uint8 *data, size_t len)
{

    ZigMsg::ZigReceivedFrame *z ;

#ifdef MSG_DEBUG
    Serial.println("read_from_peer_serv");
#endif

    while ((z = zigmsg.get_received ()) != NULL)
    {
        Serial.println("msg recv");
        //print_frame (z, casan_decode) ;
        zigmsg.skip_received () ;
    }

    size_t i;
    for (i = 0; i < len; i++)
        Serial.print(data[i]);

    if (len >= strlen(DTLS_SERVER_CMD_CLOSE) &&
            !memcmp(data
                , DTLS_SERVER_CMD_CLOSE
                , strlen(DTLS_SERVER_CMD_CLOSE))) {
        Serial.println("serv: clos co");
        dtls_close(ctx, session);
        return len;

    } else if (len >= strlen(DTLS_SERVER_CMD_RENEGOTIATE) &&
            !memcmp(data, DTLS_SERVER_CMD_RENEGOTIATE, strlen(DTLS_SERVER_CMD_RENEGOTIATE))) {
        Serial.println("serv: reneg co");
        dtls_renegotiate(ctx, session);
        return len;
    }

    return dtls_write(ctx, session, data, len);
}

// TODO : faire en sorte d'envoyer des messages à session->addr qui 
// indiquera l'adresse

static int
send_to_peer_server(struct dtls_context_t *ctx, 
        session_t *session, uint8 *data, size_t len)
{

    //return zigmsg.sendto(SENDADDR, len, data);
    return zigmsg.sendto(session->addr, len, data);
}

static int
dtls_handle_read(void)
{
    int *fd;
    session_t session;
    static uint8 buf[DTLS_MAX_BUF];
    ZigMsg::ZigReceivedFrame *z ;

#ifdef MSG_DEBUG
    Serial.println("d_hdl_read");
#endif

    fd = (int *) dtls_get_app_data(the_context);

    assert(fd);

    memset(&session, 0, sizeof(session_t));
    session.size = sizeof(session.addr);
    session.addr = CURRENT_ADDRESS;

    while ((z = zigmsg.get_received ()) != NULL)
    {
        Serial.println("recv");
        print_frame (z, false) ;
        zigmsg.skip_received () ;
    }

    // TODO vérifier que le paquet est arrivé en bon état

    int len = 0;

    // TODO récupérer le buffer avec le contenu du paquet
    int ret = dtls_handle_message(the_context, &session, buf, len);
    if(ret) {
        Serial.print("err d_hdl_msg > d_h_read : ");
        Serial.println(ret);
    }
    return ret;
}

static dtls_handler_t cb_server = {
    .write = send_to_peer_server,
    .read  = read_from_peer_server,
    .event = NULL,
    .get_psk_info = get_psk_info,
};

void init_dtls_server (char line [])
{
    zigmsg.channel (channel) ;
    zigmsg.panid (PANID) ;
    zigmsg.addr2 (RECVADDR) ;
    zigmsg.promiscuous (false) ;
    zigmsg.start () ;

#ifdef MSG_DEBUG
    Serial.println("init_d_serv");
#endif

    //session_t dst;
    //memset(&dst, 0, sizeof(session_t));
    //dst.addr = SENDADDR;
    //dst.size = 2;

    //log_t log_level = DTLS_LOG_WARN;
    //dtls_set_log_level(log_level);

    int fd;

    dtls_init();

    the_context = dtls_new_context(&fd);

    the_context->say = say;

    dtls_set_handler(the_context, &cb_server);
}

void stop_dtls_server (void)
{
    Serial.println("Stop D serv") ;
}

void do_dtls_server (void)
{
    ZigMsg::ZigReceivedFrame *z ;

#ifdef MSG_DEBUG
    Serial.println("do_dtls_server");
#endif

    while ((z = zigmsg.get_received ()) != NULL)
    {
        Serial.println("msg recv");

        // à chaque réception de message
        dtls_handle_read();

        //print_frame (z, casan_decode) ;
        zigmsg.skip_received () ;
    }

    // if error:
    //dtls_free_context(the_context);
    // then exit
}

/******************************************************************************
  DTLS client
 *******************************************************************************/

static dtls_context_t *d_ctxt_cli = NULL;

/* The PSK information for DTLS */
#define PSK_ID_MAXLEN 256
#define PSK_MAXLEN 256
static unsigned char psk_id[PSK_ID_MAXLEN];
static size_t psk_id_length = 0;
static unsigned char psk_key[PSK_MAXLEN];
static size_t psk_key_length = 0;

/* This function is the "key store" for tinyDTLS. It is called to
 * retrieve a key for the given identity within this particular
 * session. */
static int
get_psk_info_cli(struct dtls_context_t *ctx UNUSED_PARAM,
        const session_t *session UNUSED_PARAM,
        dtls_credentials_type_t type,
        const unsigned char *id, size_t id_len,
        unsigned char *result, size_t result_length)
{

    switch (type) {
        case DTLS_PSK_IDENTITY:
            if (id_len) {
                //dtls_debug("got psk_identity_hint: '%.*s'\n", id_len, id);
                Serial.print("got psk_id_hint: ");
                Serial.println(*id);
            }

            if (result_length < psk_id_length) {
                //dtls_warn("cannot set psk_identity -- buffer too small\n");
                //return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
                while(1) {
                    Serial.println("cant set psk_id -- buf 2 small");
                    delay(1000);
                }
            }

            memcpy(result, psk_id, psk_id_length);
            return psk_id_length;
        case DTLS_PSK_KEY:
            if (id_len != psk_id_length || memcmp(psk_id, id, id_len) != 0) {
                //dtls_warn("PSK for unknown id requested, exiting\n");
                //return dtls_alert_fatal_create(DTLS_ALERT_ILLEGAL_PARAMETER);

                while(1) {
                    Serial.println("PSK 4 unknown id req, exit");
                    delay(1000);
                }

                break;
            } else if (result_length < psk_key_length) {
                // dtls_warn("cannot set psk -- buffer too small\n");
                Serial.println("cant set psk -- buf 2 small");
                return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
            }

            memcpy(result, psk_key, psk_key_length);
            return psk_key_length;
        default:
            Serial.print("unsup req type: ");
            Serial.println(type);
    }

    return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
}

static void
try_send(struct dtls_context_t *ctx)
{
    int res;
    res = dtls_write(ctx, &dst, (uint8 *)buf, len);
    if (res >= 0) {
        memmove(buf, buf + res, len - res);
        len -= res;
    }
}

static int
read_from_peer(struct dtls_context_t *ctx, 
        session_t *session, uint8 *data, size_t len)
{
    ZigMsg::ZigReceivedFrame *z ;

#ifdef MSG_DEBUG
    Serial.println("read_from_peer");
#endif

    while ((z = zigmsg.get_received ()) != NULL)
    {
        Serial.println("msg recv");
        //print_frame (z, casan_decode) ;
        print_frame (z, false) ;
        zigmsg.skip_received ();
        delay(500);
    }

    size_t i;
    for (i = 0; i < len; i++)
        printf("%c", data[i]);
    return 0;
}

// TODO envoi message
// TODO merge avec send_to_peer_server

static int
send_to_peer(struct dtls_context_t *ctx, 
        session_t *session, uint8 *data, size_t len)
{

#ifdef MSG_DEBUG
    Serial.println("send2peer") ;
#endif

    //int fd = *(int *)dtls_get_app_data(ctx);
    //return sendto(fd, data, len, MSG_DONTWAIT,
    //        &session->addr.sa, session->size);

    delay(500);
    return zigmsg.sendto(session->addr, len, data);
}

static int
dtls_handle_read(struct dtls_context_t *ctx)
{

#ifdef MSG_DEBUG
    Serial.println("d_hdl_read") ;
#endif

    int fd;
    session_t session;
#define MAX_READ_BUF 2000
    static uint8 buf[MAX_READ_BUF];
    int len;

    fd = *(int *)dtls_get_app_data(ctx);

    if (!fd)
        return -1;

    memset(&session, 0, sizeof(session_t));
    session.size = sizeof(session.addr);
    //len = recvfrom(fd, buf, MAX_READ_BUF, 0, 
    //        &session.addr.sa, &session.size);

    ZigMsg::ZigReceivedFrame *z ;
    while ((z = zigmsg.get_received ()) != NULL)
    {
        //print_frame (z, casan_decode) ;

        len = z->paylen;

        if (len < 0) {
            //perror("recvfrom");
            print_frame (z, false) ;
            Serial.println("err: recv, len < 0");
            return -1;
        } else {
            //dtls_dsrv_log_addr(DTLS_LOG_DEBUG, "peer", &session);
            //dtls_debug_dump("bytes from peer", buf, len);
            Serial.print("recv : TODO");
            print_frame (z, false) ;
            //Serial.println(buf, HEX);
        }

        zigmsg.skip_received () ;
    }

    return dtls_handle_message(ctx, &session, buf, len);
}    

/*---------------------------------------------------------------------------*/

static dtls_handler_t cb = {
    .write = send_to_peer,
    .read  = read_from_peer,
    .event = NULL,
    .get_psk_info = get_psk_info_cli,
};

#define DTLS_CLIENT_CMD_CLOSE "client:close"
#define DTLS_CLIENT_CMD_RENEGOTIATE "client:renegotiate"

void init_dtls_client (char line [])
{
    zigmsg.channel (channel) ;
    zigmsg.panid (PANID) ;
    zigmsg.addr2 (SENDADDR) ;
    zigmsg.promiscuous (false) ;
    zigmsg.start () ;

#ifdef MSG_DEBUG
    Serial.println("Start cli") ;
#endif

    int fd;
    memset(&dst, 0, sizeof(session_t));
    dst.addr = RECVADDR;
    dst.size = 2;

    //log_t log_level = DTLS_LOG_WARN;
    //dtls_set_log_level(log_level);

    dtls_init();

    // PSK IDENTITY & KEY
    psk_id_length = strlen(PSK_DEFAULT_IDENTITY);
    psk_key_length = strlen(PSK_DEFAULT_KEY);
    memcpy(psk_id, PSK_DEFAULT_IDENTITY, psk_id_length);
    memcpy(psk_key, PSK_DEFAULT_KEY, psk_key_length);

    d_ctxt_cli = dtls_new_context(&fd);
    d_ctxt_cli->say = say;

    if (!d_ctxt_cli) {
        //dtls_emerg("cannot create context\n");
        while(1) { Serial.println("cant create ctxt"); delay(1000); }
    }

    dtls_set_handler(d_ctxt_cli, &cb);

    dtls_connect(d_ctxt_cli, &dst);
}

void stop_dtls_client (void)
{

    dtls_free_context(d_ctxt_cli);

#ifdef MSG_DEBUG
    Serial.println("stop_d_c");
#endif
}

void do_dtls_client (void)
{

#ifdef MSG_DEBUG
    Serial.println("do_d_client");
#endif

    dtls_handle_read(d_ctxt_cli);

    // TODO
    if (len >= strlen(DTLS_CLIENT_CMD_CLOSE) &&
            !memcmp(buf, DTLS_CLIENT_CMD_CLOSE
                , strlen(DTLS_CLIENT_CMD_CLOSE)))
    {
        Serial.println("cli: clos co");
        dtls_close(d_ctxt_cli, &dst);
        len = 0;
    } 
    else if (len >= strlen(DTLS_CLIENT_CMD_RENEGOTIATE) &&
            !memcmp(buf, DTLS_CLIENT_CMD_RENEGOTIATE
                , strlen(DTLS_CLIENT_CMD_RENEGOTIATE))) 
    {

        Serial.println("cli: reneg co");
        dtls_renegotiate(d_ctxt_cli, &dst);
        len = 0;
    } else {
        try_send(d_ctxt_cli);
    }

    //    struct mysocket msock;
    //    msock.cb_send = cb_send;
    //    msock.cb_recv = cb_recv;
    //    send_smth(msock);
}

////////////////////////////////////////////////////////////////////
// gestion des signaux, non dispo sous zigduino
//static void dtls_handle_signal(int sig)
//{
//    dtls_free_context(d_ctxt_cli);
//    signal(sig, SIG_DFL);
//    kill(getpid(), sig);
//}
//
/* stolen from libcoap: */
//static int
//resolve_address(const char *server, struct sockaddr *dst) {
//
//    struct addrinfo *res, *ainfo;
//    struct addrinfo hints;
//    static char addrstr[256];
//    int error;
//
//    memset(addrstr, 0, sizeof(addrstr));
//    if (server && strlen(server) > 0)
//        memcpy(addrstr, server, strlen(server));
//    else
//        memcpy(addrstr, "localhost", 9);
//
//    memset ((char *)&hints, 0, sizeof(hints));
//    hints.ai_socktype = SOCK_DGRAM;
//    hints.ai_family = AF_UNSPEC;
//
//    error = getaddrinfo(addrstr, "", &hints, &res);
//
//    if (error != 0) {
//        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
//        return error;
//    }
//
//    for (ainfo = res; ainfo != NULL; ainfo = ainfo->ai_next) {
//
//        switch (ainfo->ai_family) {
//            case AF_INET6:
//            case AF_INET:
//
//                memcpy(dst, ainfo->ai_addr, ainfo->ai_addrlen);
//                return ainfo->ai_addrlen;
//            default:
//                ;
//        }
//    }
//
//    freeaddrinfo(res);
//    return -1;
//}
////////////////////////////////////////////////////////////////////

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
        Serial.println ("Invalid chan") ;
    else
    {
        channel = ch ;
        Serial.print ("Chan set to ") ;
        Serial.println (channel) ;
    }

    Serial.println ("Enter idle") ;
}

void stop_chan (void)
{
}

void do_chan (void)
{
}

/******************************************************************************
  GUI ;-)
 *******************************************************************************/

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
    { 'c', "channel (n)", init_chan, stop_chan, do_chan },
    { 'S', "dtls server", init_dtls_server, stop_dtls_server, do_dtls_server },
    { 'C', "dtls client", init_dtls_client, stop_dtls_client, do_dtls_client },
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

void errHandle(radio_error_t err)
{
    Serial.println();
    Serial.print("Error: ");
    Serial.print((uint8_t)err, 10);
    Serial.println();
}

/******************************************************************************
  Classic Arduino functions
 *******************************************************************************/

void setup ()
{
    Serial.begin (38400);	// don't introduce spaces here
    Serial.println ("Starting...") ; // signal the start with a new line
    zigmsg.msgbufsize (MSGBUF_SIZE) ;	// space for 100 received messages
    help () ;
}

void loop()
{
    static char line [100], *p = line ;
    static struct gui *curmode = IDLE_MODE ;
    int n ;

    //size_t x = 50;
    //get_value(x);

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



/* code de test
//size_t cb_send (void *data, size_t len)
//{
//    Serial.println((char*)data);
//    //ZigMsg * zm = &zigmsg;
//
//    //if (zm->sendto (RECVADDR, len, (uint8_t *) data))
//    if (zigmsg.sendto (RECVADDR, len, (uint8_t *) data))
//        Serial.println ("Sent") ;
//    else
//        Serial.println ("Sent error") ;
//
//    return 0;
//}
//
//size_t cb_recv (void *data, size_t * len)
//{
//    Serial.println("On passe par là");
//    ZigMsg::ZigReceivedFrame *z ;
//
//    while ((z = zigmsg.get_received ()) != NULL)
//    {
//        print_frame (z, false) ;
//        zigmsg.skip_received () ;
//    }
//
//    return 0;
//}
 */
