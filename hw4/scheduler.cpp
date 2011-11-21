#include "scheduler.h"


struct Schedule * init_schedule(int num_nodes, struct Matrix *m) {
  struct Schedule *s;
  int val;
  int rank = get_rref_rank(m);
  
  // schedule exists IFF rank of solution to topology matrix of sdfg = nodes - 1
  if (rank != num_nodes - 1) {
    printf("ERROR: UNSCHEDULABLE SDFG, RANK: %d, NODES: %d\n\r", rank, num_nodes);
    return NULL;
  }
  
  s = (struct Schedule *) malloc(sizeof(struct Schedule));
  // TODO: check for mem error
  s->num_nodes = num_nodes;
  // period = sum(elements of kernel of topology matrix of sdfg)
  s->period = 1;
  s->q = (int *) malloc(sizeof(int) * num_nodes);
  // TODO: check for mem error
  for (int i = 0; i < num_nodes - 1; i++) {
    val = -1 * m->matrix[i][num_nodes - 1];
    s->q[i] = val;
    s->period += val;
  }
  s->q[num_nodes - 1] = 1; // last node always gets 1 for being scheduled once
  s->schedule = (int *) malloc(sizeof(int) * s->period);
  return s;
}

void print_schedule(struct Sdfg *sdfg, struct Schedule *s) {
  struct Node *n = NULL;
  printf("SCHEDULE\n\r");
  printf("NODES: %d\n\r", s->num_nodes);
  printf("PERIOD: %d\n\r", s->period);
  printf("Q: ");
  for (int i = 0; i < s->num_nodes; i++) {
    printf("%d ", s->q[i]);
  }
  printf("\n\r");
  printf("SCHEDULE: ");
  for (int i = 0; i < s->period; i++) {
    n = &sdfg->node_list[s->schedule[i]];
    printf("%c_%d ", n->type, n->id);
  }
  printf("\n\r");
}

void schedule_sdfg(struct Sdfg *sdfg, struct Schedule *s) {
  // use list scheduler, start at any node following an edge with delay
  // mark usages in period against s->q
  int s_idx = 0;
  struct Node *n = NULL;
  struct Edge *a = NULL;
  struct Edge *b = NULL;
  int *fifo_tally = (int *) malloc(sizeof(int) * sdfg->num_edges);
  int *tally = (int *) malloc(sizeof(int) * sdfg->num_nodes);
  memset(tally, 0, sizeof(int) * sdfg->num_nodes);
  
  // init delay tally with delays in system
  for (int i = 0; i < sdfg->num_edges; i++) {
    if (sdfg->edge_list[i].input == sdfg->input_node) {
      // for all intents and purposes this should be unlimited
      fifo_tally[i] = 255;
    } else {
      fifo_tally[i] = sdfg->edge_list[i].delay;
    }
  }
  
  printf("ATTEMPTING TO SCHEDULE SDFG\n\r");
  // list scheduler is guaranteed to find schedule if schedule exists
  for (int i = 0; i < s->period; i++) {
    for (int j = 0; j < sdfg->num_nodes; j++) {
      n = &sdfg->node_list[j];
      printf("CHECKING NODE %c_%d at TIMESLOT %d\n\r", n->type, n->id, i);
      if (tally[n->id] < s->q[n->id]) {
        a = n->input_a;
        b = n->input_b;
        if (n->type == 'C') {
          printf("GOT NODE %c_%d FOR TIMESLOT %d\n\r", n->type, n->id, i);
          // constant node is always schedulable
          tally[n->id]++;
          s->schedule[s_idx] = n->id;
          s_idx++;
          // "push" outputs to fifo tally
          for (int k = 0; k < n->num_outputs; k++) {
            fifo_tally[n->outputs[k]->id - 1]++;
          }
          n = NULL;
          break;
        } else if ((n->type == 'A' || n->type == 'S')
                   && fifo_tally[a->id - 1] >= 1 && fifo_tally[b->id - 1] >= 1) {
          printf("GOT NODE %c_%d FOR TIMESLOT %d\n\r", n->type, n->id, i);
          tally[n->id]++;
          s->schedule[s_idx] = n->id;
          s_idx++;
          fifo_tally[a->id - 1]--;
          fifo_tally[b->id - 1]--;
          // "push" outputs to fifo tally
          for (int k = 0; k < n->num_outputs; k++) {
            fifo_tally[n->outputs[k]->id - 1]++;
          }
          n = NULL;
          break;
        } else if (n->type == 'D' && fifo_tally[a->id - 1] >= n->constant_a) {
          printf("GOT NODE %c_%d FOR TIMESLOT %d\n\r", n->type, n->id, i);
          tally[n->id]++;
          s->schedule[s_idx] = n->id;
          s_idx++;
          fifo_tally[a->id - 1] -= n->constant_a;
          // "push" outputs to fifo tally
          for (int k = 0; k < n->num_outputs; k++) {
            fifo_tally[n->outputs[k]->id - 1]++;
          }
          n = NULL;
          break;
        } else if (n->type == 'U' && fifo_tally[a->id - 1] >= 1) {
          printf("GOT NODE %c_%d FOR TIMESLOT %d\n\r", n->type, n->id, i);
          tally[n->id]++;
          s->schedule[s_idx] = n->id;
          s_idx++;
          fifo_tally[n->input_a->id - 1]--;
          // "push" outputs to fifo tally
          for (int k = 0; k < n->num_outputs; k++) {
            fifo_tally[n->outputs[k]->id - 1] += n->constant_a;
          }
          n = NULL;
          break;
        } else if ((n->type == 'M' || n->type == 'F') && fifo_tally[a->id - 1] >= 1) {
          printf("GOT NODE %c_%d FOR TIMESLOT %d\n\r", n->type, n->id, i);
          tally[n->id]++;
          s->schedule[s_idx] = n->id;
          s_idx++;
          fifo_tally[n->input_a->id - 1]--;
          // "push" outputs to fifo tally
          for (int k = 0; k < n->num_outputs; k++) {
            fifo_tally[n->outputs[k]->id - 1]++;
          }
          n = NULL;
          break;
        }
      }
    }
    if (n) {
      error("CANNOT SCHEDULE\n\r");
      goto on_error;
    }
  }
  
on_error:
  free(fifo_tally);
  free(tally);  
}

void free_schedule(struct Schedule *s) {
  free(s->q);
  free(s->schedule);
}