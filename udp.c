#include "udp.h"
#include "networking.h"
#include "ethernet.h"
#include "ipv4.h"
#include "debug.h"

void udp_handle(uint16_t packet_len) {
	/* TODO(?) verify checksum */

	/* FIXME wat */
	enc_rx_seek(ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH + 2);
	uint16_t dport = enc_rx_read_intbe();

	if (dport == 2342) {
		debug_fstr("they're after us\n");

		udp_tx_reply();

		enc_tx_seek(ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH + UDP_HEADER_LENGTH);
		enc_tx_write_byte('w');
		enc_tx_write_byte('a');
		enc_tx_write_byte('t');
		enc_tx_write_byte('?');
		udp_tx_length(4);
		udp_tx_checksum(4);

		enc_tx_do(IPV4_HEADER_LENGTH + UDP_HEADER_LENGTH + 4);
	}
}

void udp_tx_reply() {
	ipv4_tx_reply();

	enc_rx_seek(ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH);
	uint16_t sport = enc_rx_read_intbe();
	uint16_t dport = enc_rx_read_intbe();

	enc_tx_seek(ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH);
	enc_tx_write_intbe(dport);
	enc_tx_write_intbe(sport);
}

void udp_tx_length(uint16_t payload_len) {
	ipv4_tx_header(UDP_HEADER_LENGTH + payload_len, IPPROTO_UDP);

	enc_tx_seek(ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH + 4);
	enc_tx_write_intbe(UDP_HEADER_LENGTH + payload_len);
}

void udp_tx_checksum(uint16_t payload_len) {
	ipv4_tx_checksum();

	/* for computing the checksum, some fields of the pseudo header
	 * have to be constructed
	 */
	enc_tx_seek(ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH + UDP_HEADER_LENGTH + payload_len);
	enc_tx_write_byte(0);
	enc_tx_write_byte(IPPROTO_UDP);
	enc_tx_write_intbe(UDP_HEADER_LENGTH + payload_len);

	/* clear future checksum field */
	enc_tx_seek(ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH + 6);
	enc_tx_write_intbe(0);

	uint16_t sum = enc_tx_checksum(ETH_HEADER_LENGTH + IPV4_HDR_OFF_SOURCE_IP, 4 + 4 + UDP_HEADER_LENGTH + payload_len + 4);

	enc_tx_seek(ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH + 6);
	enc_tx_write_intbe(sum);
}

