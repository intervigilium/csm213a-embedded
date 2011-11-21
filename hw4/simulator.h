#ifndef SIMULATOR_H_
#define SIMULATOR_H_

#define MAX_SAMPLES 4096

struct Simulator {
  int iterations;
  int num_samples;
  unsigned checksum;
  int read_idx;
  int in_idx;
  int in_buf_size;
  int out_idx;
  int out_buf_size;
  int *in_buf;
  int *out_buf;
};

struct Simulator * init_simulator_from_file(int *in_buf, int *out_buf, FILE *fp);

void simulate(struct Simulator *sim, struct Sdfg *sdfg, struct Schedule *s);

void write_output(struct Simulator *sim, FILE *fp);

void print_simulator(struct Simulator *sim);

void free_simulator(struct Simulator *sim);

#endif