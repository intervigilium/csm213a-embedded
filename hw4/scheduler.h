#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "matrix.h"
#include "mbed.h"
#include "sdfg.h"


struct Schedule {
  int num_nodes;
  int period;
  int *q; // array, index represents node id, values represent repetitions
  int *schedule; // array, index represents time, values represent node id
};

struct Schedule * init_schedule(int num_nodes, struct Matrix *m);

void print_schedule(struct Sdfg *sdfg, struct Schedule *s);

void schedule_sdfg(struct Sdfg *sdfg, struct Schedule *s);

void free_schedule(struct Schedule *s);

#endif