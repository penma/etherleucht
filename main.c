
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdlib.h>
#include <string.h>

#include "spi.h"
#include "networking.h"
#include "debug.h"

volatile uint8_t enc_interrupt = 0;

static uint8_t our_ip[4] = { 192, 168, 0, 250 };

ISR(INT1_vect) {
	/* record that we shall handle a packet
	 * then, disable this interrupt to prevent the enc28j60 from
	 * terrorpoking us
	 */
	enc_interrupt = 1;
	GIMSK &= ~(1 << INT1);
}

void handle_arp() {
	if (enc_rx_read_intbe() != 1) {
		/* not ethernet */
		goto ignore;
	}

	if (enc_rx_read_intbe() != 0x0800) {
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

	if (enc_rx_read_intbe() != 1) {
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
	enc_tx_seek(14);
	enc_tx_start();

	enc_tx_write_intbe(1);
	enc_tx_write_intbe(0x0800);
	enc_tx_write_byte(6);
	enc_tx_write_byte(4);
	enc_tx_write_intbe(2);

	extern uint8_t mac[6];
	enc_tx_write_buf(mac, 6);
	enc_tx_write_buf(our_ip, 4);
	enc_tx_write_buf(sender_hw, 6);
	enc_tx_write_buf(sender_proto, 4);

	enc_tx_stop();
	enc_tx_do(28, 0x0806);

	return;
ignore:
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

	if (ethertype == 0x0806 && len >= 28 + 14) {
		handle_arp();
		return;
	} else {
		debug_fstr("unknown\nethertype\n");
		debug_hex16(ethertype);
		debug_fstr("\n");

		enc_rx_stop();
	}

	if (len > 64) {
		enc_rx_acknowledge();
	} else {
		enc_rx_seek(0);
		enc_rx_start();
		enc_rx_read_buf(wat, len);
		enc_rx_stop();

		enc_rx_acknowledge();

		/*
		for (int y = 0; y < len / 6; y++) {
			for (int x = 0; x < 6; x++) {
				debug_hex8(wat[6*y + x]);
			}
			debug_str("\n");
		}
		*/

		if (wat[42] == '0') {
			debug_fstr("*** YAY ***\n");
		}
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

		sleep_mode();
	}
}


