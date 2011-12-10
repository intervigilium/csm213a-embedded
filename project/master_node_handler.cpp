#include "master_node_handler.h"

MasterNodeHandler::MasterNodeHandler(SyncedSDFileSystem *sdfs, TCPSocket *slave_socket) {
  is_closed_ = false;
  sdfs_ = sdfs;
  slave_socket_ = slave_socket;
  slave_socket_->setOnEvent(this, &MasterNodeHandler::on_socket_event);
}

MasterNodeHandler::~MasterNodeHandler() {
  close();
}

bool MasterNodeHandler::is_closed() {
  return is_closed_;
}

void MasterNodeHandler::send_block(const char *buffer, int block_number) {

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
  is_closed_ = true;
  slave_socket_->resetOnEvent();
  slave_socket_->close();
  delete slave_socket_;
}
