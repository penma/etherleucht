
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdlib.h>
#include <string.h>

#include "spi.h"
#include "networking.h"
#include "ipv4.h"
#include "debug.h"

volatile uint8_t enc_interrupt = 0;

//static uint8_t our_ip[4] = { 192, 168, 0, 250 };
static uint8_t our_ip[4] = { 172, 22, 26, 219 };

ISR(INT1_vect) {
	/* record that we shall handle a packet
	 * then, disable this interrupt to prevent the enc28j60 from
	 * terrorpoking us
	 */
	enc_interrupt = 1;
	GIMSK &= ~(1 << INT1);
}

#define ARPOP_REQUEST 1
#define ARPOP_REPLY 2
#define ARPHRD_ETHER 1

void handle_arp() {
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

	/* is it for us? */
	if (memcmp(target_proto, our_ip, 4)) {
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
	enc_tx_write_buf(our_ip, 4);
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

#define ICMP_ECHOREPLY 0
#define ICMP_ECHO 8

void handle_icmp(uint16_t len) {
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
	enc_tx_write_buf(our_ip, 4);

	enc_tx_stop();
	enc_tx_checksum_ipv4();

	enc_tx_do(IPV4_HEADER_LENGTH + len, ETHERTYPE_IPV4); /* FIXME dat api */

	return;
ignore:
	debug_fstr("ICMP IGNORED\n");
	enc_rx_stop();
	enc_rx_acknowledge();
}

void handle_ipv4() {
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
	if (memcmp(dstip, our_ip, 4)) {
		/* it's not for us... ok... */
		goto ignore;
	}

	// debug_fstr("for us!\n");

	if (proto == IPPROTO_ICMP) {
		handle_icmp(len);
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

void handle_recv() {
	uint8_t wat[64];

	uint16_t len = enc_rx_accept_packet();
	if (!len) {
		debug_fstr("WTF?\n");
		return;
	}

	/* check the ethertype */
	enc_rx_seek(12);
	enc_rx_start();
	uint16_t ethertype = enc_rx_read_intbe();

	if (ethertype == ETHERTYPE_ARP && len >= ETH_HEADER_LENGTH + 28) {
		handle_arp();
		return;
	} else if (ethertype == ETHERTYPE_IPV4 && len >= ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH) {
		// debug_fstr("IPv4 ");
		handle_ipv4();
		return;
	} else {
		debug_fstr("unknown\nethertype\n");
		debug_hex16(ethertype);
		debug_fstr("\n");

		enc_rx_stop();
		enc_rx_acknowledge();
	}
}

void main() {
	debug_init();

	spi_init();
	enc_init();

	GIMSK |= (1 << INT1);
	sei();

	while (1) {
		if (enc_interrupt) {
			while (enc_rx_has_packet()) {
				handle_recv();
			}
			enc_interrupt = 0;
			GIMSK |= (1 << INT1);
		}

		debug_fstr("good night\n");
		sleep_mode();
	}
}


