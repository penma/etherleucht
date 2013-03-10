#include <avr/io.h>
#include <util/delay.h>

#include "spi.h"

void spi_init() {
	XCK_DDR |= (1 << XCK_BIT);
	TXD_DDR |= (1 << TXD_BIT);

	UCSRC = (3 << UMSEL);
	UCSRB = (1 << RXEN) | (1 << TXEN);
	UBRRL = 1;
}

uint8_t spi_readwrite(uint8_t out) {
	while (!(UCSRA & (1 << UDRE))) { }
	UDR = out;

	while (!(UCSRA & (1 << RXC))) { }
	return UDR;
}

void spi_write(uint8_t out) {
	spi_readwrite(out);
}

uint8_t spi_read() {
	/* a dummy byte has to be written */
	return spi_readwrite(0);
}

