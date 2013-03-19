#ifndef UDP_H
#define UDP_H

#include <stdint.h>

#define UDP_HEADER_LENGTH 8

void udp_handle(uint16_t len);
void udp_tx_reply();
void udp_tx_length(uint16_t);
void udp_tx_checksum();

#endif
