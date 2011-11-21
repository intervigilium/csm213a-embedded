#ifndef HW_CLOCK_H_
#define HW_CLOCK_H_

#include "mbed.h"
#include "timed_task.h"


// GNU libc time.h defines these as long int
struct timeval {
  time_t tv_sec;
  time_t tv_usec;
};

bool is_time_earlier(struct timeval *tv0, struct timeval *tv1); 

void getTime(struct timeval *tv);

void runAtTime(void (*schedFunc)(void), struct timeval *tv);

void runAtTrigger( void (*trigFunc)(struct timeval *tv));

void free_timed_task(struct TimedTask *tt);

#endif