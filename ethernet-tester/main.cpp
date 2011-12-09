#include "mbed.h"
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

Ethernet ethernet;
DigitalOut ledLink(p30);
DigitalOut ledActivity(p29);
DigitalOut ledStage0 (LED1);
DigitalOut ledStage1 (LED2);
DigitalOut ledStage2 (LED3);
DigitalOut ledTCP80 (LED4);

volatile char stage = 0;

Ticker stage_blinker;

struct netif    netif_data;

const char testPage[] = "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "Connection: Close\r\n\r\n"
                        "<html>"
                        "<head>"
                        "<title>mbed test page</title>"
                        "<style type='text/css'>"
                        "body{font-family:'Arial, sans-serif', sans-serif;font-size:.8em;background-color:#fff;}"
                        "</style>"
                        "</head>"
                        "<body>%s</body></html>\r\n\r\n";

char buffer[1024];
char temp[1024];
err_t recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    struct netif   *netif = &netif_data;
    ledTCP80 = true;
    printf("TCP callback from %d.%d.%d.%d\r\n", ip4_addr1(&(pcb->remote_ip)),ip4_addr2(&(pcb->remote_ip)),ip4_addr3(&(pcb->remote_ip)),ip4_addr4(&(pcb->remote_ip)));
    char *data;
    /* Check if status is ok and data is arrived. */
    if (err == ERR_OK && p != NULL) {
        /* Inform TCP that we have taken the data. */
        tcp_recved(pcb, p->tot_len);
        data = static_cast<char *>(p->payload);
        /* If the data is a GET request we can handle it. */
        if (strncmp(data, "GET ", 4) == 0) {
        printf("Handling GET request...\r\n");
        printf("Request:\r\n%s\r\n", data);
        
        //generate the test page
        time_t seconds = time(NULL);
        sprintf(temp,     "<h1>Congratulations!</h1>If you can see this page, your mbed is working properly."
                          "<h2>mbed Configuration</h2>"
                          "mbed RTC time:%s<br/>"
                          "mbed HW address: %02x:%02x:%02x:%02x:%02x:%02x<br/>"
                          "mbed IP Address: %s<br/>",
         ctime(&seconds),
        (char*) netif->hwaddr[0],
        (char*) netif->hwaddr[1],
        (char*) netif->hwaddr[2],
        (char*) netif->hwaddr[3],
        (char*) netif->hwaddr[4],
        (char*) netif->hwaddr[5],
        inet_ntoa(*(struct in_addr*)&(netif->ip_addr))
        );
        sprintf(buffer, testPage, temp);
            if (tcp_write(pcb, (void *)buffer, strlen(buffer), 1) == ERR_OK) {
            tcp_output(pcb);
            printf("Closing connection...\r\n");
            tcp_close(pcb);
            }
        }
        else
        {
            printf("Non GET request...\r\nRequest:\r\n%s\r\n", data);
        }
        
        pbuf_free(p);
    }
    
     else {
            /* No data arrived */
            /* That means the client closes the connection and sent us a packet with FIN flag set to 1. */
            /* We have to cleanup and destroy out TCPConnection. */
             printf("Connection closed by client.\r\n");
            pbuf_free(p);
        }
    /* Don't panic! Everything is fine. */
    ledTCP80 = false;
    return ERR_OK;
}
/* Accept an incomming call on the registered port */
err_t accept_callback(void *arg, struct tcp_pcb *npcb, err_t err) {
    LWIP_UNUSED_ARG(arg);
    /* Subscribe a receive callback function */
    tcp_recv(npcb, &recv_callback);
    /* Don't panic! Everything is fine. */
    return ERR_OK;
}

void stageblinker()
{
    switch (stage)
    {
        case 0:
        ledStage0 = !ledStage0;
        ledStage1 = false;
        ledStage2 = false;
        break;
        case 1:
        ledStage0 = true;
        ledStage1 = !ledStage1;
        ledStage2 = false;
        break;
        case 2:
        ledStage0 = true;
        ledStage1 = true;
        ledStage2 = true;
        stage_blinker.detach();
        break;
    }
}

int main() {
    printf("mBed Ethernet Tester 1.0\r\nStarting Up...\r\n");
    stage = 0;
    struct netif   *netif = &netif_data;
    struct ip_addr  ipaddr;
    struct ip_addr  netmask;
    struct ip_addr  gateway;
    Ticker tickFast, tickSlow, tickARP, eth_tick, dns_tick, dhcp_coarse, dhcp_fine;
    stage_blinker.attach_us(&stageblinker, 1000*500);
    
    char *hostname = "my-mbed";
    
    printf("Configuring device for DHCP...\r\n");
    /* Start Network with DHCP */
    IP4_ADDR(&netmask, 255,255,255,255);
    IP4_ADDR(&gateway, 0,0,0,0);
    IP4_ADDR(&ipaddr, 0,0,0,0);
    /* Initialise after configuration */
    lwip_init();
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    device_address((char *)netif->hwaddr);
    netif = netif_add(netif, &ipaddr, &netmask, &gateway, NULL, device_init, ip_input);
    netif->hostname = hostname;
    netif_set_default(netif);
    dhcp_start(netif); // <-- Use DHCP
    
        /* Initialise all needed timers */
    tickARP.attach_us( &etharp_tmr,  ARP_TMR_INTERVAL  * 1000);
    tickFast.attach_us(&tcp_fasttmr, TCP_FAST_INTERVAL * 1000);
    tickSlow.attach_us(&tcp_slowtmr, TCP_SLOW_INTERVAL * 1000);
    dns_tick.attach_us(&dns_tmr, DNS_TMR_INTERVAL * 1000);
    dhcp_coarse.attach_us(&dhcp_coarse_tmr, DHCP_COARSE_TIMER_MSECS * 1000);
    dhcp_fine.attach_us(&dhcp_fine_tmr, DHCP_FINE_TIMER_MSECS * 1000);
    stage = 1;
         while (!netif_is_up(netif)) { 
         ledLink = ethernet.link();
         device_poll(); 
     } 

/*
    while (!(netif->dhcp->state == DHCP_BOUND || netif->dhcp->state == DHCP_PERMANENT))
    {
        ledLink = ethernet.link();
        device_poll();    
        //printf("Waiting for DHCP response, state = %d\r\n", netif->dhcp->state);
        //wait_ms(100);
        }
    */
    stage = 2;
            printf("Interface is up, local IP is %s\r\n", 
inet_ntoa(*(struct in_addr*)&(netif->ip_addr))); 

    printf("Starting Web Server...\r\n");

        /* Bind a function to a tcp port */
    struct tcp_pcb *pcb = tcp_new();
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) == ERR_OK) {
        pcb = tcp_listen(pcb);
        tcp_accept(pcb, &accept_callback);
    }
    
    printf("Waiting for connection...\r\n");
    while(1) {
        device_poll();
        ledLink = ethernet.link();
    }
}
