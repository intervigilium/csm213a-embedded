#ifndef MATRIX_H_
#define MATRIX_H_

#include "sdfg.h"


struct Matrix {
  int dim_x;
  int dim_y;
  int **matrix;
};

struct Matrix * generate_matrix_from_sdfg(struct Sdfg *sdfg);

void matrix_to_rref(struct Matrix *m);

void print_matrix(struct Matrix *m);

int get_rref_rank(struct Matrix *rref_m);

void free_matrix(struct Matrix *m);

#endif