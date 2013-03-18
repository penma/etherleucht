#include "enc28j60.h"
#include "networking.h"
#include "ipv4.h"
#include "icmp.h"
#include "debug.h"

#include <string.h>

uint8_t our_ipv4[4] = { 172, 22, 26, 219 };

void ipv4_handle(uint16_t packet_len) {
	if (packet_len < IPV4_HEADER_LENGTH) {
		return;
	}

	if (enc_rx_read_byte() != 0x45) {
		/* not v4 or not 20 byte header
		 * the latter would be legal, but nobody really does this
		 */
		return;
	}

	enc_rx_read_byte(); /* DSCP/ECN - ignored */

	uint16_t len = enc_rx_read_intbe() - IPV4_HEADER_LENGTH;
	if (len + IPV4_HEADER_LENGTH > packet_len) {
		/* something is not quite right here */
		return;
	}

	enc_rx_read_intbe(); /* fragment ID - ignored */
	uint16_t frag = enc_rx_read_intbe();

	if ((frag & ~(0b01000000 << 8)) != 0) {
		/* non-zero offset, more fragments following etc - nope */
		return;
	}

	enc_rx_read_byte(); /* TTL - ignored */

	uint8_t proto = enc_rx_read_byte();

	enc_rx_read_intbe(); /* header checksum - TODO */

	uint8_t srcip[4];
	enc_rx_read_buf(srcip, 4);

	uint8_t dstip[4];
	enc_rx_read_buf(dstip, 4);
	if (memcmp(dstip, our_ipv4, 4)) {
		/* it's not for us... ok... */
		return;
	}

	if (proto == IPPROTO_ICMP) {
		icmp_handle(len);
	} else {
		debug_fstr("unhandled\nproto ");
		debug_hex8(proto);
		debug_fstr("\n");
	}
}

void ipv4_tx_reply() {
	enc_rx_seek(ETH_HEADER_LENGTH + IPV4_HDR_OFF_SOURCE_IP);
	enc_tx_seek(ETH_HEADER_LENGTH + IPV4_HDR_OFF_SOURCE_IP);

	uint8_t sender[4];
	enc_rx_read_buf(sender, 4);
	enc_tx_write_buf(our_ipv4, 4);
	enc_tx_write_buf(sender, 4);
}

void ipv4_tx_header(uint16_t payload_len, uint8_t proto) {
	enc_tx_seek(ETH_HEADER_LENGTH);
	enc_tx_write_byte(0x45);
	enc_tx_write_byte(0x00);
	enc_tx_write_intbe(IPV4_HEADER_LENGTH + payload_len);
	enc_tx_write_intbe(0);
	enc_tx_write_intbe(0);
	enc_tx_write_byte(64);
	enc_tx_write_byte(proto);
	enc_tx_write_intbe(0);
	enc_tx_write_buf(our_ipv4, 4);
}

void ipv4_tx_checksum() {
	enc_tx_seek(ETH_HEADER_LENGTH + IPV4_HDR_OFF_CHECKSUM);
	enc_tx_write_intbe(0);

	uint16_t sum = enc_tx_checksum(ETH_HEADER_LENGTH, IPV4_HEADER_LENGTH);

	enc_tx_seek(ETH_HEADER_LENGTH + IPV4_HDR_OFF_CHECKSUM);
	enc_tx_write_intbe(sum);
}

