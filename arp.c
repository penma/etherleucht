#include "enc28j60.h"
#include "networking.h"
#include "ethernet.h"
#include "ipv4.h"

#include <stdint.h>
#include <string.h>

#define ARPOP_REQUEST 1
#define ARPOP_REPLY 2
#define ARPHRD_ETHER 1

void arp_handle() {
	if (enc_rx_read_intbe() != ARPHRD_ETHER) {
		/* not ethernet */
		goto ignore;
	}

	if (enc_rx_read_intbe() != ETHERTYPE_IPV4) {
		/* not ipv4 */
		goto ignore;
	}

	if (enc_rx_read_byte() != 6) {
		/* wrong mac address length */
		goto ignore;
	}

	if (enc_rx_read_byte() != 4) {
		/* wrong v4 address length */
		goto ignore;
	}

	if (enc_rx_read_intbe() != ARPOP_REQUEST) {
		/* not an arp request */
		goto ignore;
	}

	/* read the rest of the packet and ack it */
	uint8_t sender_hw[6], sender_proto[4], target_proto[4];
	enc_rx_read_buf(sender_hw, 6);
	enc_rx_read_buf(sender_proto, 4);
	for (int i = 0; i < 6; i++) {
		enc_rx_read_byte();
	}
	enc_rx_read_buf(target_proto, 4);

	enc_rx_stop();
	enc_tx_prepare_reply();
	enc_rx_acknowledge();

	/*
	for (int i = 0; i < 4; i++) {
		debug_dec16(target_proto[i]);
		debug_fstr(".");
	}
	debug_fstr("?\n");
	*/

	/* is it for us? */
	if (memcmp(target_proto, our_ipv4, 4)) {
		/* no */
		return;
	}

	/* yes! */
	enc_tx_seek(ETH_HEADER_LENGTH);
	enc_tx_start();

	enc_tx_write_intbe(ARPHRD_ETHER);
	enc_tx_write_intbe(ETHERTYPE_IPV4);
	enc_tx_write_byte(6);
	enc_tx_write_byte(4);
	enc_tx_write_intbe(ARPOP_REPLY);

	extern uint8_t mac[6];
	enc_tx_write_buf(mac, 6);
	enc_tx_write_buf(our_ipv4, 4);
	enc_tx_write_buf(sender_hw, 6);
	enc_tx_write_buf(sender_proto, 4);

	enc_tx_stop();
	enc_tx_do(28, ETHERTYPE_ARP);

	return;
ignore:
	// debug_fstr("ARP ignored!\n");
	enc_rx_stop();
	enc_rx_acknowledge();
}

