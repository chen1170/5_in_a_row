#ifndef PARALLEL_H
#define PARALLEL_H

#include "board.h"

void init_parallel_env();
void cleanup_parallel_env();
//int get_best_move_parallel(int * from_row, int * from_col, int * to_row, int * to_col, Piece * board, P_Colour current_player);
int get_best_parallel(P_Colour current_player);
void parallel_worker();

#endif // PARALLEL_H