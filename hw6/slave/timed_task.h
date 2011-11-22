#ifndef TIMED_TASK_
#define TIMED_TASK_

#include "time.h"


struct TimedTask {
  struct timeval *time;
  void (*task)(void);
  struct TimedTask *prev_task;
  struct TimedTask *next_task;
};

void insert_timed_task(struct TimedTask **list, struct TimedTask *task);

struct TimedTask * pop_task_list(struct TimedTask **list);

void free_timed_task(struct TimedTask *task);

#endif
