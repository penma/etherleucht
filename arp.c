#include "enc28j60.h"
#include "networking.h"
#include "ethernet.h"
#include "ipv4.h"

#include <stdint.h>
#include <string.h>

#define ARPOP_REQUEST 1
#define ARPOP_REPLY 2
#define ARPHRD_ETHER 1
#define ARP_LENGTH 28

void arp_handle(uint16_t packet_len) {
	if (packet_len < 28) {
		/* too small for anything we will ever handle */
		return;
	}

	if (enc_rx_read_intbe() != ARPHRD_ETHER) {
		/* not ethernet */
		return;
	}

	if (enc_rx_read_intbe() != ETHERTYPE_IPV4) {
		/* not ipv4 */
		return;
	}

	if (enc_rx_read_byte() != 6) {
		/* wrong mac address length */
		return;
	}

	if (enc_rx_read_byte() != 4) {
		/* wrong v4 address length */
		return;
	}

	if (enc_rx_read_intbe() != ARPOP_REQUEST) {
		/* not an arp request */
		return;
	}

	/* read the rest of the packet */
	uint8_t sender_hw[6], sender_proto[4], target_proto[4];
	enc_rx_read_buf(sender_hw, 6);
	enc_rx_read_buf(sender_proto, 4);
	for (int i = 0; i < 6; i++) {
		enc_rx_read_byte();
	}
	enc_rx_read_buf(target_proto, 4);

	/* is it for us? */
	if (memcmp(target_proto, our_ipv4, 4)) {
		/* no */
		return;
	}

	/* yes! */
	eth_tx_reply();
	eth_tx_type(ETHERTYPE_ARP);

	enc_tx_seek(ETH_HEADER_LENGTH);

	enc_tx_write_intbe(ARPHRD_ETHER);
	enc_tx_write_intbe(ETHERTYPE_IPV4);
	enc_tx_write_byte(6);
	enc_tx_write_byte(4);
	enc_tx_write_intbe(ARPOP_REPLY);

	enc_tx_write_buf(our_mac, 6);
	enc_tx_write_buf(our_ipv4, 4);
	enc_tx_write_buf(sender_hw, 6);
	enc_tx_write_buf(sender_proto, 4);

	enc_tx_do(ARP_LENGTH);
}

