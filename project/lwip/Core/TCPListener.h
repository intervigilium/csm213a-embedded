#ifndef TCPLISTENER_H
#define TCPLISTENER_H

#include "TCPItem.h"

#include "arch/cc.h"
#include "lwip/err.h"
#include "lwip/tcp.h"

namespace mbed {
  class NetServer;
  class TCPConnection;

  class TCPListener : public TCPItem {
    public:
      TCPListener(u16_t port) : _port(port) { 
      }

      virtual ~TCPListener() {}

      void bind();
    protected:
      /**
       * Function to call when a listener has been connected.
       * @param err an error argument (TODO: that is current always ERR_OK?)
       * @return ERR_OK: accept the new connection,
       *                 any other err_t abortsthe new connection
       */
      virtual err_t accept(struct tcp_pcb *, err_t err) = 0;
      
      u16_t _port;
    private:
      static err_t accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
    friend class NetServer;
    friend class TCPConnection;
    
  };

};

#endif /* TCPLISTENER_H */
