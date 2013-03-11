#include "networking.h"
#include "enc28j60.h"
#include "spi.h"

#include <util/delay.h>
#include "debug.h"

static uint16_t packet_next, packet_current;
static uint8_t mac[6] = { 0x42, 0xcc, 0xcd, 0x00, 0x13, 0x37 };

void enc_init() {
	enc_spi_init();

	/* reset */
	enc_op_write(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
	_delay_us(80);
	/* wait for reset to complete
	 * XXX is this reliable enough in case of total communication failure?
	 */
	while (!(enc_reg_read(ESTAT) & ESTAT_CLKRDY)) { }

	/* Bank 0 stuff
	 * Ethernet buffer addresses
	 */
	packet_next = RXST;
	enc_reg_write16(ERXSTL, RXST);
	enc_reg_write16(ERXNDL, RXND);
	enc_reg_write16(ERXRDPTL, RXND);

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
	enc_reg_write(MAADR5, mac[0]);
	enc_reg_write(MAADR4, mac[1]);
	enc_reg_write(MAADR3, mac[2]);
	enc_reg_write(MAADR2, mac[3]);
	enc_reg_write(MAADR1, mac[4]);
	enc_reg_write(MAADR0, mac[5]);

	/* no loopback of transmitted frames */
	enc_phy_write(PHCON2, PHCON2_HDLDIS);

	/* Enable interrupts and enable packet reception */
	enc_op_write(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
	enc_op_write(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
	/* LEDs
	 * A: Link status + receive
	 * B: Transmit
	 * long stretched
	 */
	enc_phy_write(PHLCON, 0xc1a);
}

void enc_rx_acknowledge() {
	/* errata B7 #14: ERXRDPT must contain an odd value */
	if (packet_next == RXST) {
		enc_reg_write16(ERXRDPTL, RXND);
	} else {
		enc_reg_write16(ERXRDPTL, packet_next - 1);
	}

	enc_op_write(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
}

void enc_rx_start() {
	enc_spi_select(1);
	spi_write(ENC28J60_READ_BUF_MEM);
}

void enc_rx_stop() {
	enc_spi_select(0);
}

uint8_t enc_rx_read_byte() {
	return spi_read();
}

uint16_t enc_rx_read_intle() {
	uint8_t low = spi_read();
	return low | (spi_read() << 8);
}

void enc_rx_read_buf(uint8_t dst[], uint16_t len) {
	for (uint16_t i = 0; i < len; i++) {
		dst[i] = spi_read();
	}
}


/* check if there is a packet waiting to be processed. */
uint8_t enc_rx_has_packet() {
	return enc_reg_read(EPKTCNT) != 0;
}

/* accept a pending packet.
 * also read the status headers, update next/current packet pointers.
 * will return payload length.
 */
uint16_t enc_rx_accept_packet() {
	/* set read pointer to start of new packet */
	enc_reg_write16(ERDPTL, packet_next);
	packet_current = packet_next;

	/* read headers, update pointers */
	enc_rx_start();
	packet_next = enc_rx_read_intle();

	uint16_t len = enc_rx_read_intle();

	// remove CRC from len (we don't read the CRC from
	// the receive buffer
	len -= 4;

	/* receive status (ignored) */
	enc_rx_read_intle();

	enc_rx_stop();

	return len;
}

/* seek to receive buffer location
 * relative to current packet's payload
 */
void enc_rx_seek(uint16_t pos) {
	uint16_t target = packet_current + 6 + pos;
	if (target > RXND) {
		target = RXST + target - RXND - 1;
	}
	enc_reg_write16(ERDPTL, target);
}

/* seek to transmit buffer location
 * relative to packet payload
 */
void enc_tx_seek(uint16_t pos) {
	/* skip 1 control byte */
	enc_reg_write16(EWRPTL, TXSTART_INIT + pos + 1);
}

void enc_tx_start() {
	enc_spi_select(1);
	spi_write(ENC28J60_WRITE_BUF_MEM);
}

void enc_tx_stop() {
	enc_spi_select(0);
}

void enc_tx_write_byte(uint8_t c) {
	spi_write(c);
}

void enc_tx_write_intbe(uint16_t c) {
	spi_write(c >> 8);
	spi_write(c & 0xff);
}

void enc_tx_write_buf(uint8_t src[], uint16_t len) {
	for (uint16_t i = 0; i < len; i++) {
		spi_write(src[i]);
	}
}


/* transmit a packet that has already been written to the transmit buffer
 * expects layer 3+ payload length and an ethertype
 * completes layer 2 header, then sends
 */
void enc_tx_do(uint16_t len, uint16_t ethertype, uint8_t is_reply) {
	/* errata B7 #12: reset TX logic because it may be in an inconsistent
	 * state
	 */
	enc_op_write(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRST);
	enc_op_write(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRST);

	/* end pointer points to the last byte
	 * (payload + ethernet header (14 byte) + 1 enc28j60 control byte - 1)
	 */
	enc_reg_write16(ETXSTL, TXSTART_INIT);
	enc_reg_write16(ETXNDL, TXSTART_INIT + len + 14);

	enc_reg_write16(EWRPTL, TXSTART_INIT);
	enc_tx_start();
	enc_tx_write_byte(0);

	/* destination address */
	if (is_reply) {
		debug_fstr("! no reply addr\n");
		debug_fstr("recorded yet\n");
		for (int i = 0; i < 6; i++) {
			enc_tx_write_byte(0xff);
		}
	} else {
		for (int i = 0; i < 6; i++) {
			enc_tx_write_byte(0xff);
		}
	}

	/* source address - that's us! */
	enc_tx_write_buf(mac, 6);

	/* ethertype */
	enc_tx_write_intbe(ethertype);

	enc_tx_stop();

	/* finally, transmit */
	enc_op_write(ENC28J60_BIT_FIELD_CLR, ESTAT, ESTAT_TXABRT);
	enc_op_write(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);

	/* wait for completion */
	uint8_t try = 0xff;
	while (enc_reg_read(ECON1) & ECON1_TXRTS) {
		try--;
		if (try == 0) {
			debug_fstr("TX timeout\n");
			return;
		}
	}

	uint8_t stat = enc_reg_read(ESTAT);
	if (stat & ESTAT_TXABRT) {
		debug_fstr("TX error\n");
		return;
	}
}

/* compute a checksum over some range in the buffer */
static uint16_t enc_checksum(uint16_t start, uint16_t len) {
	uint16_t end = start + len - 1;
	if ((start <= RXND) && (end > RXND)) {
		end = RXST + end - RXND - 1;
	}

	enc_reg_write16(EDMASTL, start);
	enc_reg_write16(EDMANDL, end);

	enc_op_write(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_CSUMEN | ECON1_DMAST);
	uint16_t try = 0xffff;
	while (!(enc_reg_read(EIR) & EIR_DMAIF)) {
		try--;
		if (try == 0) {
			debug_fstr("Chksum timeout\n");
			return 0;
		}
	}

	return enc_reg_read16(EDMACSL);
}

#define IP_TTL 0xff
#define IP_PROTO_UDP 0x11

void enc_tx_header_udp(uint16_t len) {
	/* we need to compute the checksum over the pseudo header
	 * we vandalize parts of the IP header to do that
	 */
	enc_tx_seek(14 + 8);
	enc_tx_start();
	enc_tx_write_byte(0);
	enc_tx_write_byte(IP_PROTO_UDP);
	enc_tx_write_intbe(len + 8);
	enc_tx_stop();

	enc_tx_seek(14 + 20 + 4);
	enc_tx_start();
	enc_tx_write_intbe(len + 8);
	enc_tx_write_intbe(0);
	enc_tx_stop();

	uint16_t sum = enc_checksum(TXSTART_INIT + 1 + 14 + 8, 20 + len);

	enc_tx_seek(14 + 20 + 6);
	enc_tx_start();
	enc_tx_write_intbe(sum);
	enc_tx_stop();
}

void enc_tx_header_ipv4() {
	/* TODO complete more headers */

	/* our header may have been vandalized by udp checksum computation.
	 * restore some fields
	 */
	enc_tx_seek(14 + 8);
	enc_tx_start();
	enc_tx_write_byte(IP_TTL);
	enc_tx_write_byte(IP_PROTO_UDP);
	enc_tx_write_intbe(0);
	enc_tx_stop();

	uint16_t sum = enc_checksum(TXSTART_INIT + 1 + 14, 20);

	enc_tx_seek(14 + 10);
	enc_tx_start();
	enc_tx_write_intbe(sum);
	enc_tx_stop();
}
