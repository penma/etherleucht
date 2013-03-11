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


#include "enc28j60.h"
#include "spi.h"

#include "debug.h"

void enc_spi_select(uint8_t act) {
	if (act) {
		PORTA &= ~(1 << PA1);
	} else {
		PORTA |= (1 << PA1);
	}
}

void enc_spi_init() {
	DDRA |= (1 << PA1);
	enc_spi_select(0);
}

uint8_t enc_op_read(uint8_t op, uint8_t address) {
	enc_spi_select(1);

	spi_write(op | (address & ADDR_MASK));
	if (address & SPRD_MASK) {
		spi_read();
	}
	uint8_t data = spi_read();

	enc_spi_select(0);
	return data;
}

void enc_op_write(uint8_t op, uint8_t address, uint8_t data) {
	enc_spi_select(1);

	spi_write(op | (address & ADDR_MASK));
	spi_write(data);

	enc_spi_select(0);
}

static void enc_set_bank(uint8_t address) {
	// set the bank (if needed)
	static uint8_t bank = 0;
	if ((BANK_MASK & address) != bank) {
		enc_op_write(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1 | ECON1_BSEL0));
		enc_op_write(ENC28J60_BIT_FIELD_SET, ECON1, (BANK_MASK & address) >> 5);
		bank = (BANK_MASK & address);
	}
}

uint8_t enc_reg_read(uint8_t address) {
	enc_set_bank(address);
	return enc_op_read(ENC28J60_READ_CTRL_REG, address);
}

void enc_reg_write(uint8_t address, uint8_t data) {
	enc_set_bank(address);
	enc_op_write(ENC28J60_WRITE_CTRL_REG, address, data);
}

void enc_reg_write16(uint8_t baseaddr, uint16_t data) {
	enc_reg_write(baseaddr      , data & 0xff);
	enc_reg_write(baseaddr | 0x1, data >> 8);
}

uint16_t enc_reg_read16(uint8_t baseaddr) {
	uint16_t data;
	data = enc_reg_read(baseaddr);
	data |= enc_reg_read(baseaddr | 0x1) << 8;
	return data;
}

uint16_t enc_phy_read(uint8_t address) {
	// Set the right address and start the register read operation
	enc_reg_write(MIREGADR, address);
	enc_reg_write(MICMD, MICMD_MIIRD);

	// wait until the PHY read completes
	while(enc_reg_read(MISTAT) & MISTAT_BUSY) { }

	// quit reading
	enc_reg_write(MICMD, 0x00);

	return enc_reg_read16(MIRDL);
}

void enc_phy_write(uint8_t address, uint16_t data) {
	// set the PHY register address
	enc_reg_write(MIREGADR, address);

	// write the PHY data
	enc_reg_write16(MIWRL, data);

	// wait until the PHY write completes
	while (enc_reg_read(MISTAT) & MISTAT_BUSY) { }
}

