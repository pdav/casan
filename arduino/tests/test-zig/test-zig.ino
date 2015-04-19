#include <ZigMsg.h>
//#include <Test.h>
#include <tinydtls.h>
#include <dtls.h>

#define	CHANNEL	25		// use "c" to change it while running

#define	PANID		CONST16 (0xbe, 0xef)
#define	SENDADDR	CONST16 (0x12, 0x34)
#define	RECVADDR	CONST16 (0x45, 0x67)

#define	MSGBUF_SIZE	100


#define	PERIODIC	100000

int channel = CHANNEL ;

/* This function is the "key store" for tinyDTLS. It is called to
 * retrieve a key for the given identity within this particular
 * session. */
static int
get_psk_info(struct dtls_context_t *ctx, const session_t *session,
        dtls_credentials_type_t type,
        const unsigned char *id, size_t id_len,
        unsigned char *result, size_t result_length) {

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
        int i;
        for (i = 0; i < sizeof(psk)/sizeof(struct keymap_t); i++) {
            if (id_len == psk[i].id_length && 
                    memcmp(id, psk[i].id, id_len) == 0) {
                if (result_length < psk[i].key_length) {
                    dtls_warn("buffer too small for PSK");
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

static int
read_from_peer(struct dtls_context_t *ctx, 
        session_t *session, uint8 *data, size_t len) {
    size_t i;
    for (i = 0; i < len; i++)
        printf("%c", data[i]);
    if (len >= strlen(DTLS_SERVER_CMD_CLOSE) &&
            !memcmp(data, DTLS_SERVER_CMD_CLOSE, strlen(DTLS_SERVER_CMD_CLOSE))) {
        printf("server: closing connection\n");
        dtls_close(ctx, session);
        return len;
    } else if (len >= strlen(DTLS_SERVER_CMD_RENEGOTIATE) &&
            !memcmp(data, DTLS_SERVER_CMD_RENEGOTIATE, strlen(DTLS_SERVER_CMD_RENEGOTIATE))) {
        printf("server: renegotiate connection\n");
        dtls_renegotiate(ctx, session);
        return len;
    }

    return dtls_write(ctx, session, data, len);
}

static int
send_to_peer(struct dtls_context_t *ctx, 
        session_t *session, uint8 *data, size_t len) {

    int fd = *(int *)dtls_get_app_data(ctx);
    return sendto(fd, data, len, MSG_DONTWAIT,
            &session->addr.sa, session->size);
}

static int
dtls_handle_read(struct dtls_context_t *ctx) {
    int *fd;
    session_t session;
    static uint8 buf[DTLS_MAX_BUF];
    int len;

    fd = dtls_get_app_data(ctx);

    assert(fd);

    memset(&session, 0, sizeof(session_t));
    session.size = sizeof(session.addr);
    len = recvfrom(*fd, buf, sizeof(buf), MSG_TRUNC,
            &session.addr.sa, &session.size);

    if (len < 0) {
        perror("recvfrom");
        return -1;
    } else {
        dtls_debug("got %d bytes from port %d\n", len, 
                ntohs(session.addr.sin6.sin6_port));
        if (sizeof(buf) < len) {
            dtls_warn("packet was truncated (%d bytes lost)\n", len - sizeof(buf));
        }
    }

    return dtls_handle_message(ctx, &session, buf, len);
}    

static int
resolve_address(const char *server, struct sockaddr *dst) {

    struct addrinfo *res, *ainfo;
    struct addrinfo hints;
    static char addrstr[256];
    int error;

    memset(addrstr, 0, sizeof(addrstr));
    if (server && strlen(server) > 0)
        memcpy(addrstr, server, strlen(server));
    else
        memcpy(addrstr, "localhost", 9);

    memset ((char *)&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_UNSPEC;

    error = getaddrinfo(addrstr, "", &hints, &res);

    if (error != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return error;
    }

    for (ainfo = res; ainfo != NULL; ainfo = ainfo->ai_next) {

        switch (ainfo->ai_family) {
            case AF_INET6:

                memcpy(dst, ainfo->ai_addr, ainfo->ai_addrlen);
                return ainfo->ai_addrlen;
            default:
                ;
        }
    }

    freeaddrinfo(res);
    return -1;
}

static void
usage(const char *program, const char *version) {
    const char *p;

    p = strrchr( program, '/' );
    if ( p )
        program = ++p;

    fprintf(stderr, "%s v%s -- DTLS server implementation\n"
            "(c) 2011-2014 Olaf Bergmann <bergmann@tzi.org>\n\n"
            "usage: %s [-A address] [-p port] [-v num]\n"
            "\t-A address\t\tlisten on specified address (default is ::)\n"
            "\t-p port\t\tlisten on specified port (default is %d)\n"
            "\t-v num\t\tverbosity level (default: 3)\n",
            program, version, program, DEFAULT_PORT);
}

static dtls_handler_t cb = {
    .write = send_to_peer,
    .read  = read_from_peer,
    .event = NULL,
    .get_psk_info = get_psk_info,
};

int 
main(int argc, char **argv) {
    dtls_context_t *the_context = NULL;
    log_t log_level = DTLS_LOG_WARN;
    fd_set rfds, wfds;
    struct timeval timeout;
    int fd, opt, result;
    int on = 1;
    struct sockaddr_in6 listen_addr;

    memset(&listen_addr, 0, sizeof(struct sockaddr_in6));

    /* fill extra field for 4.4BSD-based systems (see RFC 3493, section 3.4) */
#if defined(SIN6_LEN) || defined(HAVE_SOCKADDR_IN6_SIN6_LEN)
    listen_addr.sin6_len = sizeof(struct sockaddr_in6);
#endif

    listen_addr.sin6_family = AF_INET6;
    listen_addr.sin6_port = htons(DEFAULT_PORT);
    listen_addr.sin6_addr = in6addr_any;

    while ((opt = getopt(argc, argv, "A:p:v:")) != -1) {
        switch (opt) {
            case 'A' :
                if (resolve_address(optarg, (struct sockaddr *)&listen_addr) < 0) {
                    fprintf(stderr, "cannot resolve address\n");
                    exit(-1);
                }
                break;
            case 'p' :
                listen_addr.sin6_port = htons(atoi(optarg));
                break;
            case 'v' :
                log_level = strtol(optarg, NULL, 10);
                break;
            default:
                usage(argv[0], dtls_package_version());
                exit(1);
        }
    }

    dtls_set_log_level(log_level);

    /* init socket and set it to non-blocking */
    fd = socket(listen_addr.sin6_family, SOCK_DGRAM, 0);

    if (fd < 0) {
        dtls_alert("socket: %s\n", strerror(errno));
        return 0;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) < 0) {
        dtls_alert("setsockopt SO_REUSEADDR: %s\n", strerror(errno));
    }

#if 0
    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        dtls_alert("fcntl: %s\n", strerror(errno));
        goto error;
    }
#endif
    on = 1;
#ifdef IPV6_RECVPKTINFO
    if (setsockopt(fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on) ) < 0) {
#else /* IPV6_RECVPKTINFO */
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_PKTINFO, &on, sizeof(on) ) < 0) {
#endif /* IPV6_RECVPKTINFO */
            dtls_alert("setsockopt IPV6_PKTINFO: %s\n", strerror(errno));
        }

        if (bind(fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
            dtls_alert("bind: %s\n", strerror(errno));
            goto error;
        }

        dtls_init();

        the_context = dtls_new_context(&fd);

        dtls_set_handler(the_context, &cb);

        while (1) {
            FD_ZERO(&rfds);
            FD_ZERO(&wfds);

            FD_SET(fd, &rfds);
            /* FD_SET(fd, &wfds); */

            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            result = select( fd+1, &rfds, &wfds, 0, &timeout);

            if (result < 0) {		/* error */
                if (errno != EINTR)
                    perror("select");
            } else if (result == 0) {	/* timeout */
            } else {			/* ok */
                if (FD_ISSET(fd, &wfds))
                    ;
                else if (FD_ISSET(fd, &rfds)) {
                    dtls_handle_read(the_context);
                }
            }
        }

error:
        dtls_free_context(the_context);
        exit(0);
    }

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

    size_t cb_send (void *data, size_t len)
    {
        Serial.println((char*)data);
        //ZigMsg * zm = &zigmsg;

        //if (zm->sendto (RECVADDR, len, (uint8_t *) data))
        if (zigmsg.sendto (RECVADDR, len, (uint8_t *) data))
            Serial.println ("Sent") ;
        else
            Serial.println ("Sent error") ;

        return 0;
    }

    size_t cb_recv (void *data, size_t * len)
    {
        Serial.println("On passe par là");
        ZigMsg::ZigReceivedFrame *z ;

        while ((z = zigmsg.get_received ()) != NULL)
        {
            print_frame (z, false) ;
            zigmsg.skip_received () ;
        }

        return 0;
    }

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
        struct mysocket msock;
        msock.cb_send = cb_send;
        msock.cb_recv = cb_recv;
        recv_smth(msock);
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
        struct mysocket msock;
        msock.cb_send = cb_send;
        msock.cb_recv = cb_recv;
        send_smth(msock);
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
        { 'i', "idle", init_idle, stop_idle, do_idle },
        { 'n', "raw sniffer", init_snif, stop_snif, do_snif },
        { 'o', "casan sniffer", init_casan, stop_casan, do_casan },
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
        Serial.println ("Starting...") ; // signal the start with a new line
        zigmsg.msgbufsize (MSGBUF_SIZE) ;	// space for 100 received messages
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
        Serial.print("Error: ");
        Serial.print((uint8_t)err, 10);
        Serial.println();
    }
