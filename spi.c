/*!\file spi.c \brief Stellt die SPI-Schnittstelle bereit */
//***************************************************************************
//*            spi.c
//*
//*  Mon Jul 31 21:46:47 2006
//*  Copyright  2006  User
//*  Email
///	\ingroup hardware
///	\defgroup SPI Die SPI-Schnittstelle (spi.c)
///	\code #include "spi.h" \endcode
///	\par Uebersicht
///		Die SPI-Schnittstelle fuer den AVR-Controller
//****************************************************************************/
//@{
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "spi.h"
#include <avr/io.h>
#include <util/delay.h>

 
/* -----------------------------------------------------------------------------------------------------------*/
/*! Die Init fuer dir SPI-Schnittstelle. Es koennen verschiedene Geschwindigkeiten eingestellt werden.
 * \param 	Options		Hier kann die Geschwindigkeit der SPI eingestellt werden.
 * \retval	Null wenn alles richtig eingestellt wurde.
 */
/* -----------------------------------------------------------------------------------------------------------*/
void SPI_init()
{
	DDRD &= ~(1 << PD3); /* INT */
	DDRA |= (1 << SS);

	XCK_DDR |= (1 << XCK_BIT);
	TXD_DDR |= (1 << TXD_BIT);

	UCSRC = (3 << UMSEL);
	UCSRB = (1 << RXEN) | (1 << TXEN);
	UBRRL = 1;
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! Schreibt einen Wert auf den SPI-Bus. Gleichzeitig wird ein Wert von diesem im Takt eingelesen.
 * \warning	Auf den SPI-Bus sollte vorher per Chip-select ein Baustein ausgewaehlt werden. Dies geschied nicht in der SPI-Routine sondern
 * muss von der Aufrufenden Funktion gemacht werden.
 * \param 	Data	Der Wert der uebertragen werden soll.
 * \retval  Data	Der wert der gleichzeit empfangen wurde.
 */
/* -----------------------------------------------------------------------------------------------------------*/

unsigned char SPI_ReadWrite( unsigned char Data )
{
	while (!(UCSRA & (1 << UDRE))) {
	}

	UDR = Data;

	while (!(UCSRA & (1 << RXC))) {
	}

	return UDR;
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
		SPI_ReadWrite(buffer[i]);
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
		buffer[i] = SPI_ReadWrite(0);
	}
}

//@}

void SPI_Active(uint8_t act) {
	if (act) {
		PORTA &= ~(1 << SS);
	} else {
		PORTA |= (1 << SS);
	}
}
