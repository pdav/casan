#include <Arduino.h>
#include "ZigMsg.h"

cZigMsg ZigMsg ;

/*******************************************************************************
 * Functions called by radio_rfa.c and trx_rfa.c
 * (upon interrupt or another exceptional condition)
 */

/*
 * Called by various (not interrupt) functions when an error (such as
 * an invalid parameter) occurs
 */

void usr_radio_error (radio_error_t err)
{
}

/*
 * Called upon interrupt by radio_receive_frame when a frame has
 * been completely received and contents has been copied in the
 * radiostatus.rxframe buffer.
 * Return value is the address for the new frame.
 */

uint8_t *usr_radio_receive_frame (uint8_t len, uint8_t *frm, uint8_t lqi, int8_t ed, uint8_t crc_fail)
{
    return ZigMsg.it_receive_frame (len, frm, lqi, ed, crc_fail) ;
}

uint8_t *cZigMsg::it_receive_frame (uint8_t len, uint8_t *frm, uint8_t lqi, int8_t ed, uint8_t crc_fail)
{
    if (crc_fail)
	stat_.rx_crcfail++ ;
    else
    {
	int newlast = (rbuflast_ + 1) % msgbufsize_ ;
	if (newlast == rbuffirst_)
	    stat_.rx_overrun++ ;
	else
	{
	    /*
	     * There is room for a new frame:
	     * - update the message with length and lqi
	     * - update the frame buffer pointer
	     */

	    rbuffer_ [rbuflast_].len = len ;
	    rbuffer_ [rbuflast_].lqi = lqi ;

	    rbuflast_ = newlast ;
	    frm = (uint8_t *) rbuffer_ [newlast].frame ;
	}
    }
    return frm;
}

/*
 * Called by interrupt routine (see radio_rfa.c) when a transmission
 * is done. Update statistics.
 */

void usr_radio_tx_done (radio_tx_done_t status)
{
    ZigMsg.it_tx_done (status) ;
}

void cZigMsg::it_tx_done (radio_tx_done_t status)
{
    switch (status)
    {
	case TX_OK :
	    stat_.tx_sent++ ;
	    break ;
	case TX_CCA_FAIL :
	    stat_.tx_error_cca++ ;
	    break ;
	case TX_NO_ACK :
	    stat_.tx_error_noack++ ;
	    break ;
	case TX_FAIL :
	default :
	    stat_.tx_error_fail++ ;
	    break ;
    }
    writing_ = false ;
}

/*******************************************************************************
 * Class methods
 */

cZigMsg::cZigMsg ()
{
    txpower_ = 3 ;			// default: +3 dBm
    chan_ = 12 ;			// default: channel 12
    writing_ = false ;
}

void cZigMsg::start(void)
{
    /*
     * Initializes Fifo for received messages
     */

    if (msgbufsize_ == 0)		// prevent stupid errors...
	msgbufsize_ = DEFAULT_MSGBUF_SIZE ;

    if (rbuffer_ != NULL)
	delete rbuffer_ ;
    rbuffer_ = new ZigBuf [msgbufsize_] ;
    rbuffirst_ = 0 ;
    rbuflast_ = 0 ;

    /*
     * Initializes statistics
     */

    memset (&stat_, 0, sizeof stat_) ;

    /*
     * Start radio configuration
     */

    writing_ = false ;
    seqnum_ = 0 ;

    radio_init ((uint8_t *) rbuffer_ [rbuflast_].frame, MAX_FRAME_SIZE) ;

    radio_set_param (RP_CHANNEL (chan_)) ;

    radio_set_param (RP_TXPWR (txpower_)) ;	// -17 .. +3 dBm

    if (promisc_)
    {
	radio_set_param (RP_PANID (0)) ;
	radio_set_param (RP_SHORTADDR (0)) ;
	radio_set_param (RP_LONGADDR (0)) ;
	trx_bit_write (SR_AACK_PROM_MODE, 1) ;	// promisc mode
	trx_bit_write (SR_AACK_DIS_ACK, 1) ;	// disable ack
	radio_set_state (STATE_RX) ;		// enable reception
    }
    else
    {
	radio_set_param (RP_PANID (panid_)) ;
	radio_set_param (RP_SHORTADDR (addr2_)) ;
	radio_set_param (RP_LONGADDR (addr8_)) ;
	radio_set_state (STATE_RXAUTO) ;	// enable reception w/ auto-ack
    }
}

/******************************************************************************
 * Send a message
 */

bool cZigMsg::sendto (addr2_t a, uint8_t len, uint8_t payload [])
{
    uint8_t frame [MAX_FRAME_SIZE] ;	// not counting PHR
    uint16_t fcf ;			// frame control field
    int frmlen ;			// frame len

    frmlen = 9 + len + 2 ;			// 9: header, 2: FCS
    if (frmlen > MAX_FRAME_SIZE)
	return false ;

    // Enter state PLL_ON as soon as radio the is not busy (reading
    // a frame or sending an ack or...)
    // This ensures that we won't receive a new frame as we send one.

    // radio_set_state (STATE_TX) ;	// badly named state: actually PLL_ON
    radio_set_state (STATE_TXAUTO) ;	// csma/ca & automatic retransmission

    // Prepare the frame with MAC information

    fcf = Z_SET_FRAMETYPE (Z_FT_DATA)
	    | Z_SET_SEC_ENABLED (0)
	    | Z_SET_FRAME_PENDING (0)
	    | Z_SET_ACK_REQUEST (1)
	    | Z_SET_INTRA_PAN (1)
	    | Z_SET_RESERVED (0)
	    | Z_SET_DST_ADDR_MODE (Z_ADDRMODE_ADDR2)
	    | Z_SET_FRAME_VERSION (Z_FV_2003)
	    | Z_SET_SRC_ADDR_MODE (Z_ADDRMODE_ADDR2)
	    ;
    Z_SET_INT16 (&frame [0], fcf) ;		// fcf
    frame [2] = ++seqnum_ ; ;			// seq
    Z_SET_INT16 (&frame [3], panid_) ;		// dst panid
    Z_SET_INT16 (&frame [5], a) ;		// dst addr
    Z_SET_INT16 (&frame [7], addr2_) ;		// src addr

    // no src panid since intra-pan bit is set

    memcpy (frame + 9, payload, len) ;

    // FCS is automaticall computed. No need to add it here

    // Send the frame
    writing_ = true ;
    radio_send_frame (frmlen, frame, 0) ;

    // wait until finished
    while (writing_)
	;

    // start reading messages again
    radio_set_state (STATE_RXAUTO) ;

    return true ;
}

/******************************************************************************
 * Receive messages
 */

ZigReceivedFrame *cZigMsg::get_received (void)
{
    ZigReceivedFrame *r ;
    ZigBuf *b ;

    noInterrupts () ;
    if (rbuffirst_ == rbuflast_)
	b = NULL ;
    else b = & rbuffer_ [rbuffirst_] ;
    interrupts () ;

    if (b == NULL)
	r = NULL ;
    else
    {
	uint8_t *p = (uint8_t *) b->frame ;
	int intrapan ;

	r = & rframe_ ;
	r->rawframe = (uint8_t *) b->frame ;
	r->rawlen = b->len ;
	r->lqi = b->lqi ;

	/* decode Frame Control Field */
	r->fcf = Z_GET_INT16 (p) ;
	p += 2 ;

	/* Sequence Number */
	r->seq = *p++ ;

	/* Dest & Src address */
	intrapan = Z_GET_INTRA_PAN (r->fcf) ;
	switch (Z_GET_DST_ADDR_MODE (r->fcf))
	{
	    case Z_ADDRMODE_NOADDR :
		break ;
	    case Z_ADDRMODE_RESERVED :
		break ;
	    case Z_ADDRMODE_ADDR2 :
		r->dstpan = Z_GET_INT16 (p) ;
		p += 2 ;
		r->dstaddr = Z_GET_INT16 (p) ;
		p += 2 ;
		break ;
	    case Z_ADDRMODE_ADDR8 :
		p += 8+2 ;
		break ;
	}
	switch (Z_GET_SRC_ADDR_MODE (r->fcf))
	{
	    case Z_ADDRMODE_NOADDR :
		break ;
	    case Z_ADDRMODE_RESERVED :
		break ;
	    case Z_ADDRMODE_ADDR2 :
		if (intrapan != 0)
		    r->srcpan = r->dstpan ;
		else
		{
		    r->srcpan = Z_GET_INT16 (p) ;
		    p += 2 ;
		}
		r->srcaddr = Z_GET_INT16 (p) ;
		p += 2 ;
		break ;
	    case Z_ADDRMODE_ADDR8 :
		if (intrapan == 0)
		    p += 2 ;
		p += 8 ;
		break ;
	}

	r->payload = p ;
	r->paylen = r->rawlen - (p - r->rawframe) - 2 ;	// skip FCS
    }
    return r ;
}

void cZigMsg::skip_received (void)
{
    noInterrupts () ;
    if (rbuffirst_ != rbuflast_)
	rbuffirst_ = (rbuffirst_ + 1) % msgbufsize_ ;
    interrupts () ;
}
