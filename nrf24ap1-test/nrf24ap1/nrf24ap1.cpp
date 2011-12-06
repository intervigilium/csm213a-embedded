#include <stdlib.h>
#include <time.h>

#include "mbed.h"
#include "nrf24ap1.h"


namespace {

struct ant_packet * create_ant_packet(int length) {
  struct ant_packet *packet = (struct ant_packet *) malloc(sizeof(struct ant_packet));
  packet->data = (uint8_t *) malloc(sizeof(uint8_t) * length);
  packet->length = length;
  return packet;
}

void free_ant_packet(struct ant_packet *packet) {
  free(packet->data);
  free(packet);
}

uint8_t get_checksum(uint8_t *buf, int len) {
  uint8_t res = 0;

  for (int i = 0; i < len; i++) {
    res ^= buf[i];
  }
  return res;
}

void send_packet(Serial *port, struct ant_packet *msg) {
  uint8_t checksum = MESG_TX_SYNC ^ msg->length ^ msg->type;
  port->putc(MESG_TX_SYNC);
  port->putc(msg->length);
  port->putc(msg->type);
  for (int i = 0; i < msg->length; i++) {
    checksum ^= msg->data[i];
    port->putc(msg->data[i]);
  }
  port->putc(checksum);
}

struct ant_packet * get_assign_channel_packet(int chan_id, int chan_type) {
  struct ant_packet *packet = create_ant_packet(3);
  packet->type = MESG_ASSIGN_CHANNEL_ID;
  packet->data[0] = chan_id;
  packet->data[1] = chan_type;
  packet->data[2] = DEFAULT_NETWORK_NUMBER;
  return packet;
}

struct ant_packet * get_unassign_channel_packet(int chan_id) {
  struct ant_packet *packet = create_ant_packet(1);
  packet->type = MESG_UNASSIGN_CHANNEL_ID;
  packet->data[0] = chan_id;
  return packet;
}

struct ant_packet * get_set_channel_id_packet(int chan_id, uint16_t dev_id) {
  struct ant_packet *packet = create_ant_packet(5);
  packet->type = MESG_CHANNEL_ID_ID;
  packet->data[0] = chan_id;
  packet->data[1] = (uint8_t)((dev_id & 0xF0) >> 8);
  packet->data[2] = (uint8_t)(dev_id & 0x0F);
  packet->data[3] = DEFAULT_DEVICE_TYPE_ID;
  packet->data[4] = DEFAULT_TRANSMISSION_TYPE;
  return packet;
}

struct ant_packet * get_set_channel_rf_packet(int chan_id, uint8_t rf) {
  struct ant_packet *packet = create_ant_packet(2);
  packet->type = MESG_CHANNEL_RADIO_FREQ_ID;
  packet->data[0] = chan_id;
  packet->data[1] = rf;
  return packet;
}

struct ant_packet * get_set_channel_period_packet(int chan_id, int period) {
  // period is 32768/period Hz
  struct ant_packet *packet = create_ant_packet(3);
  packet->type = MESG_CHANNEL_MESG_PERIOD_ID;
  packet->data[0] = chan_id;
  packet->data[1] = (uint8_t)((period & 0xF0) >> 8);
  packet->data[2] = (uint8_t)(period & 0x0F);
  return packet;
}

struct ant_packet * get_open_channel_packet(int chan_id) {
  struct ant_packet *packet = create_ant_packet(1);
  packet->type = MESG_OPEN_CHANNEL_ID;
  packet->data[0] = chan_id;
  return packet;
}

struct ant_packet * get_close_channel_packet(int chan_id) {
  struct ant_packet *packet = create_ant_packet(1);
  packet->type = MESG_CLOSE_CHANNEL_ID;
  packet->data[0] = chan_id;
  return packet;
}

}

namespace Nrf24ap1 {

Nrf24ap1::Nrf24ap1(PinName tx, PinName rx, PinName ctx) {
  srand(time(NULL));
  dev_id_ = (uint16_t)(rand() % 0xFF);
  cts_pin_ = new InterruptIn(ctx);
  ap1_ = new Serial(tx, rx);
  ap1_->attach(this, &Nrf24ap1::HandleMessage, Serial::RxIrq);
  ap1_->baud(NRF24AP1_BAUD);
  ap1_->format(8, Serial::None, 1);
  msg_buf_ = NULL;
  msg_idx_ = 0;
  msg_type_ = 0;
}

void Nrf24ap1::Reset() {
  // reset packet does not require nrf24ap1 reply
  struct ant_packet *packet = create_ant_packet(1);
  packet->type = MESG_SYSTEM_RESET_ID;
  packet->data[0] = 0;
  send_packet(ap1_, packet);
}

int Nrf24ap1::OpenChannel(int chan_id, int chan_type) {
  for (list<int>::const_iterator it = channels_.begin(); it != channels_.end(); it++) {
    if (*it == chan_id) {
      printf("ERROR: Channel already open\n\r");
      return -1;
    }
  }
  QueueMessage(get_assign_channel_packet(chan_id, chan_type));
  QueueMessage(get_set_channel_id_packet(chan_id, dev_id_));
  QueueMessage(get_open_channel_packet(chan_id));
  channels_.push_back(chan_id);
  return 0;
}

void Nrf24ap1::CloseChannel(int chan_id) {
  QueueMessage(get_close_channel_packet(chan_id));
  QueueMessage(get_unassign_channel_packet(chan_id));
  channels_.remove(chan_id);
}

int Nrf24ap1::Send(int chan_id, uint8_t *buf, int len) {
  // broadcast packet does not require nrf24ap1 reply
  struct ant_packet *packet = create_ant_packet(9);
  packet->type = MESG_BROADCAST_DATA_ID;
  packet->data[0] = chan_id;
  int idx = 0;
  for (int i = 0; i < (len + 7) / 8; i++) {
    memset(packet, 0, sizeof(uint8_t) * packet->length);
    for (int j = 1; j < 9; j++) {
      // TODO: use memcpy instead here
      packet->data[j] = buf[idx++];
      if (idx >= len) {
        break;
      }
    }
    send_packet(ap1_, packet);
  }
  free_ant_packet(packet);
  return 0;
}

void Nrf24ap1::SetReceiveHandler(void (*handler)(uint8_t, uint8_t *, int)) {
  rx_handler_ = handler;
}

void Nrf24ap1::HandleMessage() {
  uint8_t c;

  while (ap1_->readable()) {
    c = ap1_->getc();
    printf("MESSAGE_HANDLER GOT: 0x%x\n\r", c);
    if (c == MESG_TX_SYNC) {
      free(msg_buf_);
      msg_idx_ = 1;
    } else {
      switch (msg_idx_) {
        case 1:
          msg_len_ = c;
          msg_buf_ = (uint8_t *) malloc(sizeof(uint8_t) * msg_len_);
          memset(msg_buf_, 0, sizeof(uint8_t) * msg_len_);
          msg_idx_++;
          break;
        case 2:
          msg_type_ = c;
          msg_idx_++;
          break;
        default:
          if (msg_idx_ == 3 + msg_len_) {
            // TODO: check/handle event messages before running callback
            (*rx_handler_)(msg_type_, msg_buf_, msg_len_);
          } else if (msg_idx_ < 3 + msg_len_) {
            msg_buf_[msg_idx_ - 3] = c;
            msg_idx_++;
          }
          break;
      }
    }
  }
}

void Nrf24ap1::QueueMessage(struct ant_packet *packet) {
  if (control_queue_.empty()) {
    // send immediately
    send_packet(ap1_, packet);
  } else {
    // queue is not empty means we are waiting on an event or reply
    control_queue_.push_back(packet);
  }
}

Nrf24ap1::~Nrf24ap1() {
  delete ap1_;
  delete cts_pin_;
}

}
