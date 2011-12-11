#include "debug.h"
#include "ipaddr.h"
#include "master_node_handler.h"

MasterNodeHandler::MasterNodeHandler(SyncedSDFileSystem *sdfs, Host slave, TCPSocket *slave_socket) 
    : NetService(),
      slave_(slave),
      slave_socket_(slave_socket),
      timeout_watchdog_(),
      is_closed_(false) {
  slave_socket_->setOnEvent(this, &MasterNodeHandler::on_socket_event);
}

MasterNodeHandler::~MasterNodeHandler() {
  close();
}

bool MasterNodeHandler::is_closed() {
  return is_closed_;
}

void MasterNodeHandler::send_block(const char *buffer, int block_number) {
  IpAddr ip = slave_.getIp();
  debug("MASTER: UPDATE_BLOCK to %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

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
      timeout_watchdog_.detach();
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
      set_timeout(ECHO_TIMEOUT);
      break;
    case TCPSOCKET_WRITEABLE:
      break;
    case TCPSOCKET_CONTIMEOUT:
    case TCPSOCKET_CONRST:
    case TCPSOCKET_CONABRT:
    case TCPSOCKET_ERROR:
    case TCPSOCKET_DISCONNECTED:
      // TODO: handle error
      close();
      break;
    case TCPSOCKET_ACCEPT:
    default:
      // these should never happen
      break;
  }
}

void MasterNodeHandler::handle_write_block() {
  IpAddr ip = slave_.getIp();
  debug("MASTER: WRITE_BLOCK from %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

  int ret;
  int block_num;

  ret = slave_socket_->recv((char *) &block_num, sizeof(int));
  if (ret != sizeof(int)) {
    // TODO: handle error
  }

  ret = slave_socket_->recv(buffer_, BLOCK_SIZE);
  if (ret != BLOCK_SIZE) {
    // TODO: handle error
  }

  // disk_write will broadcast update as part of write
  sdfs_->disk_write(buffer_, block_num);

  // TODO: send write success/fail back to node
}

void MasterNodeHandler::handle_sync() {
  IpAddr ip = slave_.getIp();
  debug("MASTER: REQUEST_SYNC from %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

  int ret;
  int block_num;
  unsigned char md4_buf[HASH_SIZE];
  char block[BLOCK_SIZE];

  ret = slave_socket_->recv((char *) &block_num, sizeof(int));
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
  if (is_closed_) {
    return;
  }
  timeout_watchdog_.detach();
  is_closed_ = true;
  if (slave_socket_) {
    slave_socket_->resetOnEvent();
    slave_socket_->close();
    delete slave_socket_;
  }
  NetService::close();
}

void MasterNodeHandler::set_timeout(int timeout) {
  timeout_watchdog_.attach(this, &MasterNodeHandler::on_timeout, ECHO_TIMEOUT);
}

void MasterNodeHandler::on_timeout() {
  close();
}
