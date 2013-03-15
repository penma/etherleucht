#include "ethernet.h"
#include "networking.h"

uint8_t our_mac[6] = { 0x42, 0xcc, 0xcd, 0x00, 0x13, 0x37 };

/* copy sender MAC of received packet to destination MAC of transmit packet,
 * i.e., prepare a reply packet
 */
void eth_tx_reply() {
	assert_spi_active(0);
	uint8_t sender[6];

	enc_rx_seek(6);
	enc_rx_start();
	for (int i = 0; i < 6; i++) {
		sender[i] = enc_rx_read_byte();
	}
	enc_rx_stop();

	enc_tx_seek(0);
	enc_tx_start();
	for (int i = 0; i < 6; i++) {
		enc_tx_write_byte(sender[i]);
	}
	enc_tx_stop();
}

/* copy our mac and an ethertype into the TX buffer */
void eth_tx_type(uint16_t ethertype) {
	/* source address - that's us! */
	enc_tx_seek(6);
	enc_tx_start();
	enc_tx_write_buf(our_mac, 6);

	/* ethertype */
	enc_tx_write_intbe(ethertype);

	enc_tx_stop();
}
