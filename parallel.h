#ifndef PARALLEL_H
#define PARALLEL_H

#include "board.h"

void init_parallel_env();
void cleanup_parallel_env();
void get_best_move_parallel(int *row, int *col, PieceType current_player);
void parallel_worker(PieceType current_player);
int evaluate_move(int row, int col, PieceType player, int depth);
int parallel_minimax(int depth, int alpha, int beta, PieceType player);

#endif // PARALLEL_H