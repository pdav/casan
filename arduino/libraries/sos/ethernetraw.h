#ifndef ETHERNET_RAW_H
#define ETHERNET_RAW_H

#include "l2eth.h"
#include "utility/w5100.h"
#include "defs.h"

#define		BUFFER_SIZE			127
#define		OFFSET_DEST_ADDR	(0)
#define		OFFSET_SRC_ADDR		(6)
#define		OFFSET_ETHTYPE		(12)
#define		OFFSET_DATA			(14)
#define		OFFSET_RECEPTION	(2)

/*******
 * this class handle the lowest layer, it will talk directly with the 
 * arduino ethernet shield
 */
class EthernetRaw {
	public:
		EthernetRaw();
		~EthernetRaw();
		int available();
		size_t send(l2addr &addr, uint8_t b);
		size_t send(l2addr &mac_dest, const uint8_t *data, size_t len);
		uint8_t recv(void);
		uint8_t recv(uint8_t *data, int *len);

		void get_mac_src(l2addr * mac_src);
		uint8_t * get_offset_payload(int offset);
		int get_payload_length(void);

		void set_master_addr(l2addr * master_addr);
		void set_mac(l2addr *mac_address);
		void set_ethtype(int eth_type);

	private:
		SOCKET _s; // our socket that will be opened in RAW mode
		l2addr_eth *_mac_addr = NULL;
		l2addr_eth *_master_addr = NULL;	// l2 network this slave is on
		int _eth_type;
		byte _rbuf[BUFFER_SIZE]; // receive buffer
		int _rbuflen; // length of data received

		void print_eth_addr (byte addr []);
		void print_packet (byte pkt [], int len);
};

#endif
