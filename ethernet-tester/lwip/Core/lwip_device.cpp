#include "mbed.h"

using namespace mbed;

Ethernet *eth;
#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/opt.h"

#include "lwip/def.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/stats.h"
#include "netif/etharp.h"
#include "string.h"

#define IFNAME0 'E'
#define IFNAME1 'X'

#define min(x,y) (((x)<(y))?(x):(y))

struct netif *gnetif;

static err_t device_output(struct netif *netif, struct pbuf *p) {
  #if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
  #endif

  do {
    eth->write((const char *)p->payload, p->len);
  } while((p = p->next)!=NULL);

  eth->send();

  #if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
  #endif
  
  LINK_STATS_INC(link.xmit);
  return ERR_OK;
}

void device_poll() {
  struct eth_hdr *ethhdr;
  struct pbuf *frame, *p;
  int len, read;

  while((len = eth->receive()) != 0) {
      frame = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
      if(frame == NULL) {
          return;
      }
      p = frame;
      do {
         read = eth->read((char *)p->payload, p->len);
         p = p->next;
      } while(p != NULL && read != 0);
      
      #if ETH_PAD_SIZE
          pbuf_header(p, ETH_PAD_SIZE);
      #endif

      ethhdr = (struct eth_hdr *)(frame->payload);

      switch(htons(ethhdr->type)) {
          
          case ETHTYPE_IP:
              etharp_ip_input(gnetif, frame);
              pbuf_header(frame, -((s16_t) sizeof(struct eth_hdr)));
              gnetif->input(frame, gnetif);
              break;
          
          case ETHTYPE_ARP:
              etharp_arp_input(gnetif, (struct eth_addr *)(gnetif->hwaddr), frame);
              break;
          
          default:
              break;
      }
      pbuf_free(frame);
  }
}

err_t device_init(struct netif *netif) {
  LWIP_ASSERT("netif != NULL", (netif != NULL));
  
  NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 0x2EA);
  
  /* maximum transfer unit */
  netif->mtu = 0x2EA;
  
  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

  netif->state = NULL;
  gnetif = netif;

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;

  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
  netif->output          = etharp_output;
  netif->linkoutput      = device_output;

  eth = new Ethernet();

  return ERR_OK;
}

void device_address(char *mac) {
    eth->address(mac);
}

#ifdef __cplusplus
};
#endif
