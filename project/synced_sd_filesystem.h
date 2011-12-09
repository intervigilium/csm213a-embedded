#ifndef SYNCED_SDFILESYSTEM_H_
#define SYNCED_SDFILESYSTEM_H_

#include "EthernetNetIf.h"
#include "mbed.h"
#include "SDFileSystem.h"

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
  SyncedSDFileSystem(PinName mosi, PinName miso, PinName sclk, PinName cs, const char* name);
  virtual int disk_initialize();
  virtual int disk_write(const char *buffer, int block_number);
  virtual int disk_read(char *buffer, int block_number);
  virtual int disk_status();
  virtual int disk_sync();
  virtual int disk_sectors();

 protected:

 private:
  bool is_master_;

};

#endif
