#include "synced_sd_filesystem.h"

SyncedSDFileSystem::SyncedSDFileSystem(IpAddr addr, bool is_master, PinName mosi, PinName miso, PinName sclk, PinName cs, const char* name) :
    SDFileSystem(mosi, miso, sclk, cs, name) {
  address_ = addr;
  is_master_ = is_master;
  TCPSocketErr err;
  if (is_master) {
    master_socket_ = new TCPSocket();
    master_socket_->setOnEvent(this, &SyncedSDFileSystem::on_master_event);
    master_socket_->bind(Host(addr, SYNC_FS_PORT));
    master_socket_->listen();
  } else {
    node_socket_ = new TCPSocket();
    node_socket_->setOnEvent(this, &SyncedSDFileSystem::on_node_event);
    err = node_socket_->connect(Host(IpAddr(192, 168, 1, MASTER_ADDR), SYNC_FS_PORT));
    if (err) {
      // TODO: retry master registration periodically
      node_socket_->close();
      delete node_socket_;
    }
  }
}

SyncedSDFileSystem::~SyncedSDFileSystem() {
  if (is_master_) {
    master_socket_->close();
    delete master_socket_;
  } else {
    node_socket_->close();
    delete node_socket_;
  }
}

int SyncedSDFileSystem::disk_initialize() {
  // do sync stuff, then do SyncedSDFileSystemclass disk_initialize
  return SDFileSystem::disk_initialize();
}

int SyncedSDFileSystem::rename(const char *oldname, const char *newname) {
  return SDFileSystem::rename(oldname, newname);
}

int SyncedSDFileSystem::mkdir(const char *name, mode_t mode) {
  return SDFileSystem::mkdir(name, mode);
}

int SyncedSDFileSystem::disk_write(const char *buffer, int block_number) {
  // TODO: make this call sleep until write comes back
  return SDFileSystem::disk_write(buffer, block_number);
}

int SyncedSDFileSystem::disk_read(char *buffer, int block_number) {
  return SDFileSystem::disk_read(buffer, block_number);
}

int SyncedSDFileSystem::disk_status() {
  return SDFileSystem::disk_status();
}

int SyncedSDFileSystem::disk_sync() {
  return SDFileSystem::disk_sync();
}

int SyncedSDFileSystem::disk_sectors() {
  return SDFileSystem::disk_sync();
}

// PROTECTED FUNCTIONS

void SyncedSDFileSystem::on_node_event(TCPSocketEvent e) {
  int ret;
  int block_num;
  char msg_type;
  char *buf;
  struct write_event *ev;
  switch (e) {
    case TCPSOCKET_CONNECTED:
      // now connected, do nothing, assume connection is constant
      break;
    case TCPSOCKET_READABLE:
      ret = node_socket_->recv(&msg_type, 1);
      if (ret != 1) {
        // TODO: handle error
        break;
      }
      switch (msg_type) {
        case MSG_UPDATE_BLOCK:
          ret = node_socket_->recv((char *) &block_num, sizeof(int));
          if (ret != sizeof(int)) {
            // TODO: handle error
          }
          buf = (char *) malloc(sizeof(char) * BLOCK_SIZE);
          ret = node_socket_->recv((char *) buf, BLOCK_SIZE);
          if (ret != BLOCK_SIZE) {
            // TODO: handle error
          }
          ret = SDFileSystem::disk_write(buf, block_num);
          if (ret) {
            // TODO: handle error
          }
          free(buf);
          break;
        case MSG_WRITE_SUCCESS:
          ret = node_socket_->recv((char *) &block_num, sizeof(int));
          if (ret != sizeof(int)) {
            // TODO: handle error
          }
          if (node_write_queue_.empty()) {
            // TODO: handle error
          }
          ev = &node_write_queue_.front();
          if (ev->block_num == block_num) {
            ret = SDFileSystem::disk_write(ev->data, ev->block_num);
            node_write_queue_.pop_front();
            // TODO: trigger SyncedSDFileSystem::disk_write to return ret
          } else {
            // TODO: trigger SyncedSDFileSystem::disk_write to return -1
          }
          break;
        case MSG_WRITE_FAIL:
          ret = node_socket_->recv((char *) &block_num, sizeof(int));
          if (ret != sizeof(int)) {
            // TODO: handle error
          }
          if (node_write_queue_.empty()) {
            // TODO: handle error
          }
          ev = &node_write_queue_.front();
          if (ev->block_num == block_num) {
            node_write_queue_.pop_front();
            free(ev);
            // TODO: trigger SyncedSDFileSystem::disk_write to return -1
          } else {
            // TODO: trigger SyncedSDFileSystem::disk_write to return -1
          }
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
      // TODO: handle errors
      break;
    case TCPSOCKET_ACCEPT:
    default:
      // these should never happen
      break;
  }
}

void SyncedSDFileSystem::on_master_event(TCPSocketEvent e) {
  TCPSocketErr err;
  Host slave;
  TCPSocket *slave_socket;
  switch (e) {
    case TCPSOCKET_ACCEPT:
      err = master_socket_->accept(&slave, &slave_socket);
      if (err) {
        // TODO: handle errors
      }
      // dispatch to master_request_dispatcher
      break;
    case TCPSOCKET_CONTIMEOUT:
    case TCPSOCKET_CONRST:
    case TCPSOCKET_CONABRT:
    case TCPSOCKET_ERROR:
    case TCPSOCKET_DISCONNECTED:
      // TODO: handle errors
      break;
    case TCPSOCKET_CONNECTED:
    case TCPSOCKET_READABLE:
    case TCPSOCKET_WRITEABLE:
     default:
      // these should never happen
      break;
  }
}

void SyncedSDFileSystem::master_update_block(IpAddr node, int block_number, const char *buffer) {
  // send MSG_UPDATE_BLOCK to node

}

void SyncedSDFileSystem::master_broadcast_update(const char *buffer, int block_number) {
  // send MSG_UPDATE_BLOCK to all slaves

}

int SyncedSDFileSystem::node_request_sync(int block_num, const char *block_checksums) {
  // send checksums for block_num to block_num+31 to master
  // master replies with MSG_UPDATE_BLOCK for blocks that need updating
  return 0;
}

int SyncedSDFileSystem::node_request_write(const char *buffer, int block_number) {
  // send request to write buffer to block_number
  // master replies with MSG_WRITE_SUCCESS or MSG_WRITE_FAIL
  return 0;
}

// PRIVATE FUNCTIONS
