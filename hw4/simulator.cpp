#include "scheduler.h"
#include "sdfg.h"
#include "simulator.h"
#include "mbed.h"


static int read_in_buf(struct Simulator *s, int idx) {
  if (s->in_buf_size < MAX_SAMPLES) {
    return s->in_buf[idx % MAX_SAMPLES];
  } else {
    return s->in_buf[(s->in_idx + idx) % MAX_SAMPLES];
  }
}

static int read_out_buf(struct Simulator *s, int idx) {
  if (s->out_buf_size < MAX_SAMPLES) {
    return s->out_buf[idx % MAX_SAMPLES];
  } else {
    // s->out_idx points to next index to be written/oldest index written
    return s->out_buf[(s->out_idx + idx) % MAX_SAMPLES];
  }
}

static void write_in_buf(struct Simulator *s, int val) {
  s->in_buf[s->in_idx] = val;
  s->in_idx = (s->in_idx + 1) % MAX_SAMPLES;
  if (s->in_buf_size < MAX_SAMPLES) {
    s->in_buf_size++;
  }
}

static void write_out_buf(struct Simulator *s, int val) {
  s->checksum ^= val;
  s->out_buf[s->out_idx] = val;
  s->out_idx = (s->out_idx + 1) % MAX_SAMPLES;
  if (s->out_buf_size < MAX_SAMPLES) {
    s->out_buf_size++;
  }
}

static int edge_pop(struct Simulator *sim, struct Edge *e, int *val) {
  if (e->input->type == 'I') {
    //printf("Try to read, %d left\r\n", sim->num_inputs);
    if (sim->read_idx == sim->iterations * sim->num_samples) {
      printf("NO MORE INPUTS LEFT, READ IDX: %d\n\r", sim->read_idx);
      return -1;
    }
    *val = read_in_buf(sim, sim->read_idx);
    sim->read_idx++;
  } else {
    //printf("head %d tail %d\r\n", e->fifo_h, e->fifo_t);
    if (e->fifo_h == e->fifo_t) {
      return -1;
    }
    *val = e->fifo[e->fifo_h];
    e->fifo_h = (e->fifo_h + 1) % EDGE_FIFO_SIZE;
  }
  //printf("Popping value %d from edge %d\r\n", *val, e->id);
  return 0;
}

static void edge_push(struct Simulator *sim, struct Edge *e, int val) {
  //printf("Pushing value %d to edge %d\r\n", val, e->id);
  if (e->output->type == 'O') {
    write_out_buf(sim, val);
  } else {
    if ((e->fifo_t + 1) % EDGE_FIFO_SIZE == e->fifo_h) {
      error("edge FIFO is full!");
    }
    e->fifo[e->fifo_t] = val;
    e->fifo_t = (e->fifo_t + 1) % EDGE_FIFO_SIZE;
  }
}

struct Simulator * init_simulator_from_file(int *in_buf, int *out_buf, FILE *fp) {
  int read;
  int val;
  struct Simulator *s = NULL;
  
  s = (struct Simulator *) malloc(sizeof(struct Simulator));
  s->checksum = 0;
  s->read_idx = 0;
  s->in_idx = 0;
  s->in_buf_size = 0;
  s->out_idx = 0;
  s->out_buf_size = 0;
  s->in_buf = in_buf;
  s->out_buf = out_buf;

  read = fscanf(fp, "%d\n%d\n", &s->iterations, &s->num_samples);
  if (read != 2) {
    printf("ERROR: BAD INPUT.TXT DESCRIPTION\n\r");
    goto on_error;
  }

  for (int i = 0; i < s->num_samples; i++) {
    if (feof(fp)) {
      printf("ERROR: READ %d SAMPLES OUT OF %d\n\r", i, s->num_samples);
      goto on_error;
    }
    read = fscanf(fp, "%d\n", &val);
    if (read != 1) {
      printf("ERROR: BAD INPUT.TXT FORMAT\n\r");
      goto on_error;
    } else {
      write_in_buf(s, val);
    }
  }
  
  return s;
on_error:
  free(s);
  return NULL;
}

void simulate(struct Simulator *sim, struct Sdfg *sdfg, struct Schedule *s) {
  struct Node *n;
  int val, temp;
  
  // loop forever
  for (int k = 0; ; k++) {
    printf("ITERATION %d \r\n", k);
    for (int i = 0; i < s->period; i++) {
      n = &sdfg->node_list[s->schedule[i]];
      printf("SIMULATING NODE: %c_%d\r\n", n->type, n->id);
      switch (n->type) {
        case 'A':
          if (edge_pop(sim, n->input_a, &val) != 0)
            goto done;
          if (edge_pop(sim, n->input_b, &temp) != 0)
            goto done;
          val += temp;
          for (int j = 0; j < n->num_outputs; j++) {
            edge_push(sim, n->outputs[j], val);
          }
          break;
        case 'S':
          if (edge_pop(sim, n->input_a, &val) != 0)
            goto done;
          if (edge_pop(sim, n->input_b, &temp) != 0)
            goto done;
          val -= temp;
          for (int j = 0; j < n->num_outputs; j++) {
            edge_push(sim, n->outputs[j], val);
          }
          break;
        case 'M':
          if (edge_pop(sim, n->input_a, &val) != 0)
            goto done;
          val = val * n->constant_a / n->constant_b;
          for (int j = 0; j < n->num_outputs; j++) {
            edge_push(sim, n->outputs[j], val);
          }
          break;
        case 'D':
          for (int j = 0; j < n->constant_a; j++) {
            if (edge_pop(sim, n->input_a, &val) != 0)
              goto done;
          }
          for (int j = 0; j < n->num_outputs; j++) {
            edge_push(sim, n->outputs[j], val);
          }
          break;
        case 'U':
          if (edge_pop(sim, n->input_a, &val) != 0)
            goto done;
          for (int l = 0; l < n->constant_a; l++) {
            for (int j = 0; j < n->num_outputs; j++) {
              edge_push(sim, n->outputs[j], val);
            }
          }
          break;
        case 'C':
          for (int j = 0; j < n->num_outputs; j++) {
            edge_push(sim, n->outputs[j], n->constant_a);
          }
          break;
        case 'F':
          if (edge_pop(sim, n->input_a, &val) != 0)
            goto done;
          for (int j = 0; j < n->num_outputs; j++) {
            edge_push(sim, n->outputs[j], val);
          }
          break;
        default:
          error("INVALID SCHEDULE");
          goto done;
      }
    }
  }

done:
  printf("we are done");
  // TODO do output
}

void write_output(struct Simulator *sim, FILE *fp) {
  fprintf(fp, "%d\n0x%x\n", sim->out_buf_size, sim->checksum);
  for (int i = 0; i < sim->out_buf_size; i++) {
    fprintf(fp, "%d\n", read_out_buf(sim, i));
  }
}

void print_simulator(struct Simulator *sim) {
  printf("SIMULATOR\n\r");
  printf("ITERATIONS: %d\n\r", sim->iterations);
  printf("NUM SAMPLES: %d\n\r", sim->num_samples);
  printf("SAMPLES:\n\r");
  for (int i = 0; i < sim->num_samples; i++) {
    printf("%d\n\r", sim->in_buf[i]);
  }
}

void free_simulator(struct Simulator *sim) {
  free(sim);
}