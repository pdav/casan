#include <ZigMsg.h>

#define	CHANNEL	25
// #define	CHANNEL	12

#define	PANID		CONST16 (0xbe, 0xef)
#define	SENDADDR	CONST16 (0x12, 0x34)
#define	RECVADDR	CONST16 (0x45, 0x67)


#define	PERIODIC	100000

/******************************************************************************
Sniffer
*******************************************************************************/

const char *desc_frametype [] =
{
    "Beacon", "Data", "Ack", "MAC-cmd",
    "Reserved", "Reserved", "Reserved", "Reserved", 
} ;

void phex (uint8_t c)
{
    Serial.print ((c >> 4) & 0xf, 16) ;
    Serial.print ((c     ) & 0xf, 16) ;
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

uint8_t *padrpan (uint8_t *p, int len, int ispan)
{
    uint8_t *adr = p ;

    if (ispan)
	adr += 2 ;
    phexn (adr, len, ':') ;
    adr += len ;
    Serial.print ('/') ;
    if (ispan)
	phexn (p, 2, ':') ;
    return adr ;
}

uint8_t *padr (int mode, uint8_t *p, int ispan)
{
    switch (mode)
    {
	case Z_ADDRMODE_NOADDR :
	    Serial.print ("()") ;
	    break ;
	case Z_ADDRMODE_RESERVED :
	    Serial.print ("?reserved") ;
	    break ;
	case Z_ADDRMODE_ADDR2 :
	    p = padrpan (p, 2, ispan) ;
	    break ;
	case Z_ADDRMODE_ADDR8 :
	    p = padrpan (p, 6, ispan) ;
	    break ;
    }
    return p ;
}

void print_frame (ZigReceivedFrame *z)
{
    uint8_t *p ;
    int intrapan ;
    int dstmode, srcmode ;

    Serial.print (desc_frametype [Z_GET_FRAMETYPE (z->fcf)]) ;
    Serial.print (" Sec=") ;	Serial.print (Z_GET_SEC_ENABLED (z->fcf)) ;
    Serial.print (" Pending=") ; Serial.print (Z_GET_FRAME_PENDING (z->fcf)) ;
    Serial.print (" AckReq=") ;	Serial.print (Z_GET_ACK_REQUEST (z->fcf)) ;
    Serial.print (" V=") ;	Serial.print (Z_GET_FRAME_VERSION (z->fcf)) ;

    intrapan = Z_GET_INTRA_PAN (z->fcf) ;
    Serial.print (" Intra=") ;	Serial.print (intrapan) ;
    dstmode = Z_GET_DST_ADDR_MODE (z->fcf) ;
    srcmode = Z_GET_SRC_ADDR_MODE (z->fcf) ;

    p = z->rawframe + 2 ;
    Serial.print (" Seq=") ;
    phex (*p) ;
    p++ ;

    Serial.print (" ") ;
    p = padr (dstmode, p, 1) ;
    Serial.print ("<-") ;
    p = padr (srcmode, p, ! intrapan) ;

    Serial.print (" Len=") ;
    Serial.print (z->paylen) ;

    Serial.print (" [") ;
    phexn (p, z->paylen, ',') ;
    Serial.print ("] LQI=") ;	Serial.print (z->lqi) ;
    Serial.println () ;
}

void print_stat (void)
{
    ZigStat *zs = ZigMsg.getstat () ;
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

void init_snif (char line [])
{
    ZigMsg.channel (CHANNEL) ;
    ZigMsg.promiscuous (true) ;
    ZigMsg.start () ;
    Serial.println ("Starting sniffer") ;
}

void stop_snif (void)
{
    Serial.println ("Stopping sniffer") ;
}

void do_snif (void)
{
    static int n = 0 ;
    ZigReceivedFrame *z ;

    if (++n % PERIODIC == 0)
    {
	print_stat () ;
	n = 0 ;
    }

    while ((z = ZigMsg.get_received ()) != NULL)
    {
	print_frame (z) ;
	ZigMsg.skip_received () ;
    }
}

/******************************************************************************
Sender
*******************************************************************************/

void init_send (char line [])
{
    ZigMsg.channel (CHANNEL) ;
    ZigMsg.panid (PANID) ;
    ZigMsg.addr2 (SENDADDR) ;
    ZigMsg.promiscuous (false) ;
    ZigMsg.start () ;
    Serial.println("Starting send") ;
}

void stop_send (void)
{
    Serial.println("Stopping send") ;
}

void do_send (void)
{
    static int n = 0 ;
    ZigStat *zs ;

    if (++n % PERIODIC == 0)
    {
	zs = ZigMsg.getstat () ;
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
	n = 0 ;

	uint32_t time ;
	time = millis () ;
	if (ZigMsg.sendto (RECVADDR, 4, (uint8_t *) &time))
	    Serial.println ("Sent") ;
	else
	    Serial.println ("Sent error") ;
    }
}

/******************************************************************************
Receiver
*******************************************************************************/

void init_recv (char line [])
{
    ZigMsg.channel (CHANNEL) ;
    ZigMsg.panid (PANID) ;
    ZigMsg.addr2 (RECVADDR) ;
    ZigMsg.promiscuous (false) ;
    ZigMsg.start () ;
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
    { 'n', "sniffer", init_snif, stop_snif, do_snif },
    { 's', "sender", init_send, stop_send, do_send },
    { 'r', "receiver", init_recv, stop_recv, do_recv },
} ;
#define	IDLE_MODE (& gui [0])

#define	NTAB(t)	((int) (sizeof (t) / sizeof (t) [0]))

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
    Serial.begin(38400);	// don't introduce spaces here
    Serial.println ("Starting...") ; // signal the start with a new line
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
