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

#define SW1 PB4
#define SW2 PD5

unsigned char SPI_InitState = 0;

#define DBG_ST PD0
#define DBG_SH PD1
#define DBG_DS PD4
void led_debug(unsigned char wat) {
	DDRD |= (1 << DBG_ST) | (1 << DBG_SH) | (1 << DBG_DS);
	PORTD &= ~( (1 << DBG_ST) | (1 << DBG_SH) );

	for (int i = 8; i > 0; i--) {
		if (wat & (1 << (i-1))) {
			PORTD |= (1 << DBG_DS);
		} else {
			PORTD &= ~(1 << DBG_DS);
		}
		PORTD |= (1 << DBG_SH);
		__builtin_avr_delay_cycles(10);
		PORTD &= ~(1 << DBG_SH);
		__builtin_avr_delay_cycles(10);
	}

	PORTD |= (1 << DBG_ST);
	__builtin_avr_delay_cycles(10);
	_delay_ms(1000);
}


 
/* -----------------------------------------------------------------------------------------------------------*/
/*! Die Init fuer dir SPI-Schnittstelle. Es koennen verschiedene Geschwindigkeiten eingestellt werden.
 * \param 	Options		Hier kann die Geschwindigkeit der SPI eingestellt werden.
 * \retval	Null wenn alles richtig eingestellt wurde.
 */
/* -----------------------------------------------------------------------------------------------------------*/
unsigned int SPI_init( unsigned int Options )
{
	DDRD &= ~(1 << PD3); /* INT */
	DDRD |= (1 << SW2)  | (1 << SS);

	DDRB &= ~(1 << MISO);
	DDRB |= (1 << MOSI) | (1 << SCK) | (1 << SW1);

	SPI_InitState = 1;

	return( 0 );
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! Schreibt einen Wert auf den SPI-Bus. Gleichzeitig wird ein Wert von diesem im Takt eingelesen.
 * \warning	Auf den SPI-Bus sollte vorher per Chip-select ein Baustein ausgewaehlt werden. Dies geschied nicht in der SPI-Routine sondern
 * muss von der Aufrufenden Funktion gemacht werden.
 * \param 	Data	Der Wert der uebertragen werden soll.
 * \retval  Data	Der wert der gleichzeit empfangen wurde.
 */
/* -----------------------------------------------------------------------------------------------------------*/

#define PROGDELAY 10
unsigned char SPI_ReadWrite( unsigned char Data )
{

	unsigned char Out = 0;
	for (int i = 8; i > 0; i--) {
		if (Data & (1 << (i-1))) {
			PORTB |= (1 << MOSI);
		} else {
			PORTB &= ~(1 << MOSI);
		}
		PORTB |= (1 << SCK);
		__builtin_avr_delay_cycles(PROGDELAY);
		Out |= (PINB & (1 << MISO) ? 1 : 0) << (i-1);
		PORTB &= ~(1 << SCK);
		__builtin_avr_delay_cycles(PROGDELAY);
	}
//	led_debug(Out);
	return Out;
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! Eine schnelle MEM->SPI Blocksende Routine mit optimierungen auf Speed.
 * \param	buffer		Zeiger auf den Puffer der gesendet werden soll.
 * \param	Datalenght	Anzahl der Bytes die gesedet werden soll.
 */
/* -----------------------------------------------------------------------------------------------------------*/
void SPI_FastMem2Write( unsigned char * buffer, unsigned int Datalenght )
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

/* -----------------------------------------------------------------------------------------------------------*/
/*! Hier wird der InitStatus abgefragt um zu sehen ob die Schnittstelle schon Eingestellt worden ist.
 * \retval  Status
 */
/* -----------------------------------------------------------------------------------------------------------*/
unsigned char SPI_GetInitState( void )
{
	return( SPI_InitState );
}

//@}

void SPI_Active(int act) {
	if (act) {
		PORTD &= ~(1 << SS);
	} else {
		PORTD |= (1 << SS);
	}
}
