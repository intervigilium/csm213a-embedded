#ifndef LWIP_UTILS_H
#define LWIP_UTILS_H

#include "ipv4/lwip/ip_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This method converts 4 given IPv4 tuples to struct ip_addr classes.
 * The Byte are seperated by ,
 * Does only work with seperated 4 Byte tuple.
 */
inline struct ip_addr IPv4(u8_t ip0, u8_t ip1, u8_t ip2, u8_t ip3) {
  struct ip_addr addr;
  IP4_ADDR(&addr, ip0, ip1, ip2, ip3);
  return addr;
}

unsigned int base64enc_len(const char *str);

void base64enc(const char *input, unsigned int length, char *output);

#ifdef __cplusplus
}
#endif

#endif /* LWIP_UTILS_H */
