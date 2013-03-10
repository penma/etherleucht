
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdlib.h>

#include "spi.h"
#include "enc28j60.h"
#include "debug.h"

ISR(INT1_vect) {
	uint8_t wat[64];

	uint16_t len = enc_rx_has_packet();
	if (len) {
		if (len > 64) {
			enc_acknowledge_packet();
		} else {
			enc_buf_read_seek(0);
			enc_buf_read_start();
			enc_buf_read_bulk(wat, len);
			enc_buf_read_stop();

			enc_acknowledge_packet();

			for (int y = 0; y < len / 6; y++) {
				for (int x = 0; x < 6; x++) {
					debug_hex8(wat[6*y + x]);
				}
				debug_str("\n");
			}

			if (wat[42] == '0') {
				debug_str("*** YAY ***\n");
			}
		}
	}
}

void main() {
	debug_init();

	spi_init();
	enc_init();

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

		debug_str("send pkg\n");
		ATOMIC_BLOCK(ATOMIC_FORCEON) {
			enc28j60PacketSend(46, wat);
		}
		_delay_ms(1000);

	}
}


