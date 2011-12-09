#include "TCPItem.h"

using namespace std;
using namespace mbed;

void TCPItem::abort() const {
  tcp_abort(this->_pcb);
}

void TCPItem::release_callbacks() const {
  tcp_arg(this->_pcb, NULL);
  tcp_sent(this->_pcb, NULL);
  tcp_recv(this->_pcb, NULL);
  tcp_poll(this->_pcb, NULL, 255);
  tcp_accept(this->_pcb, NULL);
  tcp_err(this->_pcb, NULL);
}

err_t TCPItem::close() {
  err_t err = tcp_close(this->_pcb);
  this->_pcb = NULL;
  return err;
}

void TCPItem::open() {
  if(!this->_pcb) {
    this->_pcb = tcp_new();
    tcp_arg(this->_pcb, this);
  }
}
