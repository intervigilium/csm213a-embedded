#include "timed_task.h"

void insert_timed_task(struct TimedTask **list, struct TimedTask *task) {
  struct TimedTask *ptr = *list;
  if (*list == NULL) {
    *list = task;
  } else {
    while (ptr) {
      if (!is_time_earlier(ptr->time, task->time)) {
        // ptr occurs after t, insert t before ptr
        task->prev_task = ptr->prev_task;
        task->next_task = ptr;
        if (ptr->prev_task) {
          ptr->prev_task->next_task = task;
        }
        ptr->prev_task = task;
        break;
      }
      if (ptr->next_task == NULL) {
        // end of task list
        ptr->next_task = task;
        task->prev_task = ptr;
        break;
      }
      ptr = ptr->next_task;
    }
  }
}

struct TimedTask * pop_task_list(struct TimedTask **list) {
  struct TimedTask *task = *list;
  *list = task->next_task;
  if (*list) {
    (*list)->prev_task = NULL;
  }
  task->next_task = NULL;
  return task;
}

void free_timed_task(struct TimedTask *task) {
  free(task->time);
  free(task);
}
