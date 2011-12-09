#include "lwip/arch.h"

#include "dns.h"

#include "TCPConnection.h"
#include "TCPListener.h"
#include "NetServer.h"

using namespace std;
using namespace mbed;

void TCPConnection::dnsreply_callback(const char *name, struct ip_addr *ipaddr, void *arg) {
  TCPConnection *connection = static_cast<TCPConnection *>(arg);
  if(connection) {
    (connection->dnsreply)(name, ipaddr);
  }
}

err_t TCPConnection::connected_callback(void *arg, struct tcp_pcb *pcb, err_t err) {
  TCPConnection *connection = static_cast<TCPConnection *>(arg);
  LWIP_UNUSED_ARG(pcb);
  if(connection) {
    return (connection->connected)(err);
  }
  return ERR_OK;
}

err_t TCPConnection::sent_callback(void *arg, struct tcp_pcb *pcb, u16_t space) {
  TCPConnection *connection = static_cast<TCPConnection *>(arg);
  LWIP_UNUSED_ARG(pcb);
  if(connection) {
    return (connection->sent)(space);
  }
  return ERR_OK;
}

err_t TCPConnection::recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
  TCPConnection *connection = static_cast<TCPConnection *>(arg);
  LWIP_UNUSED_ARG(pcb);
  if(connection) {
    return (connection->recv)(p, err);
  }
  return ERR_OK;
}

err_t TCPConnection::poll_callback(void *arg, struct tcp_pcb *pcb) {
  TCPConnection *connection = static_cast<TCPConnection *>(arg);
  LWIP_UNUSED_ARG(pcb);
  if(connection) {
    return (connection->poll)();
  }
  return ERR_OK;
}

void TCPConnection::error_callback(void *arg, err_t erra) {
  TCPConnection *connection = static_cast<TCPConnection *>(arg);
  if(connection) {
    (connection->err)(erra);
  }
}

TCPConnection::TCPConnection() : _parent(NULL), _port(0) {
}

TCPConnection::TCPConnection(struct ip_addr ip, u16_t port) : _parent(NULL) {
  this->_ipaddr = ip;
  this->_port   = port;
}

TCPConnection::TCPConnection(TCPListener *parent, struct tcp_pcb *pcb)
  : TCPItem(pcb), _parent(parent), _ipaddr(pcb->remote_ip), _port(pcb->remote_port) {
  tcp_arg(this->_pcb, static_cast<void *>(this));
  connected(ERR_OK);
}

TCPConnection::~TCPConnection() {
}

err_t TCPConnection::write(void *msg, u16_t len, u8_t flags) const {
  return tcp_write(this->_pcb, msg, len, flags);
}
    
void TCPConnection::recved(u32_t len) const {
  tcp_recved(this->_pcb, len);
}

err_t TCPConnection::connected(err_t err) {
  tcp_recv(this->_pcb, TCPConnection::recv_callback);
  tcp_sent(this->_pcb, TCPConnection::sent_callback);
  tcp_poll(this->_pcb, TCPConnection::poll_callback, 1); // addjust time (in twice a sec)
  tcp_err(this->_pcb,  TCPConnection::error_callback);
  return ERR_OK;                                                    
}

void TCPConnection::connect() {
  NetServer::ready();
  open();
  tcp_connect(this->_pcb, &this->_ipaddr, this->_port, TCPConnection::connected_callback); 
}

err_t TCPConnection::dnsrequest(const char *hostname, struct ip_addr *addr) const {
  return dns_gethostbyname(hostname, addr, TCPConnection::dnsreply_callback, (void *)this);
}
