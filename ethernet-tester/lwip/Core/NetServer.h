#ifndef NETSERVER_H
#define NETSERVER_H

#include "ipv4/lwip/ip_addr.h"
#include "lwip/netif.h"
#include "netif/etharp.h"
#include "iputil.h"
#include "dns.h"
#include "mbed.h"

#include <list>

namespace mbed {
  class TCPItem;
  class TCPListener;
  class TCPCallbackListener;
  class TCPConnection;

  /**
   * Network main class
   * provides the basic network features.
   */
  class NetServer {
    public:
      /**
       * Returns you the NetServer instance. 
       * If there is no object it will create a new one.
       * But it will not initialise it.
       * Configure the object for DHCP.
       */
      static NetServer *create() {
        if(!NetServer::singleton) {
          NetServer::singleton = new NetServer();
        }
        return NetServer::singleton;
      }

      /**
       * Returns you the NetServer instance. 
       * If there is no object it will create a new one.
       * But it will not initialise it.
       * You have to insert ipaddres, netmask and gateway.
       */
      inline static NetServer *create(const struct ip_addr &ip, const struct ip_addr &netmask, const struct ip_addr &gateway) {
        if(!NetServer::singleton) {
          NetServer::singleton = new NetServer(ip, netmask, gateway);
        }
        return NetServer::singleton;
      }
      
      /**
       * Returns you the NetServer instance. 
       * If there is no object it will create a new one
       * and it will initialise it.
       * A new created object will ever use DHCP and the default MAC address
       * and default hostname.
       */
      inline static NetServer *ready() {
        if(!NetServer::singleton) {
          NetServer::singleton = new NetServer();
        }
        if(!NetServer::singleton->netif->hwaddr_len) {
          NetServer::singleton->init();
          NetServer::singleton->waitUntilReady();
        }
        return NetServer::singleton;
      }

      /**
       * Returns you the NetServer instance.
       * Even if there is no one created.
       * That means use with care and in combination with NetServer::ready().
       * It is mutch quicker than NetServer::ready().
       * First call one time NetServer::ready() and then NetServer::get()
       * and you are save.
       */
      inline static NetServer *get() {
        return NetServer::singleton;
      }

      /**
       * Polls one time on the NetServer and all registert Interfaces.
       * Even if there is no one created.
       * That means use with care and in combination with NetServer::ready().
       * It is mutch faster than NetServer::ready()->_poll().
       * First call one time NetServer::ready() and then NetServer::poll()
       * and you are save.
       */
      inline static void poll() {
        singleton->_poll();
      }
      
      /**
       * Default destructor.
       */
      ~NetServer();
      
      /**
       * Set MBed IP Address
       */
      void setIPAddr(const struct ip_addr &value) { netif->ip_addr = ipaddr = value; }
      /**
       * Get MBed IP Address
       */
      const struct ip_addr &getIPAddr() { return ipaddr = netif->ip_addr; }
      
      /**
       * Set Netmask
       */
      void setNetmask(const struct ip_addr &value) { netif->netmask = netmask = value; }
      
      /**
       * Get Netmask
       */
      const struct ip_addr &getNetmask() { return netmask = netif->netmask; }

      /**
       * Set default Gateway
       */
      void setGateway(const struct ip_addr &value) { netif->gw = gateway = value; }
      
      /**
       * Get default Gateway
       */
      const struct ip_addr &getGateway() { return gateway = netif->gw; }
      
      /**
       * Set first Domain Name Server
       */
      void setDNS1(const struct ip_addr &value) { firstdns = value; dns_setserver(0, &firstdns); }
      
      /**
       * Get first Domain Name Server
       */
      const struct ip_addr &getDNS1() { return firstdns = dns_getserver(0); }
      
      /**
       * Set second Domain Name Server
       */
      void setDNS2(const struct ip_addr &value) { seconddns = value; dns_setserver(1, &firstdns); }
      
      /**
       * Get second Domain Name Server
       */
      const struct ip_addr &getDNS2() { return seconddns = dns_getserver(1); }
      
      /**
       * Set MBed Hostname
       */
      void setHostname(const char *value) { hostname = value; }
      
      /**
       * Get MBed Hostname
       */
      const char *getHostname() const { return hostname; }
      
      /**
       * Define if DHCP sould be used.
       * @param value Bool if true dhcp is used else a static ip setting is assumed.
       */
      void setUseDHCP(const bool &value) { dhcp = value; }
      
      /**
       * Is the mbed board trying to use DHCP?
       */
      const bool &getUseDHCP() const { return dhcp; }

      /**
       * Initialise the network environment. Set up all services.
       * Please call after configuration.
       */
      void init();

      /**
       * Set the network interface up.
       * To enable the network interface after calling setDown()
       * Automaticly called from init().
       */
      void setUp() const;
      
      /**
       * Set the network interface down.
       * To disable the network interface temporary.
       * To make the interface avalible again use setUp().
       */
      void setDown() const;
      
      /**
       * Returns 1 if the network is up otherwise 0.
       */
      int isUp() const;
      
      /**
       * This function waits until the network interface is Up.
       * To use to wait after init with DHCP. Helps continue work 
       * after the network interface is completly up.
       */
      void waitUntilReady();

      /**
       * Bind Callbackfunctions to a TCPPort.
       * It provides a clean lowlevel Interface to the TCPLayer.
       */      
      TCPCallbackListener *bindTCPPort(u16_t, err_t (*)(TCPCallbackListener *, struct tcp_pcb *, err_t)) const;

      /**
       * Frees TCPItems because they cant do it directly.
       */
      void free(TCPItem *item) const;

      static int time();

    protected:
      void _poll() const;

      /**
       * Default constructor tryes to bring the network interface up with dhcp.
       */
      NetServer();

      /**
       * Constructor for fix ip setting
       */
      NetServer(struct ip_addr me_ip, struct ip_addr netmask, struct ip_addr gateway);

    private:
      /**
       * This is a singleton class.
       * So we should not have a public copy constructor.
       */
      NetServer(NetServer const&)            {}
//      NetServer &operator=(NetServer const&) {}
      
      struct netif   *netif;
      struct netif    netif_data;

      struct ip_addr  ipaddr;
      struct ip_addr  netmask;
      struct ip_addr  gateway;
      
      struct ip_addr  firstdns;
      struct ip_addr  seconddns;
      
      bool            dhcp;
      
      list<TCPItem *> *del;
      
      Ticker tickARP, /*eth_tick,*/ dns_tick, dhcp_coarse, dhcp_fine;
      const char *hostname;
      static NetServer *singleton;
      Timer _time;
  };

};
#endif /* NETSERVER_H */
