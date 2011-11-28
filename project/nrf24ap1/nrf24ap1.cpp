#include <stdlib.h>
#include <time.h>

#include "mbed.h"
#include "nrf24ap1.h"


namespace {

uint8_t get_checksum(uint8_t *buf, int len) {
  uint8_t res = 0;

  for (int i = 0; i < len; i++) {
    res ^= buf[i];
  }
  return res;
}

void inline send_packet(Serial *port, uint8_t *packet, int len) {
  for (int i = 0; i < len; i++) {
    port->putc(packet[i]);
  }
}

void send_assign_channel(Serial *port, int chan_id, int chan_type) {
  uint8_t packet[7];
  packet[0] = MESG_TX_SYNC;
  packet[1] = 3;
  packet[2] = MESG_ASSIGN_CHANNEL_ID;
  packet[3] = chan_id;
  packet[4] = chan_type;
  packet[5] = DEFAULT_NETWORK_NUMBER;
  packet[6] = get_checksum(&packet, 6);
  send_packet(port, &packet, 7);
}

void send_unassign_channel(Serial *port, int chan_id) {
  uint8_t packet[5];
  packet[0] = MESG_TX_SYNC;
  packet[1] = 1;
  packet[2] = MESG_UNASSIGN_CHANNEL_ID;
  packet[3] = chan_id;
  packet[4] = get_checksum(&packet, 4);
  send_packet(port, &packet, 5);
}

void send_set_channel_id(Serial *port, int chan_id, uint16_t dev_id) {
  uint8_t packet[9];
  packet[0] = MESG_TX_SYNC;
  packet[1] = 5;
  packet[2] = MESG_CHANNEL_ID_ID;
  packet[3] = chan_id;
  packet[4] = (uint8_t)((dev_id & 0xF0) >> 8);
  packet[5] = (uint8_t)(dev_id & 0x0F);
  packet[6] = DEFAULT_DEVICE_TYPE_ID;
  packet[7] = DEFAULT_TRANSMISSION_TYPE;
  packet[8] = get_checksum(&packet, 8);
  send_packet(port, &packet, 9);
}

void send_open_channel(Serial *port, int chan_id) {
  uint8_t packet[5];
  packet[0] = MESG_TX_SYNC;
  packet[1] = 1;
  packet[2] = MESG_OPEN_CHANNEL_ID;
  packet[3] = chan_id;
  packet[4] = get_checksum(&packet, 4);
  send_packet(port, &packet, 5);
}

}

namespace Nrf24ap1 {

Nrf24ap1::Nrf24ap1(PinName tx, PinName rx, PinName ctx) {
  srand(time(NULL));
  dev_id_ = (uint16_t)(rand() % 0xFF);
  ap1_ = new Serial(tx, rx);
  cts_pin_ = new InterruptIn(ctx);
  ap1_->attach(this, &HandleMessage, Serial:RxIrq);
  ap1_->baud(NRF24AP1_BAUD);
}

void Nrf24ap1::Reset() {
  uint8_t packet[5];
  packet[0] = MESG_TX_SYNC;
  packet[1] = 1;
  packet[2] = MESG_SYSTEM_RESET_ID;
  packet[3] = 0;
  packet[4] = get_checksum(&packet, 4);
  send_packet(port, &packet, 5);
}

int Nrf24ap1::OpenChannel(int chan_id) {
  // assign channel
  wait_ms(50);
  // set channel id
  wait_ms(50);
  // open channel

  channels_.push_back(chan_id);
}

void Nrf24ap1::CloseChannel(int chan_id) {

}

int Nrf24ap1::Send(int channel_id, uint8_t *buf, int len) {
  // use BroadcastData to push data to channel_id

}

void Nrf24ap1::HandleMessage() {
  // check what data is in serial
  // build data, send it to rx_handler if complete message
}

Nrf24ap1::~Nrf24ap1() {
  delete ap1_;
  delete cts_pin_;
}

}
