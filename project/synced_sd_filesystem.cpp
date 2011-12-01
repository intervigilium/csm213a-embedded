#include "synced_sd_filesystem.h"

SyncedSDFileSystem::SyncedSDFileSystem(PinName mosi, PinName miso, PinName sclk, PinName cs, const char* name) :
  SDFileSystem(name), _spi(mosi, miso, sclk), _cs(cs) {
}

int SyncedSDFileSystem::disk_initialize() {
  // do sync stuff, then do superclass disk_initialize
  return super::disk_initialize();
}

int SyncedSDFileSystem::disk_write(const char *buffer, int block_number) {
  return super::disk_write(buffer, block_number);
}

int SyncedSDFileSystem::disk_read(char *buffer, int block_number) {
  return super::disk_read(buffer, block_number);
}

int SyncedSDFileSystem::disk_status() {
  return super::disk_status();
}

int SyncedSDFileSystem::disk_sync() {
  return super::disk_sync();
}

int SyncedSDFileSystem::disk_sectors() {
  return super::disk_sync();
}

// PRIVATE FUNCTIONS
