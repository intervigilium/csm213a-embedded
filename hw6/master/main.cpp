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

struct timeval pps_rise, pps_fall;

int sync_byte_cnt = 0;

void pinToggle(void) {
  pinout = !pinout;
  led1 = !led1;
}

void ppsRise(void) {
  pinout = !pinout;
  led1 = !led1;
  pps_rise.tv_sec += 1;
  runAtTime(&ppsRise, &pps_rise);
}

void ppsFall(void) {
  pinout = !pinout;
  led1 = !led1;
  pps_fall.tv_sec += 1;
  runAtTime(&ppsFall, &pps_fall);
}

void reportToggle(struct timeval *tv) {
  led2 = !led2;
  pc.printf("%u.%06u triggered by %s edge\n\r",
      tv->tv_sec, tv->tv_usec, led2? "rising" : "falling");
}

void cmdCallback(void) {
  char input = pc.getc();
  int len;

  /* flip visual */
  led3 = !led3;

  /* relay to slave */
  cmd.putc(input);

  /* homebrew state machine */
  if (isspace(input))
    return;

  /* debugging use: press t and print current time */
  if (input == 't') {
    getTime(&tv);
    pc.printf("Now: %u.%06u\n\r", tv.tv_sec, tv.tv_usec);
    return;
  }

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
    //runAtTime(&pinToggle, &tv);
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

void synCallback(void) {
  syn.getc();

  /* receive 64 bits of garbage from slave */
  if (++sync_byte_cnt != 8)
    return;

  sync_byte_cnt = 0;

  /* get current time */
  uint64_t ticks = getLongTime();

  /* send 64 bits of time value to slave */
  for (int i = 0; i < 8; ++i) {
    syn.putc((uint8_t)ticks);
    ticks >>= 8;
  }
}

int main(void) {
  /* Print self checking info */
  pc.printf("Master's SystemCoreClock = %u Hz\n\r", SystemCoreClock);
  pc.printf("version sync 1.5\r\n");

  /* Init global variables */
  pinout = 1;
  led1 = 1;
  led2 = 1;
  init_hw_timer();

  getTime(&pps_rise);
  pps_rise.tv_sec += 5;     /* PPS signal starts after ten seconds */
  pps_rise.tv_usec = 0;
  pps_fall.tv_sec = pps_rise.tv_sec;     /* one pause per second */
  pps_fall.tv_usec = 500000;
  runAtTime(&ppsRise, &pps_rise);
  runAtTime(&ppsFall, &pps_fall);

  /* register reportToggle */
  runAtTrigger(&reportToggle);

  /* sync clock */
  syn.attach(&synCallback, Serial::RxIrq);

  /* accept command from host */
  pc.attach(&cmdCallback, Serial::RxIrq);

  /* enter infinite loop */
  while (1) {
  }
}
