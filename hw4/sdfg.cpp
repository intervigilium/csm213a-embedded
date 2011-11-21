#include "sdfg.h"
#include "simulator.h"
#include "mbed.h"

static void add_input_edge(struct Node *n, struct Edge *e, int input_b) {
  if (input_b) {
    n->input_b = e;
  } else {
    n->input_a = e;
  }
  e->output = n;
  printf("ADDING INPUT FROM EDGE %d\n\r", e->id);
}

static void add_output_edge(struct Node *n, struct Edge *e) {
  if (n->outputs == NULL) {
    n->outputs = (struct Edge **) malloc(sizeof(struct Edge *) * BRANCH_FACTOR);
    memset(n->outputs, 0, sizeof(struct Edge *) * BRANCH_FACTOR);
  }
  n->outputs[n->num_outputs] = e;
  e->input = n;
  n->num_outputs++;
  printf("ADDING OUTPUT TO EDGE %d\n\r", e->id);
}

int create_node_from_line(struct Node *n, int id, struct Edge *edge_list, char *line) {
  int token_idx = 0;
  int val;
  n->id = id; // id is node index in sdfg->node_list
  n->num_outputs = 0;
  n->outputs = NULL; // warning: no check to see if outputs > BRANCH_FACTOR
  
  char *token = strtok(line, " ");
  n->type = token[0];  // this is kind of fragile
  printf("CURRENT NODE IS %c_%d\n\r", n->type, n->id);
  while (1) {
    token_idx++;
    token = strtok(NULL, " ");
    if (token == NULL) {
      break;
    }
    val = atoi(token);
    printf("CURRENT TOKEN IS %d\n\r", val);
    switch (n->type) {
      case 'I':
        add_output_edge(n, &edge_list[val - 1]);
        break;
      case 'O':
        if (token_idx > 1) {
          // error, badly formatted output node
          printf("ERROR: badly formatted output node %d\n\r", n->id);
          return -1;
        } else {
          add_input_edge(n, &edge_list[val - 1], 0);
          n->num_outputs++; // not sure why this is here
        }
        break;
      case 'A':
      case 'S':
        switch (token_idx) {
          case 1:
            add_input_edge(n, &edge_list[val - 1], 0);
            break;
          case 2:
            add_input_edge(n, &edge_list[val - 1], 1);
            break;
          default:
            add_output_edge(n, &edge_list[val - 1]);
            break;
        }
        break;
      case 'M':
        switch (token_idx) {
          case 1:
            n->constant_a = val;
            break;
          case 2:
            n->constant_b = val;
            break;
          case 3:
            add_input_edge(n, &edge_list[val - 1], 0);
            break;
          default:
            add_output_edge(n, &edge_list[val - 1]);
            break;
        }
        break;
      case 'D':
      case 'U':
        switch (token_idx) {
          case 1:
            n->constant_a = val;
            break;
          case 2:
            add_input_edge(n, &edge_list[val - 1], 0);
            break;
          default:
            add_output_edge(n, &edge_list[val - 1]);
            break;
        }
        break;
      case 'C':
        if (token_idx == 1) {
          n->constant_a = val;
        } else {
          add_output_edge(n, &edge_list[val - 1]);
        } 
        break;
      case 'F':
        if (token_idx == 1) {
          add_input_edge(n, &edge_list[val - 1], 0);
        } else {
          add_output_edge(n, &edge_list[val - 1]);
        }
        break;
    }
  }
  return 0;
}

int create_edge_from_line(struct Edge *edge_list, char *line) {
  struct Edge *e = NULL;
  int id = 0;
  int val;
  int token_idx = 0;
  char *token = strtok(line, " "); // eat first type token
  while (1) {
    token_idx++;
    token = strtok(NULL, " ");
    if (token == NULL) {
      break;
    }
    val = atoi(token);
    switch (token_idx) {
      case 1:
        id = val;
        e = &edge_list[id - 1];
        e->id = id;
        break;
      case 2:
        e->delay = val;
        break;
      default:
        if (token_idx - 3 > e->delay) {
          // bad formatting, more samples than delay
          printf("ERROR: badly formatted edge %d\n\r", e->id);
          return -1;
        }
        e->fifo[e->fifo_t] = val;
        e->fifo_t++;
    }
  }
  return 0;
}

void init_edge_list(struct Edge *edge_list, int len) {
  for (int i = 0; i < len; i++) {
    edge_list[i].id = i+1;
    edge_list[i].delay = 0;
    edge_list[i].fifo_h = 0;
    edge_list[i].fifo_t = 0;
    edge_list[i].fifo = (int *)malloc(sizeof(int) * EDGE_FIFO_SIZE);
    edge_list[i].input = NULL;
    edge_list[i].output = NULL;
  }
}

struct Sdfg * create_sdfg_from_file(FILE *fp) {
  int res = 0;
  int node_count = 0;
  char *buf = NULL;
  
  struct Sdfg *sdfg = (struct Sdfg *) malloc(sizeof(struct Sdfg));
  if (sdfg == NULL) {
    // malloc error
    printf("malloc error!\n\r");
    return NULL;
  }
  
  res = fscanf(fp, "%d %d\n", &sdfg->num_nodes, &sdfg->num_edges);
  if (res != 2) {
    printf("ERROR: expected input nodes, edges!\n\r");
    goto on_error;
  }
  sdfg->num_nodes -= 2; // remove output/input node from list
  sdfg->input_node = (struct Node *) malloc(sizeof(struct Node));
  sdfg->output_node = (struct Node *) malloc(sizeof(struct Node));
  sdfg->node_list = (struct Node *) malloc(sizeof(struct Node) * sdfg->num_nodes);
  sdfg->edge_list = (struct Edge *) malloc(sizeof(struct Edge) * sdfg->num_edges);
  memset(sdfg->node_list, 0, sizeof(struct Node) * sdfg->num_nodes);
  
  init_edge_list(sdfg->edge_list, sdfg->num_edges);
  
  while (!feof(fp)) {
    buf = (char *) malloc(sizeof(char) * READ_BUF_SIZE);
    if (fgets(buf, READ_BUF_SIZE, fp) == NULL) {
      if (feof(fp)) {
        // weird check to make sure trailing newlines are caught I guess
        break;
      }
      perror("ERROR: problem reading line");
      goto on_error;
    }

    if (buf[0] == 'E') {
      printf("ADDING EDGE\n\r");
      res = create_edge_from_line(sdfg->edge_list, buf);
    } else if (buf[0] == 'I') {
      printf("ADDING INPUT NODE\n\r");
      res = create_node_from_line(sdfg->input_node, -1, sdfg->edge_list, buf);
    } else if (buf[0] == 'O') {
      printf("ADDING OUTPUT NODE\n\r");
      res = create_node_from_line(sdfg->output_node, -1, sdfg->edge_list, buf);
    } else {
      printf("ADDING NODE: %d\n\r", node_count);
      res = create_node_from_line(&sdfg->node_list[node_count], node_count, sdfg->edge_list, buf);
      node_count++;
    }
    if (res) {
      printf("ERROR: problem encountered adding node or edge\n\r");
      goto on_error;
    }
    free(buf);
  }
  
  return sdfg;
on_error:
  free(buf);
  free_sdfg(sdfg);
  return NULL;
}

void free_node(struct Node *n) {
  free(n->outputs);
}

void free_edge(struct Edge *e) {
  free(e->fifo);
}

void free_sdfg(struct Sdfg *sdfg) {
  for (int i = 0; i < sdfg->num_nodes; i++) {
    free_node(&sdfg->node_list[i]);
  }
  for (int j = 0; j < sdfg->num_edges; j++) {
    free_edge(&sdfg->edge_list[j]);
  }
  free(sdfg->node_list);
  free(sdfg->edge_list);
  free(sdfg);
}