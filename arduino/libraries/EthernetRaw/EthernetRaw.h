#ifndef ETHERNET_RAW_H
#define ETHERNET_RAW_H

#include "Arduino.h"
#include "utility/w5100.h"

#define		BUFFER_SIZE			125
#define		OFFSET_DEST_ADDR	(0)
#define		OFFSET_SRC_ADDR		(6)
#define		OFFSET_ETHTYPE		(12)
#define		OFFSET_DATA			(14)
#define		OFFSET_RECEPTION	(2)
#define		TAG_1Q				0x8100


class EthernetRaw {
	public:
		EthernetRaw();
		void setmac(uint8_t *mac_address);
		void setethtype(uint8_t *eth_type);
		size_t send(uint8_t *addr, uint8_t b);
		size_t send(uint8_t *mac_dest, const uint8_t *data, size_t len);
		int available();
		int recv(uint8_t *mac_addr_src, uint8_t *buf, int *len);
		void get_mac_src(uint8_t * mac_src);
		uint8_t * get_offset_payload(int offset);
		int get_payload_length(void);

		int _rbuflen; // length of data received
	private:
		SOCKET _s; // our socket that will be opened in RAW mode
		uint8_t _mac_addr[6];
		uint8_t _eth_type[2];
		byte _rbuf[BUFFER_SIZE]; // receive buffer
		void print_eth_addr (byte addr []);
		void print_packet (byte pkt [], int len);
};

#endif
