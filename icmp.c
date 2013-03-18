#include "icmp.h"
#include "networking.h"
#include "ethernet.h"
#include "ipv4.h"
#include "debug.h"

#define ICMP_ECHOREPLY 0
#define ICMP_ECHO 8

void icmp_handle(uint16_t packet_len) {
	if (enc_rx_read_byte() != ICMP_ECHO) {
		/* not an echo request */
		return;
	}

	enc_rx_read_byte(); /* code - ignored */

	enc_rx_read_intbe(); /* checksum - TODO */

	/* all fine, we got ping, we do pong
	 * what follows now is len - 4 bytes that we just copy
	 */

	enc_tx_seek(ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH + 4);
	for (int i = 0; i < packet_len; i++) {
		uint8_t wat;
		wat = enc_rx_read_byte();
		enc_tx_write_byte(wat);
	}

	enc_rx_seek(ETH_HEADER_LENGTH + IPV4_HDR_OFF_SOURCE_IP);
	enc_tx_seek(ETH_HEADER_LENGTH + IPV4_HDR_OFF_DEST_IP);
	for (int i = 0; i < 4; i++) {
		uint8_t wat;
		wat = enc_rx_read_byte();
		enc_tx_write_byte(wat);
	}

	eth_tx_reply();
	enc_rx_acknowledge();

	enc_tx_seek(ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH);
	enc_tx_write_byte(ICMP_ECHOREPLY);
	enc_tx_write_byte(0);

	enc_tx_checksum_icmp(packet_len);

	enc_tx_seek(ETH_HEADER_LENGTH);
	enc_tx_write_byte(0x45);
	enc_tx_write_byte(0x00);
	enc_tx_write_intbe(IPV4_HEADER_LENGTH + packet_len);
	enc_tx_write_intbe(0);
	enc_tx_write_intbe(0);
	enc_tx_write_byte(255);
	enc_tx_write_byte(1);
	enc_tx_write_intbe(0);
	enc_tx_write_buf(our_ipv4, 4);

	enc_tx_checksum_ipv4();

	eth_tx_type(ETHERTYPE_IPV4);
	enc_tx_do(IPV4_HEADER_LENGTH + packet_len); /* FIXME dat api */
}

