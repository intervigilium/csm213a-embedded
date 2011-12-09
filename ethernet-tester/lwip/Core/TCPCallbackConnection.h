#ifndef TCPCALLBACKCONNECTION_H
#define TCPCALLBACKCONNECTION_H

#include "TCPConnection.h"

namespace mbed {

#define NO_SENT_FNC    ((err_t (*)(TCPCallbackConnection *, u16_t))NULL)
#define NO_RECV_FNC    ((err_t (*)(TCPCallbackConnection *, struct pbuf *, err_t))NULL)
#define NO_POLL_FNC    ((err_t (*)(TCPCallbackConnection *))NULL)
#define NO_ACCEPT_FNC  ((err_t (*)(TCPCallbackConnection *, struct tcp_pcb *, err_t))NULL)
#define NO_CONNECT_FNC ((err_t (*)(TCPCallbackConnection *, err_t))NULL)
#define NO_ERR_FNC      ((void (*)(TCPCallbackConnection *, err_t))NULL)


class TCPCallbackConnection : public TCPConnection {
  public:
    TCPCallbackConnection(struct ip_addr ip_addr, u16_t port,
       err_t (*psent)(TCPCallbackConnection *, u16_t),
       err_t (*precv)(TCPCallbackConnection *, struct pbuf *, err_t),
       err_t (*ppoll)(TCPCallbackConnection *),
       err_t (*pconnected)(TCPCallbackConnection *, err_t),
       void  (*perr )(TCPCallbackConnection *, err_t))
      : TCPConnection(ip_addr, port) {
      _sent = psent;
      _recv = precv;
      _poll = ppoll;
      _connected = pconnected;
      _err  = perr;
    }

    TCPCallbackConnection(TCPListener *parent, struct tcp_pcb *npcb,
       err_t (*psent)(TCPCallbackConnection *, u16_t),
       err_t (*precv)(TCPCallbackConnection *, struct pbuf *, err_t),
       err_t (*ppoll)(TCPCallbackConnection *),
       err_t (*pconnected)(TCPCallbackConnection *, err_t),
       void  (*perr )(TCPCallbackConnection *, err_t))
      : TCPConnection(parent, npcb) {
      _sent = psent;
      _recv = precv;
      _poll = ppoll;
      _connected = pconnected;
      _err  = perr;
    }

  private:
    /*
     * Function to be called when more send buffer space is available.
     * @param space the amount of bytes available
     * @return ERR_OK: try to send some data by calling tcp_output
     */
    virtual err_t sent(u16_t space) { 
      if(_sent) {
        return (_sent)(this, space);
      } else {
        return ERR_OK;
      }
    }
  
    /*
     * Function to be called when (in-sequence) data has arrived.
     * @param p the packet buffer which arrived
     * @param err an error argument (TODO: that is current always ERR_OK?)
     * @return ERR_OK: try to send some data by calling tcp_output
     */
    virtual err_t recv(struct pbuf *p, err_t err) { 
      if(_recv) {
        return (_recv)(this, p, err);
      } else {
        return ERR_OK;
      }
    }

    /*
     * Function which is called periodically.
     * The period can be adjusted in multiples of the TCP slow timer interval
     * by changing tcp_pcb.polltmr.
     * @return ERR_OK: try to send some data by calling tcp_output
     */
    virtual err_t poll() {
      if(_poll) {
        return (_poll)(this);
      } else {
        return ERR_OK;
      }
    }

    virtual err_t connected(err_t err) {
      err = TCPConnection::connected(err);
      if(_connected) {
        return (_connected)(this, err);
      } else {
        return ERR_OK;
      }
    }

    /*
     * Function to be called whenever a fatal error occurs.
     * There is no pcb parameter since most of the times, the pcb is
     * already deallocated (or there is no pcb) when this function is called.
     * @param err an indication why the error callback is called:
     *            ERR_ABRT: aborted through tcp_abort or by a TCP timer
     *            ERR_RST: the connection was reset by the remote host
     */
    virtual void err(err_t erra) {
      if(_err) {
        (_err)(this, erra);
      }
    }

    virtual void dnsreply(const char *hostname, struct ip_addr *addr) {};

    err_t (*_sent)(TCPCallbackConnection *, u16_t);
    err_t (*_recv)(TCPCallbackConnection *, struct pbuf *p, err_t err);
    err_t (*_poll)(TCPCallbackConnection *);
    err_t (*_accept)(TCPCallbackConnection *, struct tcp_pcb *newpcb, err_t err);
    err_t (*_connected)(TCPCallbackConnection *, err_t err);
    void  (*_err )(TCPCallbackConnection *, err_t);
};
  
};

#endif /* TCPCALLBACKCONNECTION_H */
