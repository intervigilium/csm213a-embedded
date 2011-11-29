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
uint8_t sync_bytes[16];
uint64_t t_s0, t_s1, t_s2, t_s3;
uint64_t t_m0, t_m1;

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
  char input = cmd.getc();
  int len;

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

  if (sync_byte_cnt == 8) {
    t_s1 = getLongTime();
  } else if (sync_byte_cnt == 16) {
    t_s3 = getLongTime();
    sync_byte_cnt = 0;

    t_m0 = t_m1 = 0;
    /* calculate offset and frequency */
    for (int i = 0; i < 8; ++i) {
      t_m0 <<= 8;
      t_m0 += (uint64_t)sync_bytes[7 - i];
      t_m1 <<= 8;
      t_m1 += (uint64_t)sync_bytes[15 - i];
    }

    /* print debug info */
    pc.printf("\r\nTo summarize:\r\n");
    pc.printf("t_s0: "); printLongTime(t_s0);
    pc.printf("t_m0: "); printLongTime(t_m0);
    pc.printf("t_s1: "); printLongTime(t_s1);

    pc.printf("\r\n");

    pc.printf("t_s2: "); printLongTime(t_s2);
    pc.printf("t_m1: "); printLongTime(t_m1);
    pc.printf("t_s3: "); printLongTime(t_s3);

    /* update hw_clock's parameter */
  }
}

void syncHelper(void) {
  t_s2 = getLongTime();
  for (int i = 0; i < 8; ++i)
    syn.putc(0);
}

void sync_clock(void) {
  struct timeval tv;

  tv.tv_sec = 1;
  tv.tv_usec = 0;
  runAtTime(&syncHelper, &tv);

  t_s0 = getLongTime();
  for (int i = 0; i < 8; ++i)
    syn.putc(0);
}

int main(void) {
  /* Print self checking info */
  pc.printf("Slave's SystemCoreClock = %u Hz\n\r", SystemCoreClock);
  pc.printf("version sync 1.1\r\n");

  /* Init global variables */
  pinout = 1;
  led1 = 1;
  led2 = 1;
  init_hw_timer();

  /* register reportToggle */
  runAtTrigger(&reportToggle);

  /* sync clock */
  syn.attach(&synCallback, Serial::RxIrq);
  sync_clock();

  /* accept command from master */
  cmd.attach(&cmdCallback, Serial::RxIrq);

  /* enter infinite loop */
  while (1) {
  }
}
