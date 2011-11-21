#ifndef SDFG_H_
#define SDFG_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define READ_BUF_SIZE 64
#define BRANCH_FACTOR 4
#define EDGE_FIFO_SIZE 32

struct Node;
struct Edge;
struct Sdfg;

struct Node {
  char type;
  int id; // node lives in Sdfg->node_list[id]
  int constant_a;
  int constant_b;
  int num_outputs;
  struct Edge *input_a;
  struct Edge *input_b;
  struct Edge **outputs;
};

struct Edge {
  int id; // edge lives in Sdfg->edge_list[id - 1]
  int delay;
  int fifo_h;
  int fifo_t;
  int *fifo;
  struct Node *input;
  struct Node *output;
};

struct Sdfg {
  int num_nodes;
  int num_edges;
  struct Node *input_node;
  struct Node *output_node;
  struct Node *node_list;
  struct Edge *edge_list;
};

struct Sdfg * create_sdfg_from_file(FILE *fp);

void free_sdfg(struct Sdfg *sdfg);

#endif