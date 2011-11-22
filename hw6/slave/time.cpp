#include "time.h"


bool is_time_earlier(struct timeval *tv0, struct timeval *tv1) {
  // returns true if tv0 is earlier, false if tv1 is earlier
  if (tv0->tv_sec == tv1->tv_sec) {
    return tv0->tv_usec < tv1->tv_usec;
  } else {
    return tv0->tv_sec < tv1->tv_sec;
  }
}
