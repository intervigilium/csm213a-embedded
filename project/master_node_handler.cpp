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
  int ret;
  char msg_type = MSG_UPDATE_BLOCK;
  ret = slave_socket_->send((char *) &msg_type, 1);
  if (ret != 1) {
    // TODO: handle error
  }
  ret = slave_socket_->send((char *) &block_number, sizeof(int));
  if (ret != sizeof(int)) {
    // TODO: handle error
  }
  ret = slave_socket_->send(buffer, BLOCK_SIZE);
  if (ret != BLOCK_SIZE) {
    // TODO: handle error
  }
}

void MasterNodeHandler::on_socket_event(TCPSocketEvent e) {
  char msg_type;
  switch (e) {
    case TCPSOCKET_READABLE:
      if (slave_socket_->recv(&msg_type, 1) != 1) {
        // TODO: handle error
      }
      switch (msg_type) {
        case MSG_WRITE_BLOCK:
          handle_write_block();
          break;
        case MSG_REQUEST_SYNC:
          handle_sync();
          break;
        default:
          // TODO: handle error
          break;
      }
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

void MasterNodeHandler::handle_write_block() {
  int block_num;
  int ret = slave_socket_->recv((char *) &block_num, sizeof(int));
  if (ret != sizeof(int)) {
    // TODO: handle error
  }
  ret = slave_socket_->recv(buffer_, BLOCK_SIZE);
  if (ret != BLOCK_SIZE) {
    // TODO: handle error
  }
  // disk_write will broadcast update as part of write
  sdfs_->disk_write(buffer_, block_num);
}

void MasterNodeHandler::handle_sync() {
  unsigned char md4_buf[HASH_SIZE];
  char block[BLOCK_SIZE];
  int block_num;
  int ret = slave_socket_->recv((char *) &block_num, sizeof(int));
  if (ret != sizeof(int)) {
    // TODO: handle error
  }
  ret = slave_socket_->recv(buffer_, BLOCK_SIZE);
  if (ret != BLOCK_SIZE) {
    // TODO: handle error
  }

  for (int i = 0; i < BLOCK_SIZE/HASH_SIZE; i++) {
    memcpy(md4_buf, buffer_ + (i * HASH_SIZE), HASH_SIZE);
    if (memcmp(sdfs_->block_md4_[i].md4, md4_buf, HASH_SIZE)) {
      sdfs_->disk_read(block, block_num + i);
      send_block(block, block_num + i);
    }
  }
}

void MasterNodeHandler::close() {
  is_closed_ = true;
  slave_socket_->resetOnEvent();
  slave_socket_->close();
  delete slave_socket_;
}
