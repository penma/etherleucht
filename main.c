
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdlib.h>

#include "enc28j60.h"

#define DBG_DATA PB0
#define DBG_SHIFT PB1
#define DBG_SYNC PB2
void DebugChar(uint8_t data) {
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

void DebugStr(char *wat) {
	while (*wat != 0) {
		DebugChar(*wat);
		wat++;
	}
}

static void DebugNibble(uint8_t wat) {
	if (wat < 0xa) {
		DebugChar('0' + wat);
	} else {
		DebugChar('a' + wat - 0xa);
	}
}

void DebugHex(uint8_t wat) {
	DebugNibble(wat >> 4);
	DebugNibble(wat & 0xf);
}

void DebugNum(uint16_t wat) {
	if (wat >= 10000) goto n5;
	if (wat >= 1000) goto n4;
	if (wat >= 100) goto n3;
	if (wat >= 10) goto n2;
	goto n1;
	n5: DebugNibble(wat / 10000);
	wat %= 10000;
	n4: DebugNibble(wat / 1000);
	wat %= 1000;
	n3: DebugNibble(wat / 100);
	wat %= 100;
	n2: DebugNibble(wat / 10);
	wat %= 10;
	n1: DebugNibble(wat);
}

ISR(INT1_vect) {
	uint8_t wat[64];
	DebugStr("INT\n");
	if (0 != enc28j60PacketReceive(64, wat)) {
		if (wat[42] == '0') {
			DebugStr("YAY\n");
		}
	}
}

void main() {
	DDRB |= (1 << DBG_DATA) | (1 << DBG_SHIFT) | (1 << DBG_SYNC);
	PORTB &= ~( (1 << DBG_DATA) | (1 << DBG_SHIFT) | (1 << DBG_SYNC) );

	DebugStr("* O HAI *\n");

	spi_init();
	enc28j60Init();

	GIMSK |= (1 << INT1);
	sei();

	uint8_t WAT = 0;
	while (1) {
		uint8_t wat[64];
		
		wat[0] = wat[1] = wat[2] = wat[3] = wat[4] = wat[5] = 0xff;
		wat[6] = 0x42;
		wat[7] = 0xcc;
		wat[8] = 0xcd;
		wat[9] = 0x12;
		wat[10] = 0x34;
		wat[11] = 0x56;

		wat[12] = 0x08;
		wat[13] = 0x00;

		wat[14] = 0x45;
		wat[15] = 0x00;
		wat[16] = 0x00;
		wat[17] = 0x20;
		wat[18] = 0x23;
		wat[19] = 0x42;
		wat[20] = wat[21] = 0;
		wat[22] = 0xff;
		wat[23] = 0x11;
		wat[24] = 0x17;
		wat[25] = 0x31;

		wat[26] = wat[30] = 172;
		wat[27] = wat[31] = 22;
		wat[28] = wat[32] = 26;
		wat[29] = 224;
		wat[33] = 15;

		for (int i = 0; i < 10; i++) {
			wat[34 + i] = WAT + i;
		}
		wat[44] = 0;
		wat[45] = 0;

		DebugStr("send pkg\n");
		ATOMIC_BLOCK(ATOMIC_FORCEON) {
			enc28j60PacketSend(46, wat);
		}
		_delay_ms(1000);

	}
}


