/*! \file enc28j60.c \brief Microchip ENC28J60 Ethernet Interface Driver. */
//*****************************************************************************
//
// File Name	: 'enc28j60.c'
// Title		: Microchip ENC28J60 Ethernet Interface Driver
// Author		: Pascal Stang (c)2005
// Created		: 9/22/2005
// Revised		: 9/22/2005
// Version		: 0.1
// Target MCU	: Atmel AVR series
// Editor Tabs	: 4
//
// Description	: This driver provides initialization and transmit/receive
//	functions for the Microchip ENC28J60 10Mb Ethernet Controller and PHY.
// This chip is novel in that it is a full MAC+PHY interface all in a 28-pin
// chip, using an SPI interface to the host processor.
//
//*****************************************************************************
  //#include <avr/io.h>
  //#include <avr/interrupt.h>
#include "enc28j60.h"
#include "spi.h"
#include <util/delay.h>  

#include "debug.h"

static uint16_t NextPacketPtr;
static uint8_t REVID;

static void enc_spi_init() {
	DDRA |= (1 << PA1);
	enc_spi_select(0);
}

void enc_spi_select(uint8_t act) {
	if (act) {
		PORTA &= ~(1 << PA1);
	} else {
		PORTA |= (1 << PA1);
	}
}

static uint8_t enc_op_read(uint8_t op, uint8_t address) {
	enc_spi_select(1);

	spi_write(op | (address & ADDR_MASK));
	if (address & SPRD_MASK) {
		spi_read();
	}
	uint8_t data = spi_read();

	enc_spi_select(0);
	return data;
}

static void enc_op_write(uint8_t op, uint8_t address, uint8_t data) {
	enc_spi_select(1);

	spi_write(op | (address & ADDR_MASK));
	spi_write(data);

	enc_spi_select(0);
}

//*********************************************************************************************************
//
// Buffer einlesen
//
//*********************************************************************************************************
static void enc28j60ReadBuffer(uint16_t len, uint8_t *data){
	enc_spi_select(1);

	spi_write(ENC28J60_READ_BUF_MEM);
	SPI_FastRead2Mem(data, len);

	enc_spi_select(0);
}

//*********************************************************************************************************
//
// Buffer schreiben
//
//*********************************************************************************************************
static void enc28j60WriteBuffer(uint16_t len, const uint8_t *const data) {
	enc_spi_select(1);

	spi_write(ENC28J60_WRITE_BUF_MEM);
	SPI_FastMem2Write(data, len);

	enc_spi_select(0);
}

//*********************************************************************************************************
//
// 
//
//*********************************************************************************************************
static void enc_set_bank(uint8_t address) {
	// set the bank (if needed)
	static uint8_t bank = 0;
	if ((BANK_MASK & address) != bank) {
		enc_op_write(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1 | ECON1_BSEL0));
		enc_op_write(ENC28J60_BIT_FIELD_SET, ECON1, (BANK_MASK & address) >> 5);
		bank = (BANK_MASK & address);
	}
}

//*********************************************************************************************************
//
// 
//
//*********************************************************************************************************
static uint8_t enc_reg_read(uint8_t address) {
	enc_set_bank(address);
	return enc_op_read(ENC28J60_READ_CTRL_REG, address);
}

//*********************************************************************************************************
//
// 
//
//*********************************************************************************************************
static void enc_reg_write(uint8_t address, uint8_t data) {
	enc_set_bank(address);
	enc_op_write(ENC28J60_WRITE_CTRL_REG, address, data);
}

static void enc_reg_write16(uint8_t baseaddr, uint16_t data) {
	enc_reg_write(baseaddr      , data & 0xff);
	enc_reg_write(baseaddr | 0x1, data >> 8);
}

static uint16_t enc_reg_read16(uint8_t baseaddr) {
	uint16_t data;
	data = enc_reg_read(baseaddr);
	data |= enc_reg_read(baseaddr | 0x1) << 8;
	return data;
}

static uint16_t enc_phy_read(uint8_t address) {
	// Set the right address and start the register read operation
	enc_reg_write(MIREGADR, address);
	enc_reg_write(MICMD, MICMD_MIIRD);

	// wait until the PHY read completes
	while(enc_reg_read(MISTAT) & MISTAT_BUSY) { }

	// quit reading
	enc_reg_write(MICMD, 0x00);

	return enc_reg_read16(MIRDL);
}

static void enc_phy_write(uint8_t address, uint16_t data) {
	// set the PHY register address
	enc_reg_write(MIREGADR, address);

	// write the PHY data
	enc_reg_write16(MIWRL, data);

	// wait until the PHY write completes
	while (enc_reg_read(MISTAT) & MISTAT_BUSY) { }
}

void enc_init() {
	enc_spi_init();

	/* reset */
	enc_op_write(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
	_delay_us(80);
	/* wait for reset to complete
	 * XXX is this reliable enough in case of total communication failure?
	 */
	while (!(enc_reg_read(ESTAT) & ESTAT_CLKRDY)) { }

	REVID = enc_reg_read(EREVID);

	/* Bank 0 stuff
	 * Ethernet buffer addresses
	 */
	NextPacketPtr = RXST;
	enc_reg_write16(ERXSTL, RXST);
	enc_reg_write16(ERXNDL, RXND);
	enc_reg_write16(ERXRDPTL, RXND);
	enc_reg_write16(ETXSTL, TXSTART_INIT);

	/* Bank 2 stuff
	 * setup MAC, automatic padding, auto CRC
	 */
	enc_reg_write(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	enc_reg_write(MACON2, 0x00);
	enc_op_write(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN);

	/* inter-frame gap non-B2B, B2B, maximum packet size */
	enc_reg_write16(MAIPGL, 0x0c12);
	enc_reg_write(MABBIPG, 0x12);
	enc_reg_write16(MAMXFLL, MAX_FRAMELEN);

	/* Bank 3 stuff
	 * MAC address for Unicast packet filtering
	 */
	enc_reg_write(MAADR5, 0x56);
	enc_reg_write(MAADR4, 0x34);
	enc_reg_write(MAADR3, 0x12);
	enc_reg_write(MAADR2, 0xcd);
	enc_reg_write(MAADR1, 0xcc);
	enc_reg_write(MAADR0, 0x42);

	/* no loopback of transmitted frames */
	enc_phy_write(PHCON2, PHCON2_HDLDIS);

	/* Enable interrupts and enable packet reception */
	enc_set_bank(ECON1);
	enc_op_write(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
	enc_op_write(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
	/* LEDs
	 * A: Link status + receive
	 * B: Transmit
	 * long stretched
	 */
	enc_phy_write(PHLCON, 0xc1a);
}

void enc28j60PacketSend(uint16_t len, const uint8_t *const packet) {
	// Set the write pointer to start of transmit buffer area
	enc_reg_write16(EWRPTL, TXSTART_INIT);

	// Set the TXND pointer to correspond to the packet size given
	enc_reg_write16(ETXNDL, TXSTART_INIT + len);

	// write per-packet control byte
	enc_op_write(ENC28J60_WRITE_BUF_MEM, 0, 0x00);

	// copy the packet into the transmit buffer
	enc28j60WriteBuffer(len, packet);

	// send the contents of the transmit buffer onto the network
	enc_op_write(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);

}

void enc_acknowledge_packet() {
	/* errata B7 #12: ERXRDPT must contain an odd value */
	if (NextPacketPtr == RXST) {
		enc_reg_write16(ERXRDPTL, RXND);
	} else {
		enc_reg_write16(ERXRDPTL, NextPacketPtr - 1);
	}

	enc_op_write(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
}

uint16_t enc28j60PacketReceive(uint16_t maxlen, uint8_t *packet) {
	uint16_t rxstat;
	uint16_t len;

	// check if a packet has been received and buffered
	if (!(enc_reg_read(EIR) & EIR_PKTIF)) {
		if (enc_reg_read(EPKTCNT) == 0) {
			debug_str("!R\n");
			return 0;
		}
	}

	debug_str("P");
	debug_dec16(enc_reg_read(EPKTCNT));
	debug_str(" R");
	debug_hex16(NextPacketPtr);

	// Make absolutely certain that any previous packet was discarded	
	//if( WasDiscarded == FALSE)
	//	MACDiscardRx();

	// Set the read pointer to the start of the received packet
	enc_reg_write16(ERDPTL, NextPacketPtr);

	// read the next packet pointer
	NextPacketPtr  = enc_op_read(ENC28J60_READ_BUF_MEM, 0);
	NextPacketPtr |= enc_op_read(ENC28J60_READ_BUF_MEM, 0)<<8;

	debug_str("\n>");
	debug_hex16(NextPacketPtr);

	// read the packet length
	len  = enc_op_read(ENC28J60_READ_BUF_MEM, 0);
	len |= enc_op_read(ENC28J60_READ_BUF_MEM, 0)<<8;

	debug_str("+");
	debug_hex16(len);
	debug_str("\n");
	// remove CRC from len (we don't read the CRC from
	// the receive buffer
	len -= 4;

	// read the receive status
	rxstat  = enc_op_read(ENC28J60_READ_BUF_MEM, 0);
	rxstat |= enc_op_read(ENC28J60_READ_BUF_MEM, 0)<<8;

	// limit retrieve length
	// len = MIN(len, maxlen);
	// When len bigger than maxlen, ignore the packet und read next packetptr
	if (len > maxlen) {
		enc_acknowledge_packet();
		return(0);
	}
	// copy the packet without CRC from the receive buffer
	enc28j60ReadBuffer(len, packet);

	// decrement the packet counter indicate we are done with this packet
	enc_acknowledge_packet();

	return len;
}

