#include <stdlib.h>
#include <time.h>

#include "../debug.h"
#include "mbed.h"
#include "nrf24ap1.h"


namespace {

struct ant_packet * create_ant_packet(int len) {
  struct ant_packet *packet = (struct ant_packet *) malloc(sizeof(struct ant_packet));
  packet->data = (uint8_t *) malloc(sizeof(uint8_t) * len);
  memset(packet->data, 0, sizeof(uint8_t) * len);
  packet->length = len;
  return packet;
}

void free_ant_packet(struct ant_packet *packet) {
  free(packet->data);
  free(packet);
}

struct ap1_packet * create_ap1_packet(int len) {
  struct ap1_packet *packet = (struct ap1_packet *) malloc(sizeof(struct ap1_packet));
  packet->data = (uint8_t *) malloc(sizeof(uint8_t) * len);
  memset(packet->data, 0, sizeof(uint8_t) * len);
  packet->length = len;
  return packet;
}

void free_ap1_packet(struct ap1_packet *packet) {
  if (packet && packet->data) {
    free(packet->data);
  }
  free(packet);
}

void print_ap1_packet(struct ap1_packet *p) {
  printf("PACKET: ");
  for (int i = 0; i < p->length; i++) {
    printf("%c", p->data[i]);
  }
  printf("\n\r");
}

uint8_t get_checksum(uint8_t *buf, int len, uint8_t type) {
  uint8_t res = MESG_TX_SYNC ^ len ^ type;
  for (int i = 0; i < len; i++) {
    res ^= buf[i];
  }
  return res;
}

void send_packet(Serial *port, struct ant_packet *msg) {
  uint8_t checksum = get_checksum(msg->data, msg->length, msg->type);
  port->putc(MESG_TX_SYNC);
  port->putc(msg->length);
  port->putc(msg->type);
  for (int i = 0; i < msg->length; i++) {
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

struct ant_packet * get_set_channel_id_packet(int chan_id) {
  struct ant_packet *packet = create_ant_packet(5);
  packet->type = MESG_CHANNEL_ID_ID;
  packet->data[0] = chan_id;
  packet->data[1] = (uint8_t)(MASTER_DEVICE_ID & 0x00FF);
  packet->data[2] = (uint8_t)((MASTER_DEVICE_ID & 0xFF00) >> 8);
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
  packet->data[1] = (uint8_t)(period & 0x00FF);
  packet->data[2] = (uint8_t)((period & 0xFF00) >> 8);
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
  ap1_->attach(this, &Nrf24ap1::OnAp1Rx, Serial::RxIrq);
  ap1_->baud(NRF24AP1_BAUD);
  ap1_->format(8, Serial::None, 1);
  msg_len_ = 0;
  msg_type_ = 0;
  msg_buf_ = NULL;
  msg_idx_ = 99;
  ap1_packet_buf_ = NULL;
  ap1_idx_ = 99;
}

uint16_t Nrf24ap1::GetDeviceId() {
  // randomly generated each time
  return dev_id_;
}

void Nrf24ap1::Reset() {
  struct ant_packet *packet = create_ant_packet(1);
  packet->type = MESG_SYSTEM_RESET_ID;
  packet->data[0] = 0;
  QueueMessage(packet);
}

int Nrf24ap1::OpenChannel(int chan_id, int chan_type) {
  for (list<int>::const_iterator it = channels_.begin(); it != channels_.end(); it++) {
    if (*it == chan_id) {
      error("Channel already open\n\r");
      return -1;
    }
  }
  QueueMessage(get_assign_channel_packet(chan_id, chan_type));
  QueueMessage(get_set_channel_id_packet(chan_id));
  QueueMessage(get_open_channel_packet(chan_id));
  channels_.push_back(chan_id);
  return 0;
}

void Nrf24ap1::CloseChannel(int chan_id) {
  QueueMessage(get_close_channel_packet(chan_id));
  QueueMessage(get_unassign_channel_packet(chan_id));
  channels_.remove(chan_id);
}

int Nrf24ap1::Send(int chan_id, struct ap1_packet *packet) {
  struct ant_packet *ant_packet = NULL;
  int offset = 0;
  int num_ant_packets = (packet->length + 7) / 8;

  // send ap1_packet header data
  ant_packet = create_ant_packet(9);
  ant_packet->type = MESG_BROADCAST_DATA_ID;
  ant_packet->data[0] = chan_id;
  ant_packet->data[1] = AP1_PACKET_SYNC_ID;
  ant_packet->data[2] = 0;
  ant_packet->data[3] = (uint8_t)(packet->source & 0xFF00) >> 8;
  ant_packet->data[4] = (uint8_t)(packet->source & 0x00FF);
  ant_packet->data[5] = (uint8_t)(packet->destination & 0xFF00) >> 8;
  ant_packet->data[6] = (uint8_t)(packet->destination & 0x00FF);
  ant_packet->data[7] = (uint8_t)(packet->length & 0xFF00) >> 8;
  ant_packet->data[8] = (uint8_t)(packet->length & 0x00FF);
  QueueMessage(ant_packet);

  // send ap1_packet data
  for (int i = 0; i < num_ant_packets; i++) {
    ant_packet = create_ant_packet(9);
    ant_packet->type = MESG_BROADCAST_DATA_ID;
    ant_packet->data[0] = chan_id;
    ant_packet->data[1] = AP1_PACKET_DATA_ID;
    if (i == num_ant_packets - 1) {
      memcpy(ant_packet->data + 2, packet->data + offset, packet->length % 7);
    } else {
      memcpy(ant_packet->data + 2, packet->data + offset, 7);
      offset += 7;
    }
    QueueMessage(ant_packet);
  }
  return 0;
}

void Nrf24ap1::SetReceiveHandler(void (*handler)(struct ap1_packet *)) {
  rx_handler_ = handler;
}

void Nrf24ap1::OnAp1Rx() {
  uint8_t c, checksum;
  while (ap1_->readable()) {
    c = ap1_->getc();
    switch (msg_idx_) {
      case 1:
        debug("MSG_HANDLER: len 0x%x", c);
        msg_len_ = c;
        msg_buf_ = (uint8_t *) malloc(sizeof(uint8_t) * msg_len_);
        memset(msg_buf_, 0, sizeof(uint8_t) * msg_len_);
        msg_idx_++;
        break;
      case 2:
        debug("MSG_HANDLER: type 0x%x", c);
        msg_type_ = c;
        msg_idx_++;
        break;
      default:
        if (msg_idx_ == 3 + msg_len_) {
          debug("MSG_HANDLER: end at %d of %d", msg_idx_, 3 + msg_len_);
          checksum = get_checksum(msg_buf_, msg_len_, msg_type_);
          if (checksum != c) {
            printf("ERROR: Expected checksum: 0x%x, got: 0x%x\n\r", c, checksum);
          }
          switch (msg_type_) {
            case MESG_BROADCAST_DATA_ID:
            case MESG_ACKNOWLEDGED_DATA_ID:
            case MESG_BURST_DATA_ID:
              HandleAp1DataMessage(msg_type_, msg_buf_, msg_len_);
              break;
            case MESG_RESPONSE_EVENT_ID:
              HandleAp1EventMessage(msg_type_, msg_buf_, msg_len_);
              break;
             default:
              debug("INFO: Received message type: 0x%x", msg_type_);
              break;
          }
          msg_idx_++;
        } else if (msg_idx_ < 3 + msg_len_) {
          debug("MSG_HANDLER: copying 0x%x to %d", c, msg_idx_ - 3);
          msg_buf_[msg_idx_ - 3] = c;
          msg_idx_++;
        } else if (c == MESG_TX_SYNC) {
          debug("MSG_HANDLER: sync 0x%x", c);
          free(msg_buf_);
          msg_idx_ = 1;
        }
        break;
    }
  }
}

void Nrf24ap1::HandleAp1DataMessage(uint8_t type, uint8_t *buf, int len) {
  uint8_t channel = buf[0];
  uint8_t ap1_packet_id = buf[1];
  debug("INFO: data message 0x%x, id: 0x%x", type, ap1_packet_id);
  if (ap1_packet_id == AP1_PACKET_SYNC_ID) {
    free_ap1_packet(ap1_packet_buf_);
    ap1_packet_buf_ = create_ap1_packet((buf[7] << 8) | buf[8]);
    ap1_packet_buf_->source = (buf[3] << 8) | buf[4];
    ap1_packet_buf_->destination = (buf[5] << 8) | buf[6];
    ap1_idx_ = 0;
  } else if (ap1_packet_id == AP1_PACKET_DATA_ID) {
    if (ap1_packet_buf_ && ap1_idx_ < ap1_packet_buf_->length) {
      if (ap1_idx_ + len - 2 >= ap1_packet_buf_->length) {
        memcpy(ap1_packet_buf_->data + ap1_idx_, buf + 2, ap1_packet_buf_->length - ap1_idx_);
        ap1_idx_ += ap1_packet_buf_->length - ap1_idx_;
        // TODO: check for src/dst match, dst 0 = broadcast
        (*rx_handler_)(ap1_packet_buf_);
      } else {
        // copy out 7 data bytes
        memcpy(ap1_packet_buf_->data + ap1_idx_, buf + 2, len - 2);
        ap1_idx_ += len - 2;
      }
    } else {
      // drop data until we can sync
    }
  }
}

void Nrf24ap1::HandleAp1EventMessage(uint8_t type, uint8_t *buf, int len) {
  struct ant_packet *p = NULL;
  uint8_t channel = buf[0];
  uint8_t response_type = buf[1];
  uint8_t response_code = buf[2];
  debug("INFO: response 0x%x for message 0x%x", response_code, response_type);
  switch (response_code) {
    case RESPONSE_NO_ERROR:
    case EVENT_TX:
      if (!control_queue_.empty()) {
        p = control_queue_.front();
        control_queue_.pop_front();
        switch (response_type) {
          case EVENT_RF:
            // for all RF events, like broadcast
            if (p->type == MESG_BROADCAST_DATA_ID) {
              SendNextAntMessage();
            }
            break;
          default:
            if (p->type == response_type) {
              SendNextAntMessage();
            } else {
              debug("ERROR: Response type: 0x%x, expecting: 0x%x",
                    response_type, p->type);
            }
            break;
        }
        free_ant_packet(p);
      }
      break;
    default:
      debug("ERROR: Received response 0x%x for 0x%x",
            response_code, response_type);
      break;
  }
}

void Nrf24ap1::QueueMessage(struct ant_packet *packet) {
  control_queue_.push_back(packet);
  if (control_queue_.size() == 1) {
    SendNextAntMessage();
  }
}

void Nrf24ap1::SendNextAntMessage() {
  struct ant_packet *p = NULL;
  if (!control_queue_.empty()) {
    p = control_queue_.front();
    debug("SEND: next packet type: 0x%x", p->type);
    if (p->type == MESG_SYSTEM_RESET_ID) {
      control_queue_.pop_front();
      send_packet(ap1_, p);
      free_ant_packet(p);
    } else {
      send_packet(ap1_, p);
    }
  }
}

Nrf24ap1::~Nrf24ap1() {
  delete ap1_;
  delete cts_pin_;
}

}
