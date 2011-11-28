/* This is master's copy of main.cpp */
#include "ctype.h"
#include "string.h"
#include "hw_clock.h"
#include "mbed.h"
#define BUFLEN 100

Serial pc(USBTX, USBRX); /* PC connection via USB */
Serial cmd(p9, p10);     /* relay commands to slave */
Serial syn(p13, p14);    /* used to sync clock */
DigitalOut pinout(p20);  /* pin out to toggle */

DigitalOut led1(LED1);   /* visual of pinout p20 */
DigitalOut led2(LED2);   /* visual of pinin p30 */
DigitalOut led3(LED3);   /* visual of host input */

char buf[BUFLEN];        /* host input buffer */
int buf_len = -1;
struct timeval tv;

void pinToggle(void) {
  pinout = !pinout;
  led1 = !led1;
}

void reportToggle(struct timeval *tv) {
  led2 = !led2;
  pc.printf("%u.%u triggered by %s edge\n\r",
      tv->tv_sec, tv->tv_usec, led2? "rising" : "falling");
}

void cmdCallback(void) {
  char input = pc.getc();
  int len;

  /* relay to slave */
  cmd.putc(input);

  /* flip visual */
  led3 = !led3;

  /* homebrew state machine */
  if (isspace(input))
    return;

  if (input == 'S') {
    buf_len = 0;
  } else if (input == 'E' && buf_len > 0) {
    /* convert string to timeval structure */
    buf[buf_len] = '\0';
    len = strlen(buf);
    if (len > 6) {
      sscanf(buf + len - 6, "%u", (unsigned int *)&(tv.tv_usec));
      buf[len - 6] = '\0';
      sscanf(buf, "%u", (unsigned int *)&(tv.tv_sec));
    } else {
      tv.tv_sec = 0;
      sscanf(buf, "%u", (unsigned int *)&(tv.tv_usec));
    }
    pc.printf("sched pin toggle event at %u.%06u\n\r", tv.tv_sec, tv.tv_usec);
    /* add event to event list */
    runAtTime(&pinToggle, &tv);
  } else if (isdigit(input) && buf_len >= 0) {
    if (buf_len == BUFLEN - 1) {
      pc.printf("ERROR: can't schedule event after universe ends\n\r");
      buf_len = -1;
    } else {
      buf[buf_len++] = input;
    }
  } else {
      pc.printf("ERROR: bad input\n\r");
      buf_len = -1;
  }
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
  pc.attach(&cmdCallback, Serial::RxIrq);

  /* enter infinite loop */
  while (1) {
  }
}
