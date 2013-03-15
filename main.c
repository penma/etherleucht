
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdlib.h>
#include <string.h>

#include "spi.h"
#include "networking.h"
#include "ipv4.h"
#include "arp.h"
#include "debug.h"

volatile uint8_t enc_interrupt = 0;

ISR(INT1_vect) {
	/* record that we shall handle a packet
	 * then, disable this interrupt to prevent the enc28j60 from
	 * terrorpoking us
	 */
	enc_interrupt = 1;
	GIMSK &= ~(1 << INT1);
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
		arp_handle();
		return;
	} else if (ethertype == ETHERTYPE_IPV4 && len >= ETH_HEADER_LENGTH + IPV4_HEADER_LENGTH) {
		// debug_fstr("IPv4 ");
		ipv4_handle();
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


