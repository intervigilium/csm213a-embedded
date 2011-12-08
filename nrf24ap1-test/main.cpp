#include "mbed.h"
#include "nrf24ap1/nrf24ap1.h"

#ifdef SLAVE
#define IS_MASTER 0
#else
#define IS_MASTER 1
#endif

#define CHANNEL_ID 1
#define NUM_ITERATIONS 2

struct ap1_packet * create_ap1_packet(int len) {
  struct ap1_packet *p = (struct ap1_packet *) malloc(sizeof(struct ap1_packet));
  p->data = (uint8_t *) malloc(sizeof(uint8_t) * len);
  p->length = len;
  return p;
}

void free_ap1_packet(struct ap1_packet *p) {
  if (p && p->data) {
    free(p->data);
  }
  free(p);
}

void on_master_receive(struct ap1_packet *p) {
  printf("MASTER RECEIVED MESSAGE: %s\n\r", p->data);
}

void on_slave_receive(struct ap1_packet *p) {
  printf("SLAVE RECEIVED MESSAGE: %s\n\r", p->data);
}

void do_master(Nrf24ap1::Nrf24ap1 *ap1) {
  printf("\n\r\n\rMASTER\n\r");
  struct ap1_packet *p = create_ap1_packet(13);
  sprintf((char *) p->data, "pingpingping");
  ap1->SetReceiveHandler(&on_master_receive);
  ap1->OpenChannel(CHANNEL_ID, TX_DUPLEX_CHANNEL_TYPE);
  ap1->OpenChannel(CHANNEL_ID + 1, RX_DUPLEX_CHANNEL_TYPE);
  while (1) {
    ap1->Send(CHANNEL_ID, p);
    wait_ms(250);
  }
  free_ap1_packet(p);
}

void do_slave(Nrf24ap1::Nrf24ap1 *ap1) {
  printf("\n\r\n\rSLAVE\n\r");
  struct ap1_packet *p = create_ap1_packet(13);
  sprintf((char *) p->data, "pongpongpong");
  ap1->SetReceiveHandler(&on_slave_receive);
  ap1->OpenChannel(CHANNEL_ID + 1, TX_DUPLEX_CHANNEL_TYPE);
  ap1->OpenChannel(CHANNEL_ID, RX_DUPLEX_CHANNEL_TYPE);
  while (1) {
    ap1->Send(CHANNEL_ID + 1, p);
    wait_ms(250);
  }
  free_ap1_packet(p);
}

int main() {
  Nrf24ap1::Nrf24ap1 ap1 = Nrf24ap1::Nrf24ap1(p28, p27, p29);
  ap1.Reset();
  if (IS_MASTER) {
    do_master(&ap1);
  } else {
    do_slave(&ap1);
  }
  while (1) {
    wait_ms(250);
  }
  return 0;
}
