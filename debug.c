
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "debug.h"

#define DBG_DATA PB0
#define DBG_SHIFT PB1
#define DBG_SYNC PB2

void debug_init() {
	DDRB |= (1 << DBG_DATA) | (1 << DBG_SHIFT) | (1 << DBG_SYNC);
	PORTB &= ~( (1 << DBG_DATA) | (1 << DBG_SHIFT) | (1 << DBG_SYNC) );

	debug_str("* O HAI *\n");
}

void debug_char(uint8_t data) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		PORTB |= (1 << DBG_SYNC);
		__builtin_avr_delay_cycles(20);
		PORTB &= ~(1 << DBG_SYNC);
		__builtin_avr_delay_cycles(20);

		for (int i = 8; i > 0; i--) {
			if (data & (1 << (i-1))) {
				PORTB |= (1 << DBG_DATA);
			} else {
				PORTB &= ~(1 << DBG_DATA);
			}
			PORTB |= (1 << DBG_SHIFT);
			__builtin_avr_delay_cycles(20);
			PORTB &= ~(1 << DBG_SHIFT);
			__builtin_avr_delay_cycles(20);
		}

		_delay_ms(10);
	}
}

void debug_str(char *wat) {
	while (*wat != 0) {
		debug_char(*wat);
		wat++;
	}
}

static void debug_nibble(uint8_t wat) {
	if (wat < 0xa) {
		debug_char('0' + wat);
	} else {
		debug_char('a' + wat - 0xa);
	}
}

void debug_hex8(uint8_t wat) {
	debug_nibble(wat >> 4);
	debug_nibble(wat & 0xf);
}

void debug_hex16(uint16_t wat) {
	debug_hex8(wat >> 8);
	debug_hex8(wat & 0xff);
}

void debug_dec16(uint16_t wat) {
	if (wat >= 10000) goto n5;
	if (wat >= 1000) goto n4;
	if (wat >= 100) goto n3;
	if (wat >= 10) goto n2;
	goto n1;
	n5: debug_nibble(wat / 10000);
	wat %= 10000;
	n4: debug_nibble(wat / 1000);
	wat %= 1000;
	n3: debug_nibble(wat / 100);
	wat %= 100;
	n2: debug_nibble(wat / 10);
	wat %= 10;
	n1: debug_nibble(wat);
}

