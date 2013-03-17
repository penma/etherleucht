
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdlib.h>
#include <string.h>

#include "spi.h"
#include "networking.h"
#include "ethernet.h"
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
	uint16_t len = enc_rx_accept_packet();
	if (!len) {
		debug_fstr("WTF?\n");
		return;
	}

	eth_handle(len);
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


