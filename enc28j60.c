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

void enc_spi_init() {
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

//*********************************************************************************************************
//
// Sende Rad Command
//
//*********************************************************************************************************
static uint8_t enc28j60ReadOp(uint8_t op, uint8_t address) {
	enc_spi_select(1);

	spi_write(op | (address & ADDR_MASK));
	if (address & SPRD_MASK) {
		spi_read();
	}
	uint8_t data = spi_read();

	enc_spi_select(0);
	return data;
}

//*********************************************************************************************************
//
// Sende Write Command
//
//*********************************************************************************************************
static void enc28j60WriteOp(uint8_t op, uint8_t address, uint8_t data) {
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
static void enc28j60SetBank(uint8_t address) {
	// set the bank (if needed)
	static uint8_t bank = 0;
	if ((BANK_MASK & address) != bank) {
		enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1 | ECON1_BSEL0));
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, (BANK_MASK & address) >> 5);
		bank = (BANK_MASK & address);
	}
}

//*********************************************************************************************************
//
// 
//
//*********************************************************************************************************
static uint8_t enc28j60Read(uint8_t address) {
	enc28j60SetBank(address);
	return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
}

//*********************************************************************************************************
//
// 
//
//*********************************************************************************************************
static void enc28j60Write(uint8_t address, uint8_t data) {
	enc28j60SetBank(address);
	enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

static void enc28j60Write16(uint8_t baseaddr, uint16_t data) {
	enc28j60Write(baseaddr      , data & 0xff);
	enc28j60Write(baseaddr | 0x1, data >> 8);
}

static uint16_t enc28j60Read16(uint8_t baseaddr) {
	uint16_t data;
	data = enc28j60Read(baseaddr);
	data |= enc28j60Read(baseaddr | 0x1) << 8;
	return data;
}

//*********************************************************************************************************
//
// 
//
//*********************************************************************************************************
static uint16_t enc28j60PhyRead(uint8_t address) {
	uint16_t data;

	// Set the right address and start the register read operation
	enc28j60Write(MIREGADR, address);
	enc28j60Write(MICMD, MICMD_MIIRD);

	// wait until the PHY read completes
	while(enc28j60Read(MISTAT) & MISTAT_BUSY);

	// quit reading
	enc28j60Write(MICMD, 0x00);

	// get data value
	data  = enc28j60Read(MIRDL);
	data |= enc28j60Read(MIRDH) << 8;

	return data;
}

//*********************************************************************************************************
//
// 
//
//*********************************************************************************************************
static void enc28j60PhyWrite(uint8_t address, uint16_t data) {
	// set the PHY register address
	enc28j60Write(MIREGADR, address);

	// write the PHY data
	enc28j60Write16(MIWRL, data);

	// wait until the PHY write completes
	while (enc28j60Read(MISTAT) & MISTAT_BUSY) {
	}
}

//*********************************************************************************************************
//
// Initialiesiert den ENC28J60
//
//*********************************************************************************************************
void enc28j60Init() {
	enc_spi_init();

	// perform system reset
	enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
	_delay_us(80);
	/* wait for reset to complete
	 * XXX is this reliable enough in case of total communication failure?
	 */
	while (!(enc28j60Read(ESTAT) & ESTAT_CLKRDY)) {
	}

	REVID = enc28j60Read(EREVID);

	// do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers, must write low byte first
	// set receive buffer start address
	NextPacketPtr = RXSTART_INIT;
	enc28j60Write16(ERXSTL, RXSTART_INIT);
	// set receive pointer address
	enc28j60Write16(ERXNDL, RXSTOP_INIT - 1);
	// enc28j60Write16(ERXRDPTL, RXSTART_INIT);
	enc28j60Write16(ERXRDPTL, RXSTOP_INIT - 1);
	// set receive buffer end
	// ERXND defaults to 0x1FFF (end of ram)
	// set transmit buffer start
	// ETXST defaults to 0x0000 (beginnging of ram)
	enc28j60Write16(ETXSTL, TXSTART_INIT);

	// do bank 2 stuff
	// enable MAC receive
	enc28j60Write(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	// bring MAC out of reset
	enc28j60Write(MACON2, 0x00);
	// enable automatic padding and CRC operations
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);

	// set inter-frame gap (non-back-to-back)
	enc28j60Write16(MAIPGL, 0x0c12);
	// set inter-frame gap (back-to-back)
	enc28j60Write(MABBIPG, 0x12);
	// Set the maximum packet size which the controller will accept
	enc28j60Write16(MAMXFLL, MAX_FRAMELEN);

	// do bank 3 stuff
	// write MAC address
	// NOTE: MAC address in ENC28J60 is byte-backward
	enc28j60Write(MAADR5, 0x56);
	enc28j60Write(MAADR4, 0x34);
	enc28j60Write(MAADR3, 0x12);
	enc28j60Write(MAADR2, 0xcd);
	enc28j60Write(MAADR1, 0xcc);
	enc28j60Write(MAADR0, 0x42);

	// no loopback of transmitted frames
	enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);

	// switch to bank 0
	enc28j60SetBank(ECON1);
	// enable interrutps
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
	// enable packet reception
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
	// LED initialization
	enc28j60PhyWrite(0x14,0x0420);
}

//*********************************************************************************************************
//
// Sendet ein Packet
//
//*********************************************************************************************************
void enc28j60PacketSend(uint16_t len, const uint8_t *const packet) {
	// Set the write pointer to start of transmit buffer area
	enc28j60Write16(EWRPTL, TXSTART_INIT);

	// Set the TXND pointer to correspond to the packet size given
	enc28j60Write16(ETXNDL, TXSTART_INIT + len);

	// write per-packet control byte
	enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);

	// copy the packet into the transmit buffer
	enc28j60WriteBuffer(len, packet);

	// send the contents of the transmit buffer onto the network
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);

}

//*********************************************************************************************************
//
// Hole ein empfangendes Packet
//
//*********************************************************************************************************


void enc_acknowledge_packet() {
	enc28j60Write16(ERXRDPTL, NextPacketPtr - 1);
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
}


uint16_t enc28j60PacketReceive(uint16_t maxlen, uint8_t *packet) {
	uint16_t rxstat, rs, re;
	uint16_t len;

	// check if a packet has been received and buffered
	if (!(enc28j60Read(EIR) & EIR_PKTIF)) {
		if (enc28j60Read(EPKTCNT) == 0) {
			debug_str("!R\n");
			return 0;
		}
	}

	debug_str("P");
	debug_dec16(enc28j60Read(EPKTCNT));
	debug_str(" R");
	debug_hex16(NextPacketPtr);

	// Make absolutely certain that any previous packet was discarded	
	//if( WasDiscarded == FALSE)
	//	MACDiscardRx();

	// Set the read pointer to the start of the received packet
	enc28j60Write16(ERDPTL, NextPacketPtr);

	// read the next packet pointer
	NextPacketPtr  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	NextPacketPtr |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;

	// read the packet length
	len  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	len |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;

	debug_str("+");
	debug_hex16(len);
	debug_str("\n");
	// remove CRC from len (we don't read the CRC from
	// the receive buffer
	len -= 4;

	// read the receive status
	rxstat  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	rxstat |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;

	// limit retrieve length
	// len = MIN(len, maxlen);
	// When len bigger than maxlen, ignore the packet und read next packetptr
	if (len > maxlen) {
		enc_acknowledge_packet();
		return(0);
	}
	// copy the packet without CRC from the receive buffer
	enc28j60ReadBuffer(len, packet);

	// an implementation of Errata B1 Section #13
	rs = enc28j60Read16(ERXSTL);
	re = enc28j60Read16(ERXNDL);
	if (NextPacketPtr - 1 < rs || NextPacketPtr - 1 > re) {
		enc28j60Write16(ERXRDPTL, re - 1);
	} else {
		enc28j60Write16(ERXRDPTL, NextPacketPtr - 1);
	}

	// decrement the packet counter indicate we are done with this packet
	uint16_t wtf = enc28j60Read16(ERXRDPTL);
	if (wtf != NextPacketPtr - 1) {
		for (int i = 0; i < 4; i++) {
		debug_str("exp ");
		debug_hex16(wtf);
		debug_str("\ngot ");
		debug_hex16(NextPacketPtr - 1);
		debug_str("\n");
		}
		while (1) {}
	}
	enc_acknowledge_packet();

	return len;
}

