#ifndef NETWORKING_H
#define NETWORKING_H

#include <stdint.h>

#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_ARP 0x0806
#define ETH_HEADER_LENGTH 14

void enc_init();

uint8_t enc_rx_has_packet();
uint16_t enc_rx_accept_packet();
void enc_rx_acknowledge();
void enc_rx_start();
void enc_rx_stop();
void enc_rx_seek(uint16_t pos);
uint8_t enc_rx_read_byte();
uint16_t enc_rx_read_intle();
uint16_t enc_rx_read_intbe();
void enc_rx_read_buf(uint8_t dst[], uint16_t len);

void enc_tx_start();
void enc_tx_stop();
void enc_tx_seek(uint16_t pos);
void enc_tx_write_byte(uint8_t);
void enc_tx_write_intbe(uint16_t);
void enc_tx_write_buf(uint8_t src[], uint16_t len);
void enc_tx_prepare_reply();
void enc_tx_do(uint16_t len);
void enc_tx_header_udp(uint16_t len);
void enc_tx_header_ipv4();
void enc_tx_checksum_ipv4();
void enc_tx_checksum_icmp(uint16_t len);

uint16_t enc_tx_checksum(uint16_t, uint16_t);
void enc_rxtx_copy(uint16_t, uint16_t, uint16_t);

#endif
