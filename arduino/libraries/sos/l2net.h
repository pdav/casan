#ifndef L2NET_H
#define L2NET_H

#include "defs.h"
#include "enum.h"
#include "memory_free.h"

/*******
 * this class is designed for L2 virtualisation
 */
class l2net {
	public:
		virtual ~l2net() {} ;
		virtual size_t send(l2addr &mac_dest, const uint8_t *data, size_t len) = 0 ;
		// the "recv" method copy the received packet in
		// the instance private variable (see _rbuf/_rbuflen below)
		virtual l2_recv_t recv(void) = 0 ;

		virtual l2addr *bcastaddr (void) = 0;
		virtual void get_mac_src(l2addr * mac_src) = 0;
		virtual uint8_t * get_offset_payload(int offset) = 0;
		virtual size_t get_payload_length(void) = 0;

		virtual void set_master_addr(l2addr * master_addr) = 0;

		int mtu (void) { return mtu_ ; } ;

	protected:
		int mtu_ ;	// must be initialized in derived classes
};

#endif
