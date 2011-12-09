#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "arch/cc.h"
#include "lwip/err.h"
//#include "lwip/tcp.h"

#include "TCPItem.h"

namespace mbed {

class NetServer;
class TCPListener;

class TCPConnection : public TCPItem {
  public:
    TCPConnection(struct ip_addr, u16_t);
    TCPConnection(TCPListener *, struct tcp_pcb *);

    virtual ~TCPConnection();

    void connect();
    
    err_t write(void *, u16_t len, u8_t flags = TCP_WRITE_FLAG_COPY) const;

    void recved(u32_t len) const;
    u16_t sndbuf() const { return tcp_sndbuf(_pcb); }
    
    void set_poll_timer(const u32_t &time) {
      if(_pcb) {
        _pcb->polltmr = time;
      }
    }
    
    u32_t get_poll_interval() const {
      return (_pcb)? _pcb->pollinterval: 0;
    }
    
    void set_poll_interval(const u32_t &value) {
      if(_pcb) {
        _pcb->pollinterval = value;
      }
    }
    
    u32_t get_poll_timer() const {
      return (_pcb)? _pcb->polltmr: 0;
    }
    
  protected:  
    TCPConnection();

    /**
     * Function to be called when more send buffer space is available.
     * @param space the amount of bytes available
     * @return ERR_OK: try to send some data by calling tcp_output
     */
    virtual err_t sent(u16_t space) = 0;
  
    /**
     * Function to be called when (in-sequence) data has arrived.
     * @param p the packet buffer which arrived
     * @param err an error argument (TODO: that is current always ERR_OK?)
     * @return ERR_OK: try to send some data by calling tcp_output
     */
    virtual err_t recv(struct pbuf *p, err_t err) = 0;

    /**
     * Function to be called when a connection has been set up.
     * @param pcb the tcp_pcb that now is connected
     * @param err an error argument (TODO: that is current always ERR_OK?)
     * @return value is currently ignored
     */
    virtual err_t connected(err_t err);

    /**
     * Function which is called periodically.
     * The period can be adjusted in multiples of the TCP slow timer interval
     * by changing tcp_pcb.polltmr.
     * @return ERR_OK: try to send some data by calling tcp_output
     */
    virtual err_t poll() = 0;

    /**
     * Function to be called whenever a fatal error occurs.
     * There is no pcb parameter since most of the times, the pcb is
     * already deallocated (or there is no pcb) when this function is called.
     * @param err an indication why the error callback is called:
     *            ERR_ABRT: aborted through tcp_abort or by a TCP timer
     *            ERR_RST: the connection was reset by the remote host
     */
    virtual void err(err_t err) = 0;
    
    virtual void dnsreply(const char *hostname, struct ip_addr *addr) = 0;

    err_t dnsrequest(const char *hostname, struct ip_addr *addr) const;
    
  private:
    static void dnsreply_callback(const char *name, struct ip_addr *ipaddr, void *arg);
    static err_t connected_callback(void *arg, struct tcp_pcb *pcb, err_t err);
    static err_t sent_callback(void *arg, struct tcp_pcb *pcb, u16_t space);
    static err_t recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
    static err_t poll_callback(void *arg, struct tcp_pcb *pcb);
    static void error_callback(void *arg, err_t erra);
    
  protected:
    TCPListener *_parent;
    struct ip_addr _ipaddr;
    u16_t _port;

  friend class NetServer;
};

};

#endif /* TCPCONNECTION_H */
