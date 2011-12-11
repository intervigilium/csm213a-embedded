#include "EthernetNetIf.h"
#include "mbed.h"
#include "synced_sd_filesystem.h"

#ifdef SLAVE
#define IS_MASTER 0
#else
#define IS_MASTER 1
#endif
#define NAME_FILE "/sd/spartacus.lock"

Serial pc(USBTX, USBRX);
DigitalOut master_led(LED1);
DigitalOut slave_led(LED2);
EthernetNetIf *eth;
SyncedSDFileSystem *fs;
Ticker function_ticker;


void write_name_file() {
  pc.printf("SLAVE: writing\n\r");
  IpAddr addr = eth->getIp();
  FILE *fp = fopen(NAME_FILE, "w");
  pc.printf("SLAVE: opened file\n\r");
  if (fp != NULL) {
    pc.printf("SLAVE: valid fp\n\r");
    fprintf(fp, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
    pc.printf("SLAVE: wrote file\n\r");
    fclose(fp);
    pc.printf("SLAVE: close fp\n\r");
  }
  pc.printf("SLAVE: write completed\n\r");
}

void read_name_file() {
  pc.printf("MASTER: reading\n\r");
  uint8_t q0, q1, q2, q3;
  FILE *fp = fopen(NAME_FILE, "r");
  pc.printf("MASTER: opened file\n\r");
  if (fp != NULL) {
    pc.printf("MASTER: valid fp\n\r");
    fscanf(fp, "%d.%d.%d.%d", &q0, &q1, &q2, &q3);
    pc.printf("%d.%d.%d.%d is Spartacus!\n\r", &q0, &q1, &q2, &q3);
    fclose(fp);
    pc.printf("MASTER: close fp\n\r");
  }
  pc.printf("MASTER: read completed\n\r");
}

void do_slave() {
  slave_led = 1;
  srand(time(NULL));
  fs = new SyncedSDFileSystem(eth->getIp(), false, p5, p6, p7, p8, "sd");
  function_ticker.attach(&write_name_file, rand() % 8 + 1);
  while (1) {
    // EthernetNetIf sockets all rely on polling
    Net::poll();
  }
}

void do_master() {
  master_led = 1;
  fs = new SyncedSDFileSystem(eth->getIp(), true, p5, p6, p7, p8, "sd");
  function_ticker.attach(&read_name_file, 5);
  while (1) {
    // EthernetNetIf sockets all rely on polling
    Net::poll();
  }
}

int main() {
  IpAddr ip;
  //pc.baud(115200);
  if (IS_MASTER) {
    eth = new EthernetNetIf(
        IpAddr(192,168,1,164), // ip
        IpAddr(255,255,255,0), // subnet
        IpAddr(192,168,1,1), // gateway
        IpAddr(192,168,1,1) // dns
      );
    eth->setup();
    ip = eth->getIp();
    printf("\n\r\n\rMASTER: %d.%d.%d.%d\n\r", ip[0], ip[1], ip[2], ip[3]);
    do_master();
  } else {
    eth = new EthernetNetIf();
    eth->setup();
    ip = eth->getIp();
    printf("\n\r\n\rSLAVE: %d.%d.%d.%d\n\r", ip[0], ip[1], ip[2], ip[3]);
    do_slave();
  }
  return 0;
}
