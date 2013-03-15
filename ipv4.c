#include "enc28j60.h"
#include "networking.h"
#include "ipv4.h"
#include "icmp.h"
#include "debug.h"

#include <string.h>

uint8_t our_ipv4[4] = { 172, 22, 26, 219 };

void ipv4_handle() {
	if (enc_rx_read_byte() != 0x45) {
		/* not v4 or not 20 byte header
		 * the latter would be legal, but nobody really does this
		 */
		goto ignore;
	}

	enc_rx_read_byte(); /* DSCP/ECN - ignored */

	uint16_t len = enc_rx_read_intbe() - IPV4_HEADER_LENGTH;
	// debug_dec16(len);
	// debug_fstr("b ");

	enc_rx_read_intbe(); /* fragment ID - ignored */
	uint16_t frag = enc_rx_read_intbe();

	if ((frag & ~(0b01000000 << 8)) != 0) {
		/* non-zero offset, more fragments following etc - nope */
		goto ignore;
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
		goto ignore;
	}

	// debug_fstr("for us!\n");

	if (proto == IPPROTO_ICMP) {
		icmp_handle(len);
		return;
	} else {
		debug_fstr("unhandled\nproto ");
		debug_hex8(proto);
		debug_fstr("\n");
		goto ignore;
	}

ignore:
	debug_fstr("ignored!\n");
	enc_rx_stop();
	enc_rx_acknowledge();
}

