#ifndef TCPITEM_H
#define TCPITEM_H

#include "arch/cc.h"
#include "lwip/err.h"
#include "lwip/tcp.h"

namespace mbed {
  class NetServer;

  /**
   * A simple object which provides the base for all TCP enabled objects.
   * Do not ues it directly unless you know what you doing. 
   * Normaly what you want to use is TCPListener or TCPConnector.
   */
  class TCPItem {
    public:
      TCPItem() : _pcb(NULL) {}
      TCPItem(struct tcp_pcb *pcb) : _pcb(pcb) {}
      virtual ~TCPItem() {}
      
      void abort() const;
      void release_callbacks() const;
      err_t close();
      void open();
    protected:
      struct tcp_pcb *_pcb;
  };

};

#endif /* TCPITEM_H */
