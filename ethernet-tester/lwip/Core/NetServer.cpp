#include "lwip/opt.h" 
#include "lwip/stats.h" 
#include "lwip/sys.h" 
#include "lwip/pbuf.h" 
#include "lwip/udp.h" 
#include "lwip/tcp.h" 
#include "lwip/dns.h" 
#include "lwip/dhcp.h" 
#include "lwip/init.h"
#include "lwip/netif.h" 
#include "netif/etharp.h" 
#include "netif/loopif.h" 
#include "lwip_device.h"
#include "Ethernet.h"

#include "NetServer.h"
#include "TCPListener.h"
#include "TCPCallbackListener.h"
#include "TCPConnection.h"

using namespace std;
using namespace mbed;

NetServer *NetServer::singleton = NULL;

NetServer::NetServer() : netif(&netif_data), dhcp(true), hostname(HOSTNAME) {
  _time.start();
  IP4_ADDR(&netmask, 255,255,255,255);
  IP4_ADDR(&gateway, 0,0,0,0);
  IP4_ADDR(&ipaddr, 0,0,0,0);
  netif->hwaddr_len = 0;
  del = new list<TCPItem *>();
}

NetServer::NetServer(struct ip_addr ip, struct ip_addr nm, struct ip_addr gw)
 : netif(&netif_data), ipaddr(ip), netmask(nm), gateway(gw), dhcp(false), hostname(HOSTNAME) {
  _time.start();
  netif->hwaddr_len = 0;
  del = new list<TCPItem *>();
}

NetServer::~NetServer() {
  
}

void NetServer::_poll() const {
  while(!del->empty()) {
    TCPItem *item = del->front();
    del->pop_front();
    delete item;
  }
  device_poll();
  tcp_tmr();
}

void NetServer::init() {
  lwip_init();
  
  netif->hwaddr_len = ETHARP_HWADDR_LEN;
  device_address((char *)netif->hwaddr);
  
  netif = netif_add(netif, &ipaddr, &netmask, &gateway, NULL, device_init, ip_input);
  netif->hostname = (char *)this->hostname;
  netif_set_default(netif);
  if(!dhcp) {
    netif_set_up(netif);
  } else {
    dhcp_start(netif);
  }

  tickARP.attach_us( &etharp_tmr,  ARP_TMR_INTERVAL  * 1000); 
  //eth_tick.attach_us<NetServer>(this,&emac_tmr, TCP_FAST_INTERVAL * 1000);
  dns_tick.attach_us(&dns_tmr, DNS_TMR_INTERVAL * 1000);
  if(dhcp) {
    dhcp_coarse.attach_us(&dhcp_coarse_tmr, DHCP_COARSE_TIMER_MSECS * 1000);
    dhcp_fine.attach_us(&dhcp_fine_tmr, DHCP_FINE_TIMER_MSECS * 1000);
  }
}

void NetServer::setUp() const {
  netif_set_up(netif);
}

void NetServer::setDown() const {
  netif_set_down(netif);
}

int NetServer::isUp() const {
  return netif_is_up(netif);
}

void NetServer::waitUntilReady() {
  while(!netif_is_up(netif)) {
    _poll();
    wait_ms(1);
  }
  ipaddr = netif->ip_addr;
  printf("IP: %hhu.%hhu.%hhu.%hhu\n", (ipaddr.addr)&0xFF, (ipaddr.addr>>8)&0xFF, (ipaddr.addr>>16)&0xFF, (ipaddr.addr>>24)&0xFF);
}

TCPCallbackListener *NetServer::bindTCPPort(u16_t port, err_t (*accept)(TCPCallbackListener *, struct tcp_pcb *, err_t)) const {
  TCPCallbackListener *listener = new TCPCallbackListener(port, accept);
  listener->bind();
  return listener;
}

void NetServer::free(TCPItem *item) const {
  del->push_back(item);
}

int NetServer::time() {
  return NetServer::get()->_time.read_ms();
}
