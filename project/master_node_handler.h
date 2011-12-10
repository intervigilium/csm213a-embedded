#ifndef MASTER_NODE_HANDLER_H_
#define MASTER_NODE_HANDLER_H_

#include "EthernetNetIf.h"
#include "synced_sd_filesystem.h"
#include "TCPSocket.h"

class SyncedSDFileSystem;

class MasterNodeHandler {
 public:
  MasterNodeHandler(SyncedSDFileSystem *sdfs, TCPSocket *slave_socket);
  ~MasterNodeHandler();

 protected:

 private:
  void on_socket_event(TCPSocketEvent e);
  void dispatch_request();
  void close();

  SyncedSDFileSystem *sdfs_;
  TCPSocket *slave_socket_;

};

#endif
