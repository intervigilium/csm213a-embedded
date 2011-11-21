#include "hw_clock.h"
#include "mbed.h"

DigitalOut led_1(LED1);

void blink_led1() {
  led_1 = !led_1;
}

int main() {
  struct timeval blink_time1;
  struct timeval blink_time2;
  struct timeval *tv = (struct timeval *) malloc(sizeof(struct timeval));
  
  printf("SystemCoreClock = %d Hz\n\r", SystemCoreClock);
  blink_time1.tv_sec = 2;
  blink_time1.tv_usec = 1234;
  runAtTime(&blink_led1, &blink_time1);
  blink_time2.tv_sec = 4;
  blink_time2.tv_usec = 5678;
  runAtTime(&blink_led1, &blink_time2);

  while (1) {
    getTime(tv);
    printf("Current elapsed: %u s, %u us\n\r", tv->tv_sec, tv->tv_usec);
    wait_ms(1000);
  }
}
