#ifndef TIME_H_
#define TIME_H_

#include "mbed.h"


// GNU libc time.h defines these as long int
struct timeval {
  time_t tv_sec;
  time_t tv_usec;
};

bool is_time_earlier(struct timeval *tv0, struct timeval *tv1);

#endif
