/* This is master's copy of main.cpp */
#include "hw_clock.h"
#include "mbed.h"

Serial pc(USBTX, USBRX); /* PC connection via USB */
Serial cmd(p9, p10);     /* relay commands to slave */
Serial syn(p13, p14);    /* used to sync clock */
DigitalOut pinout(p20);  /* pin out to toggle */

DigitalOut led1(LED1);   /* visual of pinout p20 */
DigitalOut led2(LED2);   /* visual of pinin p30 */

void pinToggle(void) {
  pinout = !pinout;
  led1 = !led1;
}

void reportToggle(struct timeval *tv) {
  led2 = !led2;
  pc.printf("%u.%u triggered by %s edge\n\r",
      tv->tv_sec, tv->tv_usec, led2? "rising" : "falling");
}

int main(void) {
  /* Print self checking info */
  pc.printf("Master's SystemCoreClock = %u Hz\n\r", SystemCoreClock);

  /* Init global variables */
  pinout = 1;
  led1 = 1;
  led2 = 1;
  init_hw_timer();

  /* register reportToggle */
  runAtTrigger(&reportToggle);

  /* sync clock */
  /* accept command from host */

  /* enter infinite loop */
  while (1) {
  }
}
