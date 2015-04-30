#include <ZigMsg.h>

#define	CHANNEL	20	// sed & make

#define	PANID		CONST16 (0xbe, 0xef)
#define	SENDADDR	CONST16 (0x12, 0x34)
#define	RECVADDR	CONST16 (0x45, 0x67)

#define	MSGBUF_SIZE	100


#define	PERIODIC	100000

int channel = CHANNEL ;

/******************************************************************************
  Utilities
 *******************************************************************************/

#define	HEXCHAR(c)	((c) < 10 ? (c) + '0' : (c) - 10 + 'a')

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
        else printf ("    option unrecognized\n") ;
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
            Serial.print ("    Payload (len=") ;
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
    Serial.println ("Starting sniffer") ;
}

void stop_snif (void)
{
    Serial.println ("Stopping sniffer") ;
}

void do_snif (void)
{
    snif (false) ;
}

/******************************************************************************
  CoAP sniffer
 *******************************************************************************/

void init_casan (char line [])
{
    zigmsg.channel (channel) ;
    zigmsg.promiscuous (true) ;
    zigmsg.start () ;
    Serial.println ("Starting CASAN sniffer") ;
}

void stop_casan (void)
{
    Serial.println ("Stopping CASAN sniffer") ;
}

void do_casan (void)
{
    snif (true) ;
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
    Serial.println("Stopping send") ;
}

void do_send (void)
{
    static int n = 0 ;

    if (++n % PERIODIC == 0)
    {
        print_stat ();
        n = 0 ;

        uint32_t time ;
        time = millis () ;
        if (zigmsg.sendto (RECVADDR, 4, (uint8_t *) &time))
            Serial.println ("Sent") ;
        else
            Serial.println ("Sent error") ;
    }
}

/******************************************************************************
  DTLS server
 *******************************************************************************/

void init_dtls_server (char line [])
{
    zigmsg.channel (channel) ;
    zigmsg.panid (PANID) ;
    zigmsg.addr2 (RECVADDR) ;
    zigmsg.promiscuous (false) ;
    zigmsg.start () ;
    Serial.println("Starting DTLS server") ;
#ifdef __DTLS__
    Serial.println("DTLS ROCK & ROLL") ;
#endif

}

void stop_dtls_server (void)
{
    Serial.println("Stopping DTLS server") ;
}

void do_dtls_server (void)
{
    // snif
    static int n = 0 ;
    ZigMsg::ZigReceivedFrame *z ;

    if (++n % PERIODIC == 0)
    {
        print_stat () ;
        n = 0 ;
    }

    while ((z = zigmsg.get_received ()) != NULL)
    {
        print_frame (z, false) ;
        zigmsg.skip_received () ;
    }
}

/******************************************************************************
  DTLS client
 *******************************************************************************/

void init_dtls_client (char line [])
{
    zigmsg.channel (channel) ;
    zigmsg.panid (PANID) ;
    zigmsg.addr2 (SENDADDR) ;
    zigmsg.promiscuous (false) ;
    zigmsg.start () ;
    Serial.println("Starting DTLS client") ;
}

void stop_dtls_client (void)
{
    Serial.println("Stopping DTLS client") ;
}

void do_dtls_client (void)
{
    static int n = 0 ;

    if (++n % PERIODIC == 0)
    {
        print_stat ();
        n = 0 ;

        uint32_t time ;
        time = millis () ;
        if (zigmsg.sendto (RECVADDR, 4, (uint8_t *) &time))
            Serial.println ("Sent") ;
        else
            Serial.println ("Sent error") ;
    }
}


/******************************************************************************
  Ping pong ping
 *******************************************************************************/

void init_pingpong_ping (char line [])
{
    zigmsg.channel (channel) ;
    zigmsg.panid (PANID) ;
    zigmsg.addr2 (SENDADDR) ;
    zigmsg.promiscuous (false) ;
    zigmsg.start () ;
    Serial.println("Starting PING PONG") ;
}

void stop_pingpong_ping (void)
{
    Serial.println("Stopping PING PONG PING") ;
}

void do_pingpong_ping (void)
{
    ZigMsg::ZigReceivedFrame *z ;

    static int n = 0 ;

    if (++n % PERIODIC == 0)
    {
        print_stat ();
        n = 0;
    }

    bool found = false;
    // send
    uint32_t time ;
    uint32_t duration;

    time = millis () ;

    Serial.println();
    Serial.print(time, HEX);
    Serial.print(' ');

    if (zigmsg.sendto (RECVADDR, 4, (uint8_t *) &time))
    {
        //Serial.println ("Sent") ;
    }
    else
        Serial.println ("Sent error") ;

    while(! found && ++n % PERIODIC != 0)
    {
        // recv
        while ((z = zigmsg.get_received ()) != NULL)
        {
            //print_frame (z, false) ;
            zigmsg.skip_received () ;
            found = true;
        }

    }

    duration = millis () - time ;

    Serial.print("duration: ");
    Serial.println(duration);
    delay(500);

    if (n % PERIODIC == 0)
        n = 0;
}

/******************************************************************************
  Ping pong pong
 *******************************************************************************/

uint32_t get_time ( ZigMsg::ZigReceivedFrame *z) 
{
    //time = (uint32_t) (z->rawframe + z->rawlen - 4);

    union uint32_to_uint8 {
        uint32_t x;
        uint8_t  y[4];
    } myunion;

    myunion.y[0] = z->payload[z->paylen-4];
    myunion.y[1] = z->payload[z->paylen-3];
    myunion.y[2] = z->payload[z->paylen-2];
    myunion.y[3] = z->payload[z->paylen-1];

    Serial.println();
    Serial.print("payload len : ");
    Serial.println(z->paylen);

    Serial.print(myunion.y[0], HEX);
    Serial.print(myunion.y[1], HEX);
    Serial.print(myunion.y[2], HEX);
    Serial.print(myunion.y[3], HEX);
    Serial.print(' ');

    return myunion.x;

}

void init_pingpong_pong (char line [])
{
    zigmsg.channel (channel) ;
    zigmsg.panid (PANID) ;
    zigmsg.addr2 (RECVADDR) ;
    zigmsg.promiscuous (false) ;
    zigmsg.start () ;
    Serial.println("Starting PING PONG PONG") ;
}

void stop_pingpong_pong (void)
{
    Serial.println("Stopping PING PONG PONG") ;
}

void do_pingpong_pong (void)
{
    ZigMsg::ZigReceivedFrame *z ;

    uint32_t time = 0;
    bool found = false;
    while(! found)
    {
        // recv
        while ((z = zigmsg.get_received ()) != NULL)
        {
            //print_frame (z, false) ;
            zigmsg.skip_received () ;
            found = true;
        }
    }

    if (zigmsg.sendto (SENDADDR, 0, (uint8_t *) &time))
        Serial.println ("Sent") ;
    else
        Serial.println ("Sent error") ;
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
    Serial.println("Starting receiver") ;
}

void stop_recv (void)
{
    Serial.println("Stopping receiver") ;
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
        Serial.println ("Invalid channel") ;
    else
    {
        channel = ch ;
        Serial.print ("Channel set to ") ;
        Serial.println (channel) ;
    }

    Serial.println ("Entering idle mode") ;
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
    { 'c', "channel (n)", init_chan, stop_chan, do_chan },
    { 'p', "ping pong ping", init_pingpong_ping, stop_pingpong_ping, do_pingpong_ping },
    { 'P', "ping pong pong", init_pingpong_pong, stop_pingpong_pong, do_pingpong_pong },
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
    Serial.println ("Starting...") ; // signal the start with a new line
    zigmsg.msgbufsize (MSGBUF_SIZE) ;	// space for 100 received messages
    help () ;
}

void loop()
{
    static char line [100], *p = line ;
    static struct gui *curmode = IDLE_MODE ;
    int n ;

    p[0] = 'p';
    p[1] = '\r';

    for (int i = 0 ; i < 2 ; i++)
    {
        if (*p == '\r')
        {
            struct gui *oldmode ;

            p = '\0' ;
            oldmode = curmode ;
            curmode = parse_and_init_or_stop (line, curmode) ;
            p = line ;
            if (curmode == oldmode)
                help () ;
        }
        else p++ ;
    }

    (*curmode->f_do) () ;
}

void errHandle(radio_error_t err)
{
    Serial.println();
    Serial.print("Error: ");
    Serial.print((uint8_t)err, 10);
    Serial.println();
}
