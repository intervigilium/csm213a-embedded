#ifndef HW_CLOCK_H_
#define HW_CLOCK_H_

#include "mbed.h"
#include "time.h"
#include "timed_task.h"


bool is_time_earlier(struct timeval *tv0, struct timeval *tv1);

void init_hw_timer(void);

void getTime(struct timeval *tv);

void runAtTime(void (*schedFunc)(void), struct timeval *tv);

void runAtTrigger( void (*trigFunc)(struct timeval *tv));

#endif
