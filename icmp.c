#include "icmp.h"
#include "networking.h"
#include "ipv4.h"
#include "debug.h"

#define ICMP_ECHOREPLY 0
#define ICMP_ECHO 8

void icmp_handle(uint16_t len) {
	if (enc_rx_read_byte() != ICMP_ECHO) {
		/* not an echo request */
		goto ignore;
	}

	enc_rx_read_byte(); /* code - ignored */

	enc_rx_read_intbe(); /* checksum - TODO */

	/* all fine, we got ping, we do pong
	 * what follows now is len - 4 bytes that we just copy
	 */
	enc_rx_stop();

	enc_tx_seek(ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH + 4);
	for (int i = 0; i < len; i++) {
		uint8_t wat;
		enc_rx_start();
		wat = enc_rx_read_byte();
		enc_rx_stop();
		enc_tx_start();
		enc_tx_write_byte(wat);
		enc_tx_stop();
	}

	enc_rx_seek(ETH_HEADER_LENGTH + IPV4_HDR_OFF_SOURCE_IP);
	enc_tx_seek(ETH_HEADER_LENGTH + IPV4_HDR_OFF_DEST_IP);
	for (int i = 0; i < 4; i++) {
		uint8_t wat;
		enc_rx_start();
		wat = enc_rx_read_byte();
		enc_rx_stop();
		enc_tx_start();
		enc_tx_write_byte(wat);
		enc_tx_stop();
	}

	enc_tx_prepare_reply();
	enc_rx_acknowledge();

	enc_tx_seek(ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH);
	enc_tx_start();
	enc_tx_write_byte(ICMP_ECHOREPLY);
	enc_tx_write_byte(0);
	enc_tx_stop();

	enc_tx_checksum_icmp(len);

	enc_tx_seek(ETH_HEADER_LENGTH);
	enc_tx_start();
	enc_tx_write_byte(0x45);
	enc_tx_write_byte(0x00);
	enc_tx_write_intbe(IPV4_HEADER_LENGTH + len);
	enc_tx_write_intbe(0);
	enc_tx_write_intbe(0);
	enc_tx_write_byte(255);
	enc_tx_write_byte(1);
	enc_tx_write_intbe(0);
	enc_tx_write_buf(our_ipv4, 4);

	enc_tx_stop();
	enc_tx_checksum_ipv4();

	enc_tx_do(IPV4_HEADER_LENGTH + len, ETHERTYPE_IPV4); /* FIXME dat api */

	return;
ignore:
	debug_fstr("ICMP IGNORED\n");
	enc_rx_stop();
	enc_rx_acknowledge();
}

