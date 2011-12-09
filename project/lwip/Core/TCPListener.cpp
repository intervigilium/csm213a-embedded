#include "TCPListener.h"
#include "NetServer.h"

using namespace std;
using namespace mbed;

err_t TCPListener::accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
  TCPListener *listener   = static_cast<TCPListener *>(arg);
  if(listener) {
    return (listener->accept)(newpcb, err);
  }
  return ERR_OK;
}

void TCPListener::bind() {
  NetServer::ready();
  open();
  tcp_arg(this->_pcb, static_cast<void *>(this));
  if(tcp_bind(this->_pcb, IP_ADDR_ANY, this->_port) == ERR_OK) {
    this->_pcb = tcp_listen(this->_pcb);
    tcp_accept(this->_pcb, TCPListener::accept_callback);
  }
}
