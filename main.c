
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

	while (1) {
		uint8_t wat[32];

		wat[ 0] = 0x45;
		wat[ 1] = 0x00;
		wat[ 2] = 0x00;
		wat[ 3] = 0x20;
		wat[ 4] = 0x23;
		wat[ 5] = 0x42;
		wat[ 6] = wat[7] = 0;
		wat[ 8] = 0xff;
		wat[ 9] = 0x11;
		wat[10] = 0x00;
		wat[11] = 0x00;

		wat[12] = wat[16] = 172;
		wat[13] = wat[17] = 22;
		wat[14] = wat[18] = 26;
		wat[15] = 224;
		wat[19] = 15;

		wat[20] = wat[21] = wat[22] = wat[23] = wat[24] = wat[25] = wat[26] = wat[27] = 0;

		wat[28] = 'w';
		wat[29] = 'a';
		wat[30] = 't';
		wat[31] = '!';


		debug_str("send pkg\n");
		ATOMIC_BLOCK(ATOMIC_FORCEON) {
			enc_tx_seek(14);
			enc_tx_start();

			enc_tx_write_buf(wat, 20 + 8 + 4);

			enc_tx_stop();
			enc_tx_checksum_ipv4();
			enc_tx_do(20 + 8 + 4, 0x0800, 0);
		}
		_delay_ms(1000);

	}
}


