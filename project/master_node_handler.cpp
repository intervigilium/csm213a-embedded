#include "master_node_handler.h"

MasterNodeHandler::MasterNodeHandler(SyncedSDFileSystem *sdfs, TCPSocket *slave_socket) {
  sdfs_ = sdfs;
  slave_socket_ = slave_socket;
  slave_socket_->setOnEvent(this, &MasterNodeHandler::on_socket_event);
}

MasterNodeHandler::~MasterNodeHandler() {
  close();
}

void MasterNodeHandler::on_socket_event(TCPSocketEvent e) {
  switch (e) {
    case TCPSOCKET_READABLE:
      // don't care about other events yet
      break;
    case TCPSOCKET_WRITEABLE:
      break;
    case TCPSOCKET_CONTIMEOUT:
    case TCPSOCKET_CONRST:
    case TCPSOCKET_CONABRT:
    case TCPSOCKET_ERROR:
    case TCPSOCKET_DISCONNECTED:
      // TODO: handle error
      break;
    case TCPSOCKET_ACCEPT:
    default:
      // these should never happen
      break;
  }
}

void MasterNodeHandler::dispatch_request() {

}

void MasterNodeHandler::close() {
  slave_socket_->resetOnEvent();
  slave_socket_->close();
  delete slave_socket_;
}
