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

unsigned char Enc28j60Bank;
unsigned int NextPacketPtr;
unsigned char REVID;


//*********************************************************************************************************
//
// Sende Rad Command
//
//*********************************************************************************************************
unsigned char enc28j60ReadOp(unsigned char op, unsigned char address) {
	SPI_Active(1);

	SPI_ReadWrite(op | (address & ADDR_MASK));
	unsigned char data = SPI_ReadWrite(0x00);
	if (address & 0x80) {
		data = SPI_ReadWrite(0x00);
	}

	SPI_Active(0);
	return data;
}

//*********************************************************************************************************
//
// Sende Write Command
//
//*********************************************************************************************************
void enc28j60WriteOp(unsigned char op, unsigned char address, unsigned char data) {
	SPI_Active(1);

	SPI_ReadWrite(op | (address & ADDR_MASK));
	SPI_ReadWrite(data);

	SPI_Active(0);
}

//*********************************************************************************************************
//
// Buffer einlesen
//
//*********************************************************************************************************
void enc28j60ReadBuffer(unsigned int len, unsigned char *data){
	SPI_Active(1);

	SPI_ReadWrite(ENC28J60_READ_BUF_MEM);
	SPI_FastRead2Mem(data, len);

	SPI_Active(0);
}

//*********************************************************************************************************
//
// Buffer schreiben
//
//*********************************************************************************************************
void enc28j60WriteBuffer(unsigned int len, unsigned char *data) {
	SPI_Active(1);

	SPI_ReadWrite(ENC28J60_WRITE_BUF_MEM);
	SPI_FastMem2Write(data, len);

	SPI_Active(0);
}

//*********************************************************************************************************
//
// 
//
//*********************************************************************************************************
void enc28j60SetBank(unsigned char address) {
	// set the bank (if needed)
	if ((address & BANK_MASK) != Enc28j60Bank) {
		enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1 | ECON1_BSEL0));
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK) >> 5);
		Enc28j60Bank = (address & BANK_MASK);
	}
}

//*********************************************************************************************************
//
// 
//
//*********************************************************************************************************
unsigned char enc28j60Read(unsigned char address) {
	enc28j60SetBank(address);
	return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
}

//*********************************************************************************************************
//
// 
//
//*********************************************************************************************************
void enc28j60Write(unsigned char address, unsigned char data) {
	enc28j60SetBank(address);
	enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

void enc28j60Write16(unsigned char baseaddr, uint16_t data) {
	enc28j60Write(baseaddr      , data & 0xff);
	enc28j60Write(baseaddr | 0x1, data >> 8);
}

//*********************************************************************************************************
//
// 
//
//*********************************************************************************************************
unsigned int enc28j60PhyRead(unsigned char address) {
	unsigned int data;

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
void enc28j60PhyWrite(unsigned char address, unsigned int data) {
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
	if (SPI_GetInitState() == SPI_NOT_INIT) {
		SPI_init( SPI_FULL_SPEED);
	}

	SPI_Active(0);

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
	enc28j60Write16(ERXRDPTL, RXSTART_INIT);
	// set receive buffer end
	// ERXND defaults to 0x1FFF (end of ram)
	enc28j60Write16(ERXNDL, RXSTOP_INIT);
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
void enc28j60PacketSend(unsigned int len, unsigned char* packet){
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
unsigned int enc28j60PacketReceive(unsigned int maxlen, unsigned char* packet){
	unsigned int rxstat, rs,re;;
	unsigned int len;

	// check if a packet has been received and buffered
	if( !(enc28j60Read(EIR) & EIR_PKTIF) )
		if (enc28j60Read(EPKTCNT) == 0)
			return 0;

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
	// remove CRC from len (we don't read the CRC from
	// the receive buffer
	len -= 4;

	// read the receive status
	rxstat  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	rxstat |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;

	// limit retrieve length
	// len = MIN(len, maxlen);
	// When len bigger than maxlen, ignore the packet und read next packetptr
	if ( len  > maxlen ){
		enc28j60Write16(ERXRDPTL, NextPacketPtr);
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
		return(0);
	}
	// copy the packet without CRC from the receive buffer
	enc28j60ReadBuffer(len, packet);

	// an implementation of Errata B1 Section #13
	rs = enc28j60Read(ERXSTH);
	rs <<= 8;
	rs |= enc28j60Read(ERXSTL);
	re = enc28j60Read(ERXNDH);
	re <<= 8;
	re |= enc28j60Read(ERXNDL);
	if (NextPacketPtr - 1 < rs || NextPacketPtr - 1 > re){
		enc28j60Write16(ERXRDPTL, re);
	}else{
		enc28j60Write16(ERXRDPTL, NextPacketPtr - 1);
	}

	// decrement the packet counter indicate we are done with this packet
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);

	return len ;
}
