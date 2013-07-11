#ifndef L2NET_ETH_H
#define L2NET_ETH_H

#include "l2addr_eth.h"
#include "l2net.h"
#include "utility/w5100.h"
#include "defs.h"
#include "enum.h"
#include "memory_free.h"

#define		OFFSET_DEST_ADDR	(0)
#define		OFFSET_SRC_ADDR		(6)
#define		OFFSET_ETHTYPE		(12)
#define		OFFSET_SIZE			(14)
#define		OFFSET_DATA			(16)
#define		OFFSET_RECEPTION	(2)

/*******
 * this class handle the lowest layer, it will talk directly with the 
 * arduino ethernet shield
 */
class l2net_eth : public l2net {
	public:
		l2net_eth(l2addr *mac_addr, int mtu, int ethtype);
		~l2net_eth();
		size_t send(l2addr &mac_dest, const uint8_t *data, size_t len);
		// the "recv" method copy the received packet in
		// the instance private variable (see _rbuf/_rbuflen below)
		l2_recv_t recv(void);

		l2addr *bcastaddr (void);
		void get_mac_src(l2addr * mac_src);
		uint8_t * get_offset_payload(int offset);
		size_t get_payload_length(void);

		void set_master_addr(l2addr * master_addr);

	private:
		SOCKET _s; // our socket that will be opened in RAW mode
		l2addr_eth *_mac_addr = NULL;
		l2addr_eth *_master_addr = NULL;	// l2 network this slave is on
		int _eth_type;
		byte *_rbuf;	// receive buffer (of size mtu_ bytes)
		int _rbuflen; // length of data received

		int available();
		void print_eth_addr (byte addr []);
		void print_packet (byte pkt [], int len);
		l2_recv_t check_received(void);
};

#endif
