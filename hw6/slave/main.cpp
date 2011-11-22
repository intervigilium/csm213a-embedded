#include "mbed.h"


DigitalOut pin_24(p24);
DigitalOut led_1(LED1);

int main() {

  printf("SystemCoreClock = %u Hz\n\r", SystemCoreClock);

  while (1) {
    printf("PULSING P24\n\r");
    pin_24 = !pin_24;
    wait_ms(1000);
  }
}
