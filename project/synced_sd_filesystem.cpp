#include "debug.h"
#include "md4.h"
#include "synced_sd_filesystem.h"

string ip_to_string(IpAddr addr) {
  char buf[17];
  sprintf(buf, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
  return string(buf);
}

SyncedSDFileSystem::SyncedSDFileSystem(IpAddr addr, bool is_master, PinName mosi, PinName miso, PinName sclk, PinName cs, const char* name) :
    SDFileSystem(mosi, miso, sclk, cs, name) {
  TCPSocketErr err;
  address_ = addr;
  is_master_ = is_master;
  tcp_socket_ = new TCPSocket();
  dirty_ = vector<bool>(BLOCK_NUM, false);
  block_md4_ = vector<struct block_hash>(BLOCK_NUM);

  for (int i = 0; i < BLOCK_NUM; ++i) {
    block_md4_[i].block_num = i;
    if (SDFileSystem::disk_read((char *)buffer_, i)) {
      // TODO: handle error
    }
    mdfour((unsigned char *) block_md4_[i].md4, buffer_, BLOCK_SIZE);
  }

  if (is_master) {
    tcp_socket_->setOnEvent(this, &SyncedSDFileSystem::on_master_event);
    tcp_socket_->bind(Host(addr, SYNC_FS_PORT));
    tcp_socket_->listen();
  } else {
    tcp_socket_->setOnEvent(this, &SyncedSDFileSystem::on_node_event);
    err = tcp_socket_->connect(Host(IpAddr(192, 168, 1, MASTER_ADDR), SYNC_FS_PORT));
    if (err) {
      // TODO: retry master registration periodically
      tcp_socket_->close();
      delete tcp_socket_;
      printf("SLAVE: failed to connect to master, entering standalone mode\n\r");
    } else {
      debug("SLAVE: connected to master");
    }
  }
}

SyncedSDFileSystem::~SyncedSDFileSystem() {
  if (tcp_socket_) {
    tcp_socket_->close();
    delete tcp_socket_;
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

/*
 * Assume write success and return automatically.
 */
int SyncedSDFileSystem::disk_write(const char *buffer, int block_number) {
  int ret;

  if (is_master_) {
    ret = SDFileSystem::disk_write(buffer, block_number);
    master_broadcast_update(buffer, block_number);
  } else {
    dirty_[block_number] = true;
    ret = SDFileSystem::disk_write(buffer, block_number);
    node_request_write(buffer, block_number);
  }
  return ret;
}

int SyncedSDFileSystem::disk_read(char *buffer, int block_number) {
  while (dirty_[block_number]) {
    wait_us(5); //FIXME
  }
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

// PRIVATE FUNCTIONS

void SyncedSDFileSystem::on_node_event(TCPSocketEvent e) {
  char msg_type;

  switch (e) {
    case TCPSOCKET_CONNECTED:
      // now connected, do nothing, assume connection is constant
      break;
    case TCPSOCKET_READABLE:
      if (tcp_socket_->recv(&msg_type, 1) != 1) {
        // TODO: handle error
        break;
      }
      switch (msg_type) {
        case MSG_UPDATE_BLOCK:
          node_handle_update_block();
          break;
        case MSG_WRITE_SUCCESS:
          node_handle_write_success();
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
  MasterNodeHandler *dispatcher;

  switch (e) {
    case TCPSOCKET_ACCEPT:
      err = tcp_socket_->accept(&slave, &slave_socket);
      if (err) {
        // TODO: handle errors
      }
      dispatcher = new MasterNodeHandler(this, slave_socket);
      node_handlers_.insert(pair<string, MasterNodeHandler *>(ip_to_string(slave.getIp()), dispatcher));
      // dispatcher should destroy self when done or disconnected
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

void SyncedSDFileSystem::master_broadcast_update(const char *buffer, int block_number) {
  debug("MASTER: broadcasting update of block %d", block_number);

  map<string, MasterNodeHandler *>::const_iterator it;
  for (it = node_handlers_.begin(); it != node_handlers_.end(); it++) {
    (*it).second->send_block(buffer, block_number);
  }
}

int SyncedSDFileSystem::node_request_sync(int block_num, const char *block_checksums) {
  // send checksums for block_number to block_num+31 to master
  // master replies with MSG_UPDATE_BLOCK for blocks that need updating
  debug("SLAVE: requesting sync starting at block %d", block_number);

  int ret;
  char msg_type = MSG_REQUEST_SYNC;

  ret = tcp_socket_->send(&msg_type, 1);
  if (ret != 1) {
    // TODO: handle error
  }

  // send a dummy block number
  ret = tcp_socket_->send((char *)&ret, sizeof(int));
  if (ret != sizeof(int)) {
    // TODO: handle error
  }

  /* FIXME: 32 * 16 = 512 super clowny
   * put all hashes into buffer_ and send over at once
   */
  for (int i = 0; i < BLOCK_SIZE/HASH_SIZE; i++) {
    memcpy(buffer_ + (i * HASH_SIZE), block_md4_[block_number + i].md4, HASH_SIZE);
  }
  ret = tcp_socket_->send((char *)buffer_, BLOCK_SIZE);
  if (ret != BLOCK_SIZE) {
    // TODO: handle error
  }
  return 0;
}

int SyncedSDFileSystem::node_request_write(const char *buffer, int block_number) {
  // send request to write buffer to block_number
  // master replies with MSG_WRITE_SUCCESS or MSG_WRITE_FAIL
  debug("SLAVE: requesting write to block %d", block_number);

  int ret;
  char msg_type = MSG_WRITE_BLOCK;

  ret = tcp_socket_->send(&msg_type, 1);
  if (ret != 1) {
    // TODO: handle error
  }

  ret = tcp_socket_->send((char *)&block_number, sizeof(int));
  if (ret != sizeof(int)) {
    // TODO: handle error
  }

  ret = tcp_socket_->send(buffer, BLOCK_SIZE);
  if (ret != BLOCK_SIZE) {
    // TODO: handle error
  }
  return 0;
}

void SyncedSDFileSystem::node_handle_update_block() {
  debug("SLAVE: handling UPDATE_BLOCK");

  int ret;
  int block_num;

  ret = tcp_socket_->recv((char *) &block_num, sizeof(int));
  if (ret != sizeof(int)) {
    // TODO: handle error
  }

  ret = tcp_socket_->recv((char *) buffer_, BLOCK_SIZE);
  if (ret != BLOCK_SIZE) {
    // TODO: handle error
  }

  ret = SDFileSystem::disk_write((char *)buffer_, block_num);
  if (ret) {
    // TODO: handle error
  }

  mdfour((unsigned char*)block_md4_[block_num].md4, buffer_, BLOCK_SIZE);
  dirty_[block_num] = false;
}

void SyncedSDFileSystem::node_handle_write_success() {
  debug("SLAVE: handling WRITE_SUCCESS");

  int ret;
  int block_num;

  ret = tcp_socket_->recv((char *) &block_num, sizeof(int));
  if (ret != sizeof(int)) {
    // TODO: handle error
  }

  dirty_[block_num] = false;
}
