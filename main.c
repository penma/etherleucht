
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>

#include "enc28j60.h"

uint16_t dbga = 0;
uint16_t dbgb = 0;
uint16_t dbgc = 0;
uint16_t dbgd = 0;

void main() {
	enc28j60Init();

	DDRD |= (1 << PD2);
	DDRA |= (1 << PA0);

	PORTA |= (1 << PA0);

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
		wat[44] = dbga >> 8;
		wat[45] = dbga;

		/* = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0x42, 0xcc, 0xcd, 0x84, 0xf4, 0x7c,
		0x08, 0x00,
//		0x45, 0x00, 0x00, 0x20, 0x23, 0x42, 0x00, 0x00, 0xff, 0x11, 0x17, 0x31,
//		0xc0, 0xa8, 0x00, 0x02, 0xc0, 0xa8, 0x00, 0x07,
//		0x90, 0x01, 0x09, 0x26, 0x00, 0x0c, 0x00, 0x00, 0x7a, 0x6f, 0x6d, 0x67
		0xaa, 0x55,
		0xaa, 0x55,
		0xaa, 0x55,
		0xaa, 0x55,
		0xaa, 0x55,
		0xaa, 0x55,
		0xaa, 0x55,
		0xaa, 0x55,
		0xaa, 0x55,
		0xaa, 0x55,
		0xaa, 0x55,
		0xaa, 0x55,
		0xaa, 0x55,
		0xaa, 0x55,
		0xaa, 0x55,
		0xaa, 0x55,
		};

/*		wat[34] = wat[35] = 0xff;

		wat[36] = dbga >> 8;
		wat[38] = dbgb >> 8;
		wat[40] = dbgc >> 8;
		wat[42] = dbgd >> 8;

		wat[37] = (dbga & 0xff);
		wat[39] = (dbgb & 0xff);
		wat[41] = (dbgc & 0xff);
		wat[43] = (dbgd & 0xff);

		wat[44] = wat[45] = 0xff;

		wat[33] = _SFR_IO8(0x3d);
*/

		enc28j60PacketSend(46, wat);
		_delay_ms(900);
		PORTD ^= (1 << PD2);
		_delay_ms(100);
		PORTD ^= (1 << PD2);

		while (0 != enc28j60PacketReceive(64, wat)) {
			_delay_ms(50);
			PORTA ^= (1 << PA0);
			_delay_ms(10);
			PORTA ^= (1 << PA0);
#define NN 20
			if (wat[42] == '0') {
				PORTD ^= (1 << PD2);
				WAT++;
			}
		}
	}
}

