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

void inline send_packet(Serial *port, uint8_t type, uint8_t *packet, int len) {
  uint8_t checksum = MESG_TX_SYNC ^ len ^ type;
  port->putc(MESG_TX_SYNC);
  port->putc(len);
  port->putc(type);
  for (int i = 0; i < len; i++) {
    checksum ^= packet[i];
    port->putc(packet[i]);
  }
  port->putc(checksum);
}

void send_assign_channel(Serial *port, int chan_id, int chan_type) {
  uint8_t packet[3];
  packet[0] = chan_id;
  packet[1] = chan_type;
  packet[2] = DEFAULT_NETWORK_NUMBER;
  send_packet(port, MESG_ASSIGN_CHANNEL_ID, packet, 3);
}

void send_unassign_channel(Serial *port, int chan_id) {
  uint8_t packet[1];
  packet[0] = chan_id;
  send_packet(port, MESG_UNASSIGN_CHANNEL_ID, packet, 1);
}

void send_set_channel_id(Serial *port, int chan_id, uint16_t dev_id) {
  uint8_t packet[5];
  packet[0] = chan_id;
  packet[1] = (uint8_t)((dev_id & 0xF0) >> 8);
  packet[2] = (uint8_t)(dev_id & 0x0F);
  packet[3] = DEFAULT_DEVICE_TYPE_ID;
  packet[4] = DEFAULT_TRANSMISSION_TYPE;
  send_packet(port, MESG_CHANNEL_ID_ID, packet, 5);
}

void send_set_channel_rf(Serial *port, int chan_id, uint8_t rf) {
  uint8_t packet[2];
  packet[0] = chan_id;
  packet[1] = rf;
  send_packet(port, MESG_CHANNEL_RADIO_FREQ_ID, packet, 2);
}

void send_set_channel_period(Serial *port, int chan_id, int period) {
  // period is 32768/period Hz
  uint8_t packet[3];
  packet[0] = chan_id;
  packet[1] = (uint8_t)((period & 0xF0) >> 8);
  packet[2] = (uint8_t)(period & 0x0F);
  send_packet(port, MESG_CHANNEL_MESG_PERIOD_ID, packet, 3);
}

void send_open_channel(Serial *port, int chan_id) {
  uint8_t packet[1];
  packet[0] = chan_id;
  send_packet(port, MESG_OPEN_CHANNEL_ID, packet, 1);
}

void send_close_channel(Serial *port, int chan_id) {
  uint8_t packet[1];
  packet[0] = chan_id;
  send_packet(port, MESG_CLOSE_CHANNEL_ID, packet, 1);
}

}

namespace Nrf24ap1 {

Nrf24ap1::Nrf24ap1(PinName tx, PinName rx, PinName ctx) {
  srand(time(NULL));
  dev_id_ = (uint16_t)(rand() % 0xFF);
  ap1_ = new Serial(tx, rx);
  cts_pin_ = new InterruptIn(ctx);
  ap1_->attach(this, &Nrf24ap1::HandleMessage, Serial::RxIrq);
  ap1_->baud(NRF24AP1_BAUD);
  msg_idx_ = 0;
  msg_type_ = 0;
}

void Nrf24ap1::Reset() {
  uint8_t packet[1];
  packet[0] = 0;
  send_packet(ap1_, MESG_SYSTEM_RESET_ID, packet, 1);
}

int Nrf24ap1::OpenChannel(int chan_id, int chan_type) {
  for (list<int>::const_iterator it = channels_.begin(); it != channels_.end(); it++) {
    if (*it == chan_id) {
      printf("ERROR: Channel already open\n\r");
      return -1;
    }
  }
  send_assign_channel(ap1_, chan_id, chan_type);
  wait_ms(50);
  send_set_channel_rf(ap1_, chan_id, DEFAULT_CHANNEL_FREQ);
  wait_ms(50);
  send_set_channel_id(ap1_, chan_id, dev_id_);
  wait_ms(50);
  // TODO(echen): set channel period, RF, timeout?
  send_open_channel(ap1_, chan_id);
  wait_ms(50);
  channels_.push_back(chan_id);
  return 0;
}

void Nrf24ap1::CloseChannel(int chan_id) {
  send_close_channel(ap1_, chan_id);
  wait_ms(50);
  send_unassign_channel(ap1_, chan_id);
  wait_ms(50);
  channels_.remove(chan_id);
}

int Nrf24ap1::Send(int chan_id, uint8_t *buf, int len) {
  int idx = 0;
  uint8_t packet[9];
  packet[0] = chan_id;

  for (int i = 0; i < (len + 7) / 8; i++) {
    for (int j = 1; j < 9; j++) {
      packet[j] = buf[idx++];
      if (idx >= len) {
        break;
      }
    }
    send_packet(ap1_, MESG_BROADCAST_DATA_ID, packet, 9);
  }
  return 0;
}

void Nrf24ap1::SetReceiveHandler(void (*handler)(uint8_t, uint8_t *, int)) {
  rx_handler_ = handler;
}

void Nrf24ap1::HandleMessage() {
  // check what data is in serial
  // build data, send it to rx_handler if complete message
  uint8_t c = ap1_->getc();
  switch (msg_idx_) {
    case 0:
      if (c != MESG_TX_SYNC) {
        // skip bad character
        printf("ERROR: Badly formatted message!\n\r");
      } else {
        msg_idx_++;
      }
      break;
    case 1:
      msg_len_ = c;
      msg_buf_ = (uint8_t *) malloc(sizeof(uint8_t) * msg_len_);
      msg_idx_++;
      break;
    case 2:
      msg_type_ = c;
      msg_idx_++;
      break;
    default:
      if (msg_idx_ == 3 + msg_len_) {
        (*rx_handler_)(msg_type_, msg_buf_, msg_len_);
        // clean up after handling message
        free(msg_buf_);
        msg_idx_ = 0;
      } else {
        msg_buf_[msg_idx_ - 3] = c;
        msg_idx_++;
      }
      break;
  }
}

Nrf24ap1::~Nrf24ap1() {
  delete ap1_;
  delete cts_pin_;
}

}
