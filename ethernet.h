#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>

#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_ARP 0x0806
#define ETH_HEADER_LENGTH 14

extern uint8_t our_mac[6];

void eth_tx_reply();
void eth_tx_type(uint16_t ethertype);

#endif
