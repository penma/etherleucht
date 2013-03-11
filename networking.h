#ifndef NETWORKING_H
#define NETWORKING_H

#include <stdint.h>

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
void enc_tx_do(uint16_t len, uint16_t ethertype, uint8_t is_reply);
void enc_tx_header_udp(uint16_t len);
void enc_tx_header_ipv4();

#endif
