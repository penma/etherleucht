#include "ethernet.h"
#include "networking.h"
#include "arp.h"
#include "ipv4.h"
#include "debug.h"

uint8_t our_mac[6] = { 0x42, 0xcc, 0xcd, 0x00, 0x13, 0x37 };

/* copy sender MAC of received packet to destination MAC of transmit packet,
 * and our own MAC to source MAC.
 * i.e., prepare a reply packet
 */
void eth_tx_reply() {
	uint8_t sender[6];

	enc_rx_seek(ETH_HDR_OFF_SOURCE);
	enc_rx_read_buf(sender, 6);

	enc_tx_seek(ETH_HDR_OFF_DEST);
	enc_tx_write_buf(sender, 6);
	enc_tx_write_buf(our_mac, 6);
}

/* write ethertype into the TX buffer */
void eth_tx_type(uint16_t ethertype) {
	enc_tx_seek(ETH_HDR_OFF_ETHERTYPE);
	enc_tx_write_intbe(ethertype);
}

void eth_handle(uint16_t frame_length) {
	enc_rx_seek(ETH_HDR_OFF_ETHERTYPE);
	uint16_t ethertype = enc_rx_read_intbe();

	if (ethertype == ETHERTYPE_ARP) {
		arp_handle(frame_length - ETH_HEADER_LENGTH);
	} else if (ethertype == ETHERTYPE_IPV4) {
		ipv4_handle(frame_length - ETH_HEADER_LENGTH);
	} else {
		debug_fstr("unknown\nethertype\n");
		debug_hex16(ethertype);
		debug_fstr("\n");
	}

	enc_rx_acknowledge();
}
