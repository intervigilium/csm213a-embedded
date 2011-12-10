#ifndef SYNCED_SDFILESYSTEM_H_
#define SYNCED_SDFILESYSTEM_H_

#include "EthernetNetIf.h"
#include "ipaddr.h"
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
#define MASTER_ADDR 64
#define SYNC_FS_PORT 31415
#define BLOCK_SIZE 512
#define HASH_SIZE 16

// slave -> master messages
#define MSG_WRITE_BLOCK 0x72 // block number, 512 byte block buffer
#define MSG_REQUEST_SYNC 0x73 // block number, MD4 for block 0 to 31

// master -> slave messages
#define MSG_UPDATE_BLOCK 0x71 // block number, 512 byte block buffer
#define MSG_WRITE_SUCCESS 0x76 // block number written

struct block_hash {
  int block_num;
  char md4[HASH_SIZE];
};

struct write_event {
  int block_num;
  char data[BLOCK_SIZE];
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
  virtual void on_node_event(TCPSocketEvent e);
  virtual void on_master_event(TCPSocketEvent e);
  virtual void master_update_block(IpAddr node, int block_number, const char *buffer);
  virtual void master_broadcast_update(const char *buffer, int block_number);
  virtual int node_request_sync(int block_num, const char *block_checksums);
  virtual int node_request_write(const char *buffer, int block_number);

 private:
  bool is_master_;
  IpAddr address_;
  list<IpAddr> nodes_;
  vector<struct block_hash> block_md4_;
  /* A block that's written and not confirmed by master is dirty */
  vector<bool> dirty_;
  TCPSocket *master_socket_;
  TCPSocket *node_socket_;
};

#endif
