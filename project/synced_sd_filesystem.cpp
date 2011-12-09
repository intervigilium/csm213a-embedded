#include "synced_sd_filesystem.h"

SyncedSDFileSystem::SyncedSDFileSystem(IpAddr addr, bool is_master, PinName mosi, PinName miso, PinName sclk, PinName cs, const char* name) :
    SDFileSystem(mosi, miso, sclk, cs, name) {
  address_ = addr;
  is_master_ = is_master;
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

void SyncedSDFileSystem::on_node_receive() {

}

void SyncedSDFileSystem::on_master_receive() {

}

void SyncedSDFileSystem::master_broadcast_time() {

}

void SyncedSDFileSystem::master_broadcast_update(const char *buffer, int block_number) {

}

int SyncedSDFileSystem::node_request_sync(char *block_checksums) {
  return 0;
}

int SyncedSDFileSystem::node_request_write(char *buffer, int block_number) {
  return 0;
}

int SyncedSDFileSystem::node_request_update(char *updated_buffer, int block_number) {
  return 0;
}

// PRIVATE FUNCTIONS
