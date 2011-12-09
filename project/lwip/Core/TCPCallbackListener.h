#ifndef TCPCALLBACKLISTENER_H
#define TCPCALLBACKLISTENER_H

#include "TCPListener.h"

namespace mbed {
  class NetServer;

  class TCPCallbackListener : public TCPListener {
    public:
      TCPCallbackListener( 
        u16_t port,
        err_t (*paccept)(TCPCallbackListener *, struct tcp_pcb *, err_t))
       : TCPListener(port), _accept(paccept) {
      }
      
    private:
      virtual err_t accept(struct tcp_pcb *newpcb, err_t err) {
        if(_accept) {
          return (_accept)(this, newpcb, err);
        } else {
          return ERR_OK;
        }
      }
    
      err_t (*_accept)(TCPCallbackListener *, struct tcp_pcb *newpcb, err_t err);
      
    friend class NetServer;
  };

};

#endif /* TCPCALLBACKLISTENER_H */
