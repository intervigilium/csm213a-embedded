#include <string.h>
#include <list>

#include "matrix.h"


/***** BEGIN ROSETTA CODE SECTION *****/
void MtxMulAndAddRows(struct Matrix *m, int ixrdest, int ixrsrc, int mplr)
{
  int *drow, *srow;
  drow = m->matrix[ixrdest];
  srow = m->matrix[ixrsrc];
  for (int ix = 0; ix < m->dim_x; ix++) {
    drow[ix] += mplr * srow[ix];
  }
//    printf("Mul row %d by %d and add to row %d\n", ixrsrc, mplr, ixrdest);
//    MtxDisplay(m);
}

void MtxSwapRows(struct Matrix *m, int rix1, int rix2)
{
  int *r1, *r2, temp;
  if (rix1 == rix2) {
    return;
  }
  r1 = m->matrix[rix1];
  r2 = m->matrix[rix2];
  for (int ix = 0; ix < m->dim_x; ix++) {
    temp = r1[ix];
    r1[ix] = r2[ix]; 
    r2[ix] = temp;
  }
//    printf("Swap rows %d and %d\n", rix1, rix2);
//    MtxDisplay(m);
}

void MtxNormalizeRow(struct Matrix *m, int rix, int lead)
{
  int *drow;
  int lv;
  drow = m->matrix[rix];
  lv = drow[lead];
  for (int ix = 0; ix < m->dim_x; ix++) {
    drow[ix] /= lv;
  }
//    printf("Normalize row %d\n", rix);
//    MtxDisplay(m);
}

void matrix_to_rref(struct Matrix *m)
{
  int lead;
  int rix, iix;
  int lv;
  int rowCount = m->dim_y;
 
  lead = 0;
  for (rix = 0; rix < rowCount; rix++) {
    if (lead >= m->dim_x)
      return;
    iix = rix;
    while (m->matrix[iix][lead] == 0) {
      iix++;
      if (iix == rowCount) {
        iix = rix;
        lead++;
        if (lead == m->dim_x)
          return;
      }
    }
    MtxSwapRows(m, iix, rix );
    MtxNormalizeRow(m, rix, lead );
    for (iix = 0; iix < rowCount; iix++) {
      if (iix != rix ) {
        lv = m->matrix[iix][lead];
        MtxMulAndAddRows(m, iix, rix, -lv) ;
      }
    }
    lead++;
  }
}
/***** END ROSETTA CODE SECTION *****/

struct Matrix * generate_matrix_from_sdfg(struct Sdfg *sdfg) {
  struct Matrix *matrix;
  std::list<int *> rows;
  struct Node *n;
  struct Node *dst;
  int *current_row = NULL;
  
  matrix = (struct Matrix *) malloc(sizeof(struct Matrix));
  // TODO: check mem allocation
  // +1 0 per row representing solution [M][Q] = [O]
  matrix->dim_x = sdfg->num_nodes + 1;
  matrix->dim_y = 0;
  for (int i = 0; i < sdfg->num_nodes; i++) {
    n = &sdfg->node_list[i];
    printf("PARSING ROW FOR NODE %c_%d\n\r", n->type, n->id);
    for (int j = 0; j < n->num_outputs; j++) {
      dst = n->outputs[j]->output;
      if (dst->type == 'O') {
        // skip nodes that output to outside world
        continue;
      }
      printf("ADDING ROW FOR PAIR %c_%d, %c_%d\n\r", n->type, n->id, dst->type, dst->id);
      current_row = (int *) malloc(sizeof(int) * matrix->dim_x);
      // TODO: check mem allocation
      memset(current_row, 0, sizeof(int) * matrix->dim_x);
      rows.push_back(current_row);
      if (n == dst) {
        continue;
      }
      if (n->type == 'U') {
        current_row[n->id] = n->constant_a;
      } else {
        current_row[n->id] = 1;
      }
      if (dst->type == 'D') {
        current_row[dst->id] = -1 * dst->constant_a;
      } else {
        current_row[dst->id] = -1;
      }
    }
  }
  
  matrix->dim_y = rows.size();
  matrix->matrix = (int **) malloc(sizeof(int *) * matrix->dim_y);
  // TODO: check mem allocation
  for (int i = 0; i < matrix->dim_y; i++) {
    printf("ADDED ROW TO MATRIX\n\r");
    matrix->matrix[i] = rows.front(); // rows.front is a reference
    rows.pop_front();
  }
  
  return matrix;
}

void print_matrix(struct Matrix *m) {
  for (int i = 0; i < m->dim_y; i++) {
    for (int j = 0; j < m->dim_x; j++) {
      printf("%d ", m->matrix[i][j]);
    }
    printf("\n\r");
  }
}

int get_rref_rank(struct Matrix *rref_m) {
  int row_rank = 0;
  for (int i = rref_m->dim_y - 1; i >= 0; i--) {
    for (int j = 0; j < rref_m->dim_x; j++) {
      if (rref_m->matrix[i][j]) {
        row_rank = i;
        return row_rank + 1;
      }
    }
  }
  return -1;
}

void free_matrix(struct Matrix *m) {
  for (int i = 0; i < m->dim_y; i++) {
    free(m->matrix[i]);
  }
  free(m->matrix);
}