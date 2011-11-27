#ifndef NRF24AP1_H_
#define NRF24AP1_H_

#include <list>

#include "libant.h"
#include "mbed.h"


#define NRF24AP1_BAUD 4800 // default Sparkfun ANT Baud

namespace Nrf24ap1 {

class Nrf24ap1 {
 public:
  Nrf24ap1(PinName tx, PinName rx, PinName cts);
  void Reset();
  int OpenChannel(int chan_id);
  void CloseChannel(int chan_id);
  int Send(uint8_t *buf, int len);
  void SetReceiveHandler(void (*handler)(int, uint8_t *, int));
  ~Nrf24ap1();

 protected:
  void HandleMessage();

 private:
  Serial *ap1_;
  InterruptIn *cts_pin_;
  std::list<int> channels_;
  void (*rx_handler_)(int, uint8_t *, int); // channel, data, len
};

}

#endif
