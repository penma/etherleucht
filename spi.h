#ifndef _SPI_H
#define _SPI_H

#include <avr/io.h>

void spi_init();
uint8_t spi_readwrite(uint8_t out);
uint8_t spi_read();
void spi_write(uint8_t out);
void SPI_FastRead2Mem( unsigned char * buffer, unsigned int Datalenght );
void SPI_FastMem2Write( const unsigned char *const buffer, unsigned int Datalenght );

#endif
