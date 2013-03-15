#ifndef IPV4_H
#define IPV4_H

#define IPPROTO_ICMP    1
#define IPPROTO_UDP    17

#define IPV4_HDR_OFF_LENGTH     2
#define IPV4_HDR_OFF_TTL        8
#define IPV4_HDR_OFF_PROTO      9
#define IPV4_HDR_OFF_CHECKSUM  10
#define IPV4_HDR_OFF_SOURCE_IP 12
#define IPV4_HDR_OFF_DEST_IP   16
/* yes, options. but we don't accept any packets with options, and never send
 * them out ourselves. so this is fine
 */
#define IPV4_HEADER_LENGTH     20

extern uint8_t our_ipv4[4];
void ipv4_handle();

#endif
