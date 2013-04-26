#ifndef ETHERNET_RAW_H
#define ETHERNET_RAW_H

#include "Arduino.h"
#include "utility/w5100.h"

#define		BUFFER_SIZE			127
#define		OFFSET_DEST_ADDR	(0)
#define		OFFSET_SRC_ADDR		(6)
#define		OFFSET_ETHTYPE		(12)
#define		OFFSET_DATA			(14)
#define		OFFSET_RECEPTION	(2)
#define		TAG_1Q				0x8100


class EthernetRaw {
	public:
		EthernetRaw();
		~EthernetRaw();
		int available();
		size_t send(l2addr &addr, uint8_t b);
		size_t send(l2addr &mac_dest, const uint8_t *data, size_t len);
		uint8_t recv(void);
		uint8_t recv(uint8_t *buf, int *len);

		void get_mac_src(l2addr * mac_src);
		uint8_t * get_offset_payload(int offset);
		int get_payload_length(void);

		void set_master_addr(l2addr * master_addr);
		void set_mac(l2addr *mac_address);
		void set_ethtype(uint8_t *eth_type);
		void set_ethtype(uint16_t eth_type);

	private:
		SOCKET _s; // our socket that will be opened in RAW mode
		l2addr_eth *_mac_addr = NULL;
		l2addr_eth *_master_addr = NULL;	// l2 network this slave is on
		uint8_t _eth_type[2];
		byte _rbuf[BUFFER_SIZE]; // receive buffer
		int _rbuflen; // length of data received

		void print_eth_addr (byte addr []);
		void print_packet (byte pkt [], int len);
};

#endif
