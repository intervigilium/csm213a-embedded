#ifndef NRF24AP1_H_
#define NRF24AP1_H_

#include <list>

#include "libant.h"
#include "mbed.h"


#define DEFAULT_WAIT_MS 100 // default ms to wait after config
#define NRF24AP1_BAUD 4800 // default Sparkfun ANT Baud
#define AP1_PACKET_SYNC_ID 0xAF
#define AP1_PACKET_DATA_ID 0xDA

struct ap1_packet {
  uint16_t source;
  uint16_t destination;
  uint16_t length;
  uint8_t *data;
};

namespace Nrf24ap1 {

class Nrf24ap1 {
 public:
  Nrf24ap1(PinName tx, PinName rx, PinName cts);
  uint16_t GetDeviceId();
  void Reset();
  int OpenChannel(int chan_id, int chan_type);
  void CloseChannel(int chan_id);
  int Send(int chan_id, struct ap1_packet *p);
  void SetReceiveHandler(void (*handler)(struct ap1_packet *));
  ~Nrf24ap1();

 private:
  void HandleAp1DataMessage(uint8_t type, uint8_t *buf, int len);
  void HandleAp1EventMessage(uint8_t type, uint8_t *buf, int len);
  void OnAp1Rx();
  void QueueMessage(struct ant_packet *p);
  void SendNextAntMessage();

  uint16_t dev_id_;
  uint8_t msg_len_;
  uint8_t msg_type_;
  uint8_t *msg_buf_;
  int msg_idx_;
  struct ap1_packet *ap1_packet_buf_;
  int ap1_idx_;
  Serial *ap1_;
  InterruptIn *cts_pin_;
  std::list<int> channels_;
  std::list<struct ant_packet *> control_queue_;
  void (*rx_handler_)(struct ap1_packet *);
};

}

#endif
