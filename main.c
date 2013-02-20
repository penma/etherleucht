#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>

int main (void) {
	enc28j60Init();

	while (1) {
		PORTD |= (1 << PD2);
		enc28j60PacketSend(46, "\xff\xff\xff\xff\xff\xff\x42\xcc\xcd\x84\xf4\x7c\x08\x00\x45\x00\x00\x20\x23\x42\x00\x00\xff\x11\x17\x31\xc0\xa8\x00\x02\xc0\xa8\x00\x07\x90\x01\x09\x26\x00\x0c\x00\x00\x7a\x6f\x6d\x67");
		_delay_ms(100);
		PORTD &= ~(1 << PD2);
		_delay_ms(900);

	}
}
