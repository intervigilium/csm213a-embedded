#ifndef NRF24AP1_H_
#define NRF24AP1_H_

#include <list>

#include "libant.h"
#include "mbed.h"


#define DEFAULT_WAIT_MS 100 // default ms to wait after config
#define NRF24AP1_BAUD 4800 // default Sparkfun ANT Baud

namespace Nrf24ap1 {

struct ap1_packet {
  uint16_t source;
  uint16_t destination;
  int length;
  uint8_t *data;
}

class Nrf24ap1 {
 public:
  Nrf24ap1(PinName tx, PinName rx, PinName cts);
  uint16_t GetDeviceId();
  void Reset();
  int OpenChannel(int chan_id, int chan_type);
  void CloseChannel(int chan_id);
  int Send(int chan_id, struct ap1_packet *packet);
  void SetReceiveHandler(void (*handler)(uint8_t, uint8_t *, int));
  ~Nrf24ap1();

 private:
  void HandleAp1Message(uint8_t type, uint8_t *buf, int len);
  void OnAp1Rx();
  void QueueMessage(struct ant_packet *packet);

  uint16_t dev_id_;
  uint8_t msg_len_;
  uint8_t msg_type_;
  uint8_t *msg_buf_;
  int msg_idx_;
  Serial *ap1_;
  InterruptIn *cts_pin_;
  std::list<int> channels_;
  std::list<struct ant_packet *> control_queue_;
  void (*rx_handler_)(uint8_t, uint8_t *, int); // type, data, len
};

}

#endif
