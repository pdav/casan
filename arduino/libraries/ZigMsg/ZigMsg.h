#ifndef ZigMsg_h
#define ZigMsg_h

extern "C" {
#include "config.h"
#include "radio.h"
#include "board.h"
#include "transceiver.h"
}

#define	DEFAULT_MSGBUF_SIZE	10

/* warning: uint16_t are stored as little endian, thus "12:34" = 0x3412 */
typedef uint16_t panid_t ;
typedef uint16_t addr2_t ;
typedef uint64_t addr8_t ;		// not supported at this time

/* macro to help write constant uint16_t such as addr2 or panid */
#define	CONST16(lo,hi)	(((hi) << 8) | (lo))

/* a decoded frame */
struct ZigReceivedFrame
{
    uint8_t frametype ;
    uint8_t *rawframe ;			// in ZigBuf
    uint8_t rawlen ;
    uint8_t lqi ;
    uint8_t *payload ;			// in ZigBuf
    uint8_t paylen ;
    uint16_t fcf ;			// Frame Control Field
    		// warning : don't print FCF, you will be fooled by bit order
		// use Z_* macros instead
    uint8_t seq ;
    addr2_t dstaddr ;
    addr2_t dstpan ;
    addr2_t srcaddr ;
    addr2_t srcpan ;
} ;

/*
 * Definitions for FCF (IEEE 802.15.4 Frame Control Field)
 */

#define	Z_GET_FRAMETYPE(w)	(((w)>>0) & 0x7)
#define	Z_SET_FRAMETYPE(n)	(n)
#define	 Z_FT_BEACON			0
#define	 Z_FT_DATA			1
#define  Z_FT_ACK			2
#define  Z_FT_MACCMD			3
#define	Z_GET_SEC_ENABLED(w)	(((w)>>3) & 0x1)
#define	Z_SET_SEC_ENABLED(n)	((n)<<3)
#define	Z_GET_FRAME_PENDING(w)	(((w)>>4) & 0x1)
#define	Z_SET_FRAME_PENDING(n)	((n)<<4)
#define	Z_GET_ACK_REQUEST(w)	(((w)>>5) & 0x1)
#define	Z_SET_ACK_REQUEST(n)	((n)<<5)
#define	Z_GET_INTRA_PAN(w)	(((w)>>6) & 0x1)
#define	Z_SET_INTRA_PAN(n)	((n)<<6)
#define	Z_GET_RESERVED(w)	(((w)>>7) & 0x3)
#define	Z_SET_RESERVED(n)	((n)<<7)
#define	Z_GET_DST_ADDR_MODE(w)	(((w)>>10) & 0x3)
#define	Z_SET_DST_ADDR_MODE(n)	((n)<<10)
#define	 Z_ADDRMODE_NOADDR		0
#define	 Z_ADDRMODE_RESERVED		1
#define	 Z_ADDRMODE_ADDR2		2
#define	 Z_ADDRMODE_ADDR8		3
#define	Z_GET_FRAME_VERSION(w)	(((w)>>12) & 0x3)
#define	Z_SET_FRAME_VERSION(n)	((n)<<12)
#define	 Z_FV_2003			0
#define	 Z_FV_2006			1
#define	 Z_FV_RESERVED2			2
#define	 Z_FV_RESERVED3			3
#define	Z_GET_SRC_ADDR_MODE(w)	(((w)>>14) & 0x3)
#define	Z_SET_SRC_ADDR_MODE(n)	((n)<<14)

// uint16_t are stored with the least significant byte first
#define	Z_SET_INT16(p,val) \
			(((uint8_t *) p) [0] = ((val)>>0) & 0xff, \
			 ((uint8_t *) p) [1] = ((val)>>8) & 0xff )
#define	Z_GET_INT16(p)	( (((uint8_t *) p) [0] << 0) | \
			  (((uint8_t *) p) [1] << 8) )

/* buffer for one received frame */
struct ZigBuf
{
    volatile uint8_t frame [MAX_FRAME_SIZE] ;
    volatile uint8_t len ;
    volatile uint8_t lqi ;
} ;

/* all statistics */
struct ZigStat
{
    int rx_overrun ;
    int rx_crcfail ;
    int tx_sent ;
    int tx_error_cca ;
    int tx_error_noack ;
    int tx_error_fail ;
} ;

class cZigMsg
{
    private:
	/*
	 * Global statistics
	 */

	ZigStat stat_ ;

	/*
	 * Radio configuraiton
	 */

	txpwr_t txpwr_ ;
	channel_t chan_ ;
	panid_t panid_ ;
	addr2_t addr2_ ;
	addr8_t addr8_ ;
	txpwr_t txpower_ ;
	bool promisc_ ;

	/*
	 * Fifo for received messages
	 */

	ZigBuf *rbuffer_ ;		// with msgbufsize_ entries
	volatile int rbuffirst_ ;
	volatile int rbuflast_ ;
	int msgbufsize_ ;
	ZigReceivedFrame rframe_ ;	// decoded received frame

	/*
	 * Transmission
	 */

	uint8_t seqnum_ ;		// to be placed in MAC header
	volatile bool writing_ ;

    public:
	cZigMsg () ;

	// Set radio configuration (before starting) or get parameters

	void msgbufsize (int msgbufsize) { msgbufsize_ = msgbufsize ; }
	int msgbufsize (void) { return msgbufsize_ ; }

	void channel (channel_t chan) { chan_ = chan ; }	// 11..26
	channel_t channel (void) { return chan_ ; }

	void addr2 (addr2_t addr) { addr2_ = addr ; }
	addr2_t addr2 (void) { return addr2_ ; }

	void addr8 (addr8_t addr) { addr8_ = addr ; }
	addr8_t addr8 (void) { return addr8_ ; }

	void panid (panid_t panid) { panid_ = panid ; }
	panid_t panid (void) { return panid_ ; }

	void txpower (txpwr_t txpower) { txpower_ = txpower ; }
	txpwr_t txpower (void) { return txpower_ ; }	// -17 .. +3 dBm

	void promiscuous (bool promisc) { promisc_ = promisc ; }
	bool promiscuous (void) { return promisc_ ; }

	// Start radio processing

	void start (void) ;

	// Not really public: interrupt functions are designed to
	// be called outside of an interrupt
	uint8_t *it_receive_frame (uint8_t len, uint8_t *frm, uint8_t lqi, int8_t ed, uint8_t crc_fail) ;
	void it_tx_done (radio_tx_done_t status) ;

	// Send and receive frames

	bool sendto (addr2_t a, uint8_t len, uint8_t payload []) ;
	ZigReceivedFrame *get_received (void) ;	// get current frame (or NULL)
	void skip_received (void) ;	// skip to next read frame

	// Get statistics

	struct ZigStat *getstat (void) { return &stat_ ; }
} ;

extern cZigMsg ZigMsg ;

#endif
