#ifndef PARALLEL_H
#define PARALLEL_H

#include "x_board.h"
#include <mpi.h>

void init_parallel_env();
void cleanup_parallel_env();
void createPieceMPIType(MPI_Datatype *newtype);
int get_best_move_parallel(int *from_row, int *from_col, int *to_row, int *to_col, Piece board[X_BOARD_SIZE_X][X_BOARD_SIZE_Y], P_Colour current_player);
void parallel_worker();

#endif // PARALLEL_H