#include "mbed.h"
#include "nrf24ap1/nrf24ap1.h"

#define IS_MASTER 1
#define CHANNEL_ID 1
#define NUM_ITERATIONS 10

void on_master_receive(uint8_t type, uint8_t *data, int len) {
  switch (type) {
    case MESG_BROADCAST_DATA_ID:
      for (int i = 0; i < len; i++) {
        printf("%c", data[i]);
      }
      printf("\n\r");
      break;
    default:
      printf("MASTER RECEIVED MESSAGE TYPE: 0x%x\n\r", type);
      break;
  }
}

void on_slave_receive(uint8_t type, uint8_t *data, int len) {
  switch (type) {
    case MESG_BROADCAST_DATA_ID:
      for (int i = 0; i < len; i++) {
        printf("%c", data[i]);
      }
      printf("\n\r");
      break;
    default:
      printf("SLAVE RECEIVED MESSAGE TYPE: 0x%x\n\r", type);
      break;
  }
}

void do_master(Nrf24ap1::Nrf24ap1 *ap1) {
  printf("MASTER\n\r");
  char buf[13] = "pingpingping";
  ap1->SetReceiveHandler(&on_master_receive);
  ap1->OpenChannel(CHANNEL_ID, TX_DUPLEX_CHANNEL_TYPE);
  for (int i = 0; i < NUM_ITERATIONS; i++) {
    wait_ms(4);
    ap1->Send(CHANNEL_ID, (uint8_t *) buf, 13);
  }
}

void do_slave(Nrf24ap1::Nrf24ap1 *ap1) {
  printf("SLAVE\n\r");
  char buf[13] = "pongpongpong";
  ap1->SetReceiveHandler(&on_slave_receive);
  ap1->OpenChannel(CHANNEL_ID, RX_DUPLEX_CHANNEL_TYPE);
  wait_ms(2);
  for (int i = 0; i < NUM_ITERATIONS; i++) {
    wait_ms(4);
    ap1->Send(CHANNEL_ID, (uint8_t *) buf, 13);
  }
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
    // spin
  }
  return 0;
}
