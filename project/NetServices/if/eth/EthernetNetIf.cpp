
/*
Copyright (c) 2010 Donatien Garnier (donatiengar [at] gmail [dot] com)
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "EthernetNetIf.h"

#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/igmp.h"

#include "drv/eth/eth_drv.h"
#include "mbed.h"

#define __DEBUG
#include "dbg/dbg.h"

#include "netCfg.h"
#if NET_ETH

EthernetNetIf::EthernetNetIf() : LwipNetIf(), m_ethArpTimer(), m_dhcpCoarseTimer(), m_dhcpFineTimer(), m_igmpTimer(), m_pNetIf(NULL),
m_netmask(255,255,255,255), m_gateway(), m_hostname(NULL)
{
  m_hostname = NULL;
  m_pNetIf = new netif;
  m_useDhcp = true;
}

EthernetNetIf::EthernetNetIf(IpAddr ip, IpAddr netmask, IpAddr gateway, IpAddr dns) : LwipNetIf(), m_ethArpTimer(), m_dhcpCoarseTimer(), m_dhcpFineTimer(), m_igmpTimer(), m_pNetIf(NULL), m_hostname(NULL) //W/o DHCP
{
  m_hostname = NULL;
  m_netmask = netmask;
  m_gateway = gateway;
  m_ip = ip;
  m_pNetIf = new netif;
  dns_setserver(0, &dns.getStruct());
  m_useDhcp = false;
}

EthernetNetIf::~EthernetNetIf()
{
  if(m_pNetIf)
  {
    igmp_stop(m_pNetIf); //Stop IGMP processing
    netif_set_down(m_pNetIf);
    netif_remove(m_pNetIf);
    delete m_pNetIf;
    eth_free();
  }
}
  
EthernetErr EthernetNetIf::setup(int timeout_ms /*= 15000*/)
{
  LwipNetIf::init();
  //m_ethArpTicker.attach_us(&etharp_tmr,  ARP_TMR_INTERVAL  * 1000); // = 5s in etharp.h
  m_ethArpTimer.start();
  if(m_useDhcp)
  {
    //m_dhcpCoarseTicker.attach(&dhcp_coarse_tmr, DHCP_COARSE_TIMER_SECS); // = 60s in dhcp.h
    //m_dhcpFineTicker.attach_us(&dhcp_fine_tmr, DHCP_FINE_TIMER_MSECS * 1000); // = 500ms in dhcp.h
    m_dhcpCoarseTimer.start();
    m_dhcpFineTimer.start();
  }
  m_pNetIf->hwaddr_len = ETHARP_HWADDR_LEN; //6
  eth_address((char *)m_pNetIf->hwaddr);
  
  DBG("HW Addr is : %02x:%02x:%02x:%02x:%02x:%02x.\n", 
  m_pNetIf->hwaddr[0], m_pNetIf->hwaddr[1], m_pNetIf->hwaddr[2],
  m_pNetIf->hwaddr[3], m_pNetIf->hwaddr[4], m_pNetIf->hwaddr[5]);

  m_pNetIf = netif_add(m_pNetIf, &(m_ip.getStruct()), &(m_netmask.getStruct()), &(m_gateway.getStruct()), NULL, eth_init, ip_input);//ethernet_input);// ip_input);
  //m_pNetIf->hostname = "mbedDG";//(char *)m_hostname; //Not used for now
  netif_set_default(m_pNetIf);
  
  //DBG("\r\nStarting DHCP.\r\n");
  
  if(m_useDhcp)
  {
    dhcp_start(m_pNetIf);
    DBG("DHCP Started, waiting for IP...\n");
  }
  else
  {
    netif_set_up(m_pNetIf);
  }
  
  Timer timeout;
  timeout.start();
  while( !netif_is_up(m_pNetIf) ) //Wait until device is up
  {
    if(m_useDhcp)
    {
      if(m_dhcpFineTimer.read_ms()>=DHCP_FINE_TIMER_MSECS)
      {
        m_dhcpFineTimer.reset();
        dhcp_fine_tmr();
      }
      if(m_dhcpCoarseTimer.read()>=DHCP_COARSE_TIMER_SECS)
      {
        m_dhcpCoarseTimer.reset();
        dhcp_coarse_tmr();
      }
    }
    poll();
    if( timeout.read_ms() > timeout_ms )
    {
      //Abort
      if(m_useDhcp)
        dhcp_stop(m_pNetIf);
      else
        netif_set_down(m_pNetIf);
      DBG("\r\nTimeout.\r\n");
      return ETH_TIMEOUT;
    }
  }
  
  #if LWIP_IGMP
  igmp_start(m_pNetIf); //Start IGMP processing
  #endif
  
  m_ip = IpAddr(&(m_pNetIf->ip_addr));
   
  DBG("Connected, IP : %d.%d.%d.%d\n", m_ip[0], m_ip[1], m_ip[2], m_ip[3]);
  
  return ETH_OK;
}

void EthernetNetIf::poll()
{
  if(m_ethArpTimer.read_ms()>=ARP_TMR_INTERVAL)
  {
    m_ethArpTimer.reset();
    etharp_tmr();
  }
  #if LWIP_IGMP
  if(m_igmpTimer.read_ms()>=IGMP_TMR_INTERVAL)
  {
    m_igmpTimer.reset();
    igmp_tmr();
  }
  #endif
  LwipNetIf::poll();
  eth_poll();
}

#endif
