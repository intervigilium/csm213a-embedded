/* This is slave's copy of main.cpp */
#include "ctype.h"
#include "string.h"
#include "hw_clock.h"
#include "mbed.h"
#define BUFLEN 100

Serial pc(USBTX, USBRX); /* PC connection via USB */
Serial cmd(p9, p10);     /* receive commands from master */
Serial syn(p13, p14);    /* used to sync clock */
DigitalOut pinout(p20);  /* pin out to toggle */

DigitalOut led1(LED1);   /* visual of pinout p20 */
DigitalOut led2(LED2);   /* visual of pinin p30 */
DigitalOut led3(LED3);   /* visual of host input */

char buf[BUFLEN];        /* host input buffer */
int buf_len = -1;
struct timeval tv;

int sync_byte_cnt = 0;
int synced = 0;
double k_;
uint8_t sync_bytes[16];
uint64_t t_s0, t_s1, t_s2, t_s3;
uint64_t t_m0, t_m1;

struct timeval pps_rise, pps_fall;

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
  char input = cmd.getc();
  int len;

  /* flip visual */
  led3 = !led3;

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

void printLongTime(uint64_t ticks) {
  struct timeval tv;
  tv.tv_usec = (time_t) (ticks % 1000000);
  tv.tv_sec = (time_t) (ticks / 1000000);
  pc.printf(" =  %u.%06u\n\r", tv.tv_sec, tv.tv_usec);
}

void synCallback(void) {
  if (sync_byte_cnt > 15) {
    pc.printf("WATCH OUT, we've got a bad ass over here!!!\n\r");
    return;
  }

  sync_bytes[sync_byte_cnt++] = syn.getc();

  if (!synced) {
    if (sync_byte_cnt == 8) {
      t_s1 = getLongTime();
    } else if (sync_byte_cnt == 16) {
      t_s3 = getLongTime();
      sync_byte_cnt = 0;

      t_m0 = t_m1 = 0;
      for (int i = 0; i < 8; ++i) {
        t_m0 <<= 8;
        t_m0 += (uint64_t)sync_bytes[7 - i];
        t_m1 <<= 8;
        t_m1 += (uint64_t)sync_bytes[15 - i];
      }

      /* calculate offset and frequency */
      uint64_t temp;
      int64_t offset;

      t_s0 = (t_s0 + t_s1) / 2;
      t_s1 = (t_s2 + t_s3) / 2;

      k_ = (double)(t_s1 - t_s0) / (double)(t_m1 - t_m0);

      temp = (uint64_t)(k_ * t_m0);
      if (t_s0 > temp) {
        offset = t_s0 - temp;
      } else {
        offset = temp - t_s0;
        offset = -offset;
      }

      pc.printf("t_s = t_m * %f + %lld us\n\r", k_, offset);

      update_clock(k_, offset);
      synced = 1;

      getTime(&pps_rise);
      pps_rise.tv_sec += 5;     /* PPS signal starts after ten seconds */
      pps_rise.tv_usec = 0;
      pps_fall.tv_sec = pps_rise.tv_sec;     /* one pause per second */
      pps_fall.tv_usec = 500000;
      runAtTime(&ppsRise, &pps_rise);
      runAtTime(&ppsFall, &pps_fall);
    }
  } else {
    if (sync_byte_cnt == 8) {
      t_s1 = getLongTime();
      sync_byte_cnt = 0;


      t_m0 = 0;
      for (int i = 0; i < 8; ++i) {
        t_m0 <<= 8;
        t_m0 += (uint64_t)sync_bytes[7 - i];
      }
      t_s0 = (t_s0 + t_s1) / 2;

      int64_t offset;
      uint64_t temp = (uint64_t)(k_ * t_m0);
      if (t_s0 > temp) {
        offset = t_s0 - temp;
      } else {
        offset = temp - t_s0;
        offset = -offset;
      }
      update_clock(k_, offset);
    }
  }
}

void sync_helper0(void) {
  t_s0 = getLongTime();
  for (int i = 0; i < 8; ++i)
    syn.putc(0);
  getTime(&tv);
  tv.tv_sec += 5;
  runAtTime(&sync_helper0, &tv);
}

void sync_helper1(void) {
  t_s2 = getLongTime();
  for (int i = 0; i < 8; ++i)
    syn.putc(0);
}

void sync_clock(void) {
  struct timeval tv;

  tv.tv_sec = 2;
  tv.tv_usec = 0;
  runAtTime(&sync_helper0, &tv);

  tv.tv_sec = 5;
  tv.tv_usec = 0;
  runAtTime(&sync_helper1, &tv);
}

int main(void) {
  /* Print self checking info */
  pc.printf("Slave's SystemCoreClock = %u Hz\n\r", SystemCoreClock);
  pc.printf("version sync 1.5\r\n");

  /* Init global variables */
  pinout = 1;
  led1 = 1;
  led2 = 1;
  init_hw_timer();

  /* register reportToggle */
  runAtTrigger(&reportToggle);

  /* sync clock */
  syn.attach(&synCallback, Serial::RxIrq);
  
  /* initial sync */
  sync_clock();

  /* accept command from master */
  cmd.attach(&cmdCallback, Serial::RxIrq);

  /* enter infinite loop */
  while (1) {
  }
}
