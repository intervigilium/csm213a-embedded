#ifndef LWIPOPTS_H
#define LWIPOPTS_H

#include <string.h>
#include <stdlib.h>
//#include <mbed.h>
#include <stdio.h>

#ifdef __cplusplus
using namespace std;
#endif

// Application specific lwIP Options.
#define IPv6                            0
#define NO_SYS                          1
#define LWIP_ARP                        1
#define LWIP_RAW                        0
#define LWIP_UDP                        1
#define LWIP_TCP                        1
#define LWIP_DNS                        1
#define LWIP_DHCP                       1
#define LWIP_IGMP                       0
#define LWIP_SNMP                       0
#define LWIP_SOCKET                     0
#define LWIP_NETCONN                    0
#define LWIP_AUTOIP                     0
#define LWIP_CALLBACK_API               1

#define MEM_LIBC_MALLOC                 0
#define MEMP_MEM_MALLOC                 1
#define MEM_ALIGNMENT                   4
//#define MEM_SIZE                     5000
#define MEM_SIZE                      10000
//#define MEM_SIZE            (EMAC_MEM_SIZE - (2 * SIZEOF_STRUCT_MEM) - MEM_ALIGNMENT)
#define MEM_POSITION                    __attribute((section("AHBSRAM1"),aligned))
//        EMAC_MEM_ADDR

#define ARP_QUEUEING                    0
#define LWIP_NETIF_HOSTNAME             1

#define ARP_TABLE_SIZE                  4

#define DNS_TABLE_SIZE                  1
#define DNS_USES_STATIC_BUF             0
// 0 - Stack
// 1 - RW-MEM
// 2 - Heap

#define IP_FRAG_USES_STATIC_BUF         0
#define LWIP_STATS                      0

#define DNS_LOCAL_HOSTLIST_IS_DYNAMIC   1

#define TCP_SND_BUF                  2000
#define TCP_MSS                     0x276
//0x300
//#define TCP_SND_QUEUELEN                    (2 * TCP_SND_BUF/TCP_MSS)
#define TCP_SND_QUEUELEN               16
#define MEMP_NUM_TCP_PCB                5
#define MEMP_NUM_TCP_PCB_LISTEN         8
#define MEMP_NUM_TCP_SEG               20
#define MEMP_NUM_PBUF                  16
#define PBUF_POOL_SIZE                  6

#ifndef HOSTNAME
#define HOSTNAME "mbed-c3p0"
#endif

//#define LWIP_DEBUG               1
//#define LWIP_DBG_TYPES_ON     ~0x0
//#define LWIP_DBG_MIN_LEVEL       0
//#define MEM_DEBUG (LWIP_DBG_ON | LWIP_DBG_LEVEL_WARNING)
//#define TCP_INPUT_DEBUG (LWIP_DBG_ON | LWIP_DBG_LEVEL_WARNING)
//#define TCP_OUTPUT_DEBUG    (LWIP_DBG_ON | LWIP_DBG_LEVEL_WARNING)
//#define NETIF_DEBUG     (LWIP_DBG_ON | LWIP_DBG_LEVEL_WARNING)
//#define DHCP_DEBUG      (LWIP_DBG_ON | LWIP_DBG_LEVEL_WARNING)
//#define IP_DEBUG        (LWIP_DBG_ON | LWIP_DBG_LEVEL_WARNING)
//#define TCP_DEBUG       (LWIP_DBG_ON | LWIP_DBG_LEVEL_WARNING)
//#define TCP_CWND_DEBUG    (LWIP_DBG_ON | LWIP_DBG_LEVEL_WARNING)

#endif
