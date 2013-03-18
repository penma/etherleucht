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

	/* TODO(?) verify checksum */

	/* all fine, we got ping, we do pong
	 * we copy packet_len - 4 bytes of payload (sequence number etc)
	 */

	enc_rxtx_copy(
		ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH + 4,
		ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH + 4,
		packet_len - 4);

	eth_tx_reply();
	eth_tx_type(ETHERTYPE_IPV4);

	ipv4_tx_reply();
	ipv4_tx_header(packet_len, IPPROTO_ICMP);
	ipv4_tx_checksum();

	enc_tx_seek(ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH);
	enc_tx_write_byte(ICMP_ECHOREPLY);
	enc_tx_write_byte(0);
	enc_tx_write_intbe(0);

	uint16_t icmp_sum = enc_tx_checksum(ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH, packet_len);
	enc_tx_seek(ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH + 2);
	enc_tx_write_intbe(icmp_sum);

	enc_tx_do(IPV4_HEADER_LENGTH + packet_len);
}

