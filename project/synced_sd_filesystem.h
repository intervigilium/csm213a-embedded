#ifndef SYNCED_SDFILESYSTEM_H_
#define SYNCED_SDFILESYSTEM_H_

#include <map>
#include <string>

#include "EthernetNetIf.h"
#include "fs_constants.h"
#include "host.h"
#include "ipaddr.h"
#include "master_node_handler.h"
#include "mbed.h"
#include "SDFileSystem.h"
#include "TCPSocket.h"

/** Access the filesystem on an SD Card using SPI
 *
 * @code
 * #include "mbed.h"
 * #include "SDFileSystem.h"
 *
 * SyncedSDFileSystem sd(p5, p6, p7, p12, "sd"); // mosi, miso, sclk, cs
 *
 * int main() {
 *     FILE *fp = fopen("/sd/myfile.txt", "w");
 *     fprintf(fp, "Hello World!\n");
 *     fclose(fp);
 * }
 */

// TODO: dynamic discovery of master
class MasterNodeHandler;

struct block_hash {
  int block_num;
  unsigned char md4[HASH_SIZE];
};

struct write_event {
  int block_num;
  unsigned char data[BLOCK_SIZE];
};

class SyncedSDFileSystem : public SDFileSystem {
 public:
  /** Create the File System for accessing an SD Card using SPI
   *
   * @param mosi SPI mosi pin connected to SD Card
   * @param miso SPI miso pin conencted to SD Card
   * @param sclk SPI sclk pin connected to SD Card
   * @param cs   DigitalOut pin used as SD Card chip select
   * @param name The name used to access the virtual filesystem
   */
  SyncedSDFileSystem(IpAddr addr, bool is_master, PinName mosi, PinName miso, PinName sclk, PinName cs, const char* name);
  ~SyncedSDFileSystem();

  virtual int rename(const char *oldname, const char *newname);
  virtual int mkdir(const char *name, mode_t mode);

  virtual int disk_initialize();
  virtual int disk_write(const char *buffer, int block_number);
  virtual int disk_read(char *buffer, int block_number);
  virtual int disk_status();
  virtual int disk_sync();
  virtual int disk_sectors();

 protected:

 private:
  void on_node_event(TCPSocketEvent e);
  void on_master_event(TCPSocketEvent e);
  void master_broadcast_update(const char *buffer, int block_number);
  int node_request_sync(int block_number, const char *block_checksums);
  int node_request_write(const char *buffer, int block_number);
  void node_handle_update_block();
  void node_handle_write_success();

  bool is_master_;
  IpAddr address_;
  map<string, MasterNodeHandler *> node_handlers_;
  vector<struct block_hash> block_md4_;
  /* A block that's written and not confirmed by master is dirty */
  vector<bool> dirty_;
  TCPSocket *tcp_socket_;

  unsigned char buffer_[BLOCK_SIZE];
  friend class MasterNodeHandler;
};

#endif
