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

/* -----------------------------------------------------------------------------------------------------------*/
/*! Eine schnelle MEM->SPI Blocksende Routine mit optimierungen auf Speed.
 * \param	buffer		Zeiger auf den Puffer der gesendet werden soll.
 * \param	Datalenght	Anzahl der Bytes die gesedet werden soll.
 */
/* -----------------------------------------------------------------------------------------------------------*/
void SPI_FastMem2Write( const unsigned char *const buffer, unsigned int Datalenght )
{
	for (int i = 0; i < Datalenght; i++) {
		spi_write(buffer[i]);
	}
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! Eine schnelle SPI->MEM Blockempfangroutine mit optimierungen auf Speed.
 * \warning Auf einigen Controller laufen die Optimierungen nicht richtig. Bitte teil des Sourcecode der dies verursacht ist auskommentiert.
 * \param	buffer		Zeiger auf den Puffer wohin die Daten geschrieben werden sollen.
 * \param	Datalenght	Anzahl der Bytes die empfangen werden sollen.
 */
/* -----------------------------------------------------------------------------------------------------------*/
void SPI_FastRead2Mem( unsigned char * buffer, unsigned int Datalenght )
{
	for (int i = 0; i < Datalenght; i++) {
		buffer[i] = spi_read();
	}
}

