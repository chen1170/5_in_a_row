#ifndef PARALLEL_H
#define PARALLEL_H

#include "board.h"

void init_parallel_env();
void cleanup_parallel_env();
int get_best_parallel(int show_board, P_Colour current_player);
void parallel_worker();

#endif // PARALLEL_H