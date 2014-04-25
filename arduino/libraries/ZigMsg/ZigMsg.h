/**
 * @file ZigMsg.h
 * @brief Low level library to access IEEE 802.15.4 radio though
 *	ATmel AT128RFA1 microcontroller chip
 */

#ifndef ZigMsg_h
#define ZigMsg_h

extern "C" {
#include "config.h"
#include "radio.h"
#include "board.h"
#include "transceiver.h"
}

#define	DEFAULT_MSGBUF_SIZE	10

/** Macro to help write uint16_t (such as addr2 or panid) constants */
#define	CONST16(lo,hi)	(((hi) << 8) | (lo))

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

/**
 * @brief Low-level class to access IEEE 802.15.4 network through 
 *	ATmel AT128RFA1 microcontroller chip
 *
 * This class can be viewed:
 * - either as a low-level mechanism to access 802.15.4 network,
 *	from an upper-level protocol point of view
 * - or as a high-level interface to AT128RFA1 hardware
 *	from a micro-controller point of view
 *
 * This class uses the [uracoli](http://uracoli.nongnu.org)
 * library as a C abstraction layer to program the radio
 * component of AT128RFA1. Some selected uracoli source
 * files are copied in this directory for ZigMsg use.
 *
 * The ZigMsg class uses interrupts to receive
 * 802.15.4 frames while processor is busy on other tasks.
 * Received frames are stored in a circular buffer.
 * The ZigMsg::get_received method retrieves the current
 * frame in the buffer and perform minimal 802.15.4 header
 * decoding, while the ZigMsg::skip_received method
 * advances to the next frame in the buffer.
 *
 * The ZigMsg::sendto method, on another hand, does not buffer 
 * the outgoing message: there is no contention to send frame
 * since this method waits for the frame to be sent.
 * Interrupt is only used to update operational statistics.
 *
 * @bug This class does not handle 64-bits addresses
 * @bug This class does not handle AES frame encryption
 * @bug This class does not support various IEEE 802.15.4 modes (mesh, etc.)
 */

class ZigMsg
{
    public:
	/** @brief PAN id
	 *
	 * Warning: uint16_t are stored as little endian,
	 * thus 0x1234 == "34:12". Use the CONST16 macro
	 * to get the correct value:
	 * ~~~
	 * panid_t p = CONST16 (0xCA, 0xFE) ;    // p <- "ca:fe"
	 * ~~~
	 */

	typedef uint16_t panid_t ;

	/** @brief 16-bits address
	 *
	 * Warning: uint16_t are stored as little endian,
	 * thus 0x1234 == "34:12". Use the CONST16 macro
	 * to get the correct value:
	 * ~~~
	 * addr2_t p = CONST16 (0xBE, 0xEF) ;    // p <- "be:ef"
	 * ~~~
	 */

	typedef uint16_t addr2_t ;
	typedef uint64_t addr8_t ;	/** 64-bits address (not supported at this time */

	/** @brief This structure describes a received frame
	 *	with minimal decoding
	 *
	 * The ZigReceivedFrame is the result of the
	 * ZigMsg::get_received method. It includes pointers
	 * in the receive buffer as well as additional informations
	 * decoded in the IEEE 802.15.4 header such as source and
	 * destination addresses and PAN ids.
	 */

	struct ZigReceivedFrame
	{
	    uint8_t frametype ;		///< Z_FT_DATA, Z_FT_ACK, etc.
	    uint8_t *rawframe ;		///< Pointer to frame header in reception buffer (ZigBuf)
	    uint8_t rawlen ;		///< Frame length
	    uint8_t lqi ;		///< Link Quality Indicator
	    uint8_t *payload ;		///< Pointer to 802.15.5 payload in reception buffer
	    uint8_t paylen ;		///< Payload length (without frame footer)
	    /** @brief Frame Control Field
	     *
	     * The Frame Control Field is the first field (16 bits) of
	     * the 802.15.4 header. Other items in the ZigReceivedFrame
	     * structure are decoded from this field.
	     *
	     * Don't display this field as an integer, since you will
	     * be fooled by the bit order. Use Z_* macros instead.
	     */
	    uint16_t fcf ;
	    uint8_t seq ;		///< Sequence number
	    addr2_t dstaddr ;		///< Destination address
	    addr2_t dstpan ;		///< Destination PAN id
	    addr2_t srcaddr ;		///< Source address
	    addr2_t srcpan ;		///< Source PAN id (same as dstpan if IntraPAN bit is set in FCF)
	} ;

	/** @brief This structure gathers operational statistics
	 *
	 * It is updated when an interrupt (received frame or tx done) occurs.
	 * The ZigMsg::get_stats method returns the current structure,
	 * which may still be updated by interrupt routines.
	 */

	struct ZigStat
	{
	    int rx_overrun ;
	    int rx_crcfail ;
	    int tx_sent ;
	    int tx_error_cca ;
	    int tx_error_noack ;
	    int tx_error_fail ;
	} ;


	ZigMsg () ;

	// Set radio configuration (before starting) or get parameters

	/** Accessor method to get the size (in number of frames) of the receive buffer */
	int msgbufsize (void) { return msgbufsize_ ; }

	/** Accessor method to get the channel id (11 ... 26) */
	channel_t channel (void) { return chan_ ; }

	/** Accessor method to get our 802.15.4 hardware address (16 bits) */
	addr2_t addr2 (void) { return addr2_ ; }

	/** Accessor method to get our 802.15.4 hardware address (64 bits) */
	addr8_t addr8 (void) { return addr8_ ; }

	/** Accessor method to get our 802.15.4 PAN id */
	panid_t panid (void) { return panid_ ; }

	/** Accessor method to get the TX power (-17 ... +3 dBM) */
	txpwr_t txpower (void) { return txpower_ ; }	// -17 .. +3 dBm

	/** Accessor method to get promiscuous status */
	bool promiscuous (void) { return promisc_ ; }

	/** Mutator method to set the size (in number of frames) of the receive buffer */
	void msgbufsize (int msgbufsize) { msgbufsize_ = msgbufsize ; }

	/** Mutator method to set the channel id (11 ... 26) */
	void channel (channel_t chan) { chan_ = chan ; }	// 11..26

	/** Mutator method to set our 802.15.4 hardware address (16 bits) */
	void addr2 (addr2_t addr) { addr2_ = addr ; }

	/** Mutator method to set our 802.15.4 hardware address (16 bits) */
	void addr8 (addr8_t addr) { addr8_ = addr ; }

	/** Mutator method to set our 802.15.4 PAN id */
	void panid (panid_t panid) { panid_ = panid ; }

	/** Mutator method to set the TX power (-17 ... +3 dBM) */
	void txpower (txpwr_t txpower) { txpower_ = txpower ; }

	/** Mutator method to set promiscuous status */
	void promiscuous (bool promisc) { promisc_ = promisc ; }

	// Start radio processing

	void start (void) ;

	// Not really public: interrupt functions are designed to
	// be called outside of an interrupt
	uint8_t *it_receive_frame (uint8_t len, uint8_t *frm, uint8_t lqi, int8_t ed, uint8_t crc_fail) ;
	void it_tx_done (radio_tx_done_t status) ;

	// Send and receive frames

	bool sendto (addr2_t a, uint8_t len, const uint8_t payload []) ;
	ZigReceivedFrame *get_received (void) ;	// get current frame (or NULL)
	void skip_received (void) ;	// skip to next read frame

	/**
	 * Return operational statistics
	 *
	 * Note that returned ZigMsg::ZigStat structure may still be
	 * modified by an interrupt routine.
	 */

	ZigStat *getstat (void) { return &stat_ ; }

    private:
	/* buffer for one received frame */
	struct ZigBuf
	{
	    volatile uint8_t frame [MAX_FRAME_SIZE] ;
	    volatile uint8_t len ;
	    volatile uint8_t lqi ;
	} ;

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
} ;

extern ZigMsg zigmsg ;

#endif
