#include <mpi.h>
#include <omp.h>
#include <limits.h>
#include "x_parallel.h"
#include "x_board.h"
#include "x_game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DEPTH 2
#define MAX_PLAYER BLACK
#define MIN_PLAYER WHITE

// static int evaluate_board();
// static int is_game_over();
// static int count_consecutive(int row, int col, int dx, int dy, PieceType piece);
// static int evaluate_position(int row, int col, PieceType piece);

extern int rank, size; // Declared in main

MPI_Request req;
MPI_Datatype pieceType;
int flag;
int signalBuf;
int work_signal = 1;
int terminate_signal = 0;
int WORK_TAG = 0;
int IDLE_TAG = 1;
int task_count = 0;
char board_copy[X_BOARD_SIZE_X * X_BOARD_SIZE_Y];
Piece board_worker_copy[X_BOARD_SIZE_X][X_BOARD_SIZE_Y];

void init_parallel_env()
{
    omp_set_num_threads(omp_get_max_threads());
}

void cleanup_parallel_env()
{
    // 如果需要清理并行环境，可以在这里添加代码
    // Send termination signals to other MPI processes to shutdown the workers
    signalBuf = 0;
    for (int i = 1; i < size; i++)
    {
        printf("Sending term signal to p(%d)\n", i);
        MPI_Isend(&signalBuf, 1, MPI_INT, i, WORK_TAG, MPI_COMM_WORLD, &req);
    }
}

void createPieceMPIType(MPI_Datatype *newtype)
{
    const int items = 4;
    int blocklengths[4] = {1, 1, 1, 1};
    MPI_Datatype types[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_CHAR};
    MPI_Aint offsets[4];

    offsets[0] = offsetof(Piece, type);
    offsets[1] = offsetof(Piece, colour);
    offsets[2] = offsetof(Piece, code);
    offsets[3] = offsetof(Piece, board_code);

    MPI_Type_create_struct(items, blocklengths, offsets, types, newtype);
    MPI_Type_commit(newtype);
}

int get_best_move_parallel(int *from_row, int *from_col, int *to_row, int *to_col, Piece *board, P_Colour current_player) {
    if (rank == 0) {
        signalBuf = work_signal;
        for (int i = 1; i < size; i++) {
            MPI_Isend(&signalBuf, 1, MPI_INT, i, WORK_TAG, MPI_COMM_WORLD, &req);
            MPI_Wait(&req, MPI_STATUS_IGNORE);
        }

        createPieceMPIType(&pieceType);
        int board_size = X_BOARD_SIZE_X * X_BOARD_SIZE_Y;
        MPI_Bcast(board, board_size, pieceType, 0, MPI_COMM_WORLD);

        int piece_count = 0;
        for (int i = 0; i < X_BOARD_SIZE_X; i++) {
            for (int j = 0; j < X_BOARD_SIZE_Y; j++) {
                if (get_piece(i, j).colour == current_player) {
                    piece_count++;
                }
            }
        }

        int piece_index = 0;
        int results_received = 0;
        MPI_Request send_request, recv_request;
        MPI_Status status;

        int best_score = INT_MIN;
        int best_from_row = -1, best_from_col = -1, best_to_row = -1, best_to_col = -1;

        while (results_received < piece_count) {
            printf("Master waiting for idle signal...\n");
            MPI_Irecv(&signalBuf, 1, MPI_INT, MPI_ANY_SOURCE, IDLE_TAG, MPI_COMM_WORLD, &recv_request);
            MPI_Wait(&recv_request, &status);

            if (piece_index < piece_count) {
                int from_row = -1;
                int from_col = -1;
                int count = 0;
                for (int i = 0; i < X_BOARD_SIZE_X; i++) {
                    for (int j = 0; j < X_BOARD_SIZE_Y; j++) {
                        if (get_piece(i, j).colour == current_player) {
                            if (count == piece_index) {
                                from_row = i;
                                from_col = j;
                                break;
                            }
                            count++;
                        }
                    }
                    if (from_row != -1) {
                        break;
                    }
                }

                int piece[4] = { piece_index, from_row, from_col, current_player };
                printf("Master sending piece %d (%d, %d) to worker %d\n", piece_index, from_row, from_col, signalBuf);
                MPI_Isend(piece, 4, MPI_INT, signalBuf, 0, MPI_COMM_WORLD, &send_request);
                MPI_Wait(&send_request, &status);
                piece_index++;
            }

            if (results_received < piece_index) {
                int move[4];
                printf("Master waiting for move from worker...\n");
                MPI_Irecv(move, 4, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &recv_request);
                MPI_Wait(&recv_request, &status);
                printf("Master received move for piece %d\n", move[0]);
                
                if (move[3] > best_score) {
                    best_score = move[3];
                    best_from_row = move[1];
                    best_from_col = move[2];
                    best_to_row = move[1];
                    best_to_col = move[2];
                }
                
                results_received++;
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Type_free(&pieceType);

        *from_row = best_from_row;
        *from_col = best_from_col;
        *to_row = best_to_row;
        *to_col = best_to_col;

        return 0;
    } else {
        return 1;
    }
}


void parallel_worker() {
    P_Colour current_player;

    createPieceMPIType(&pieceType);
    int piece_size = sizeof(Piece);

    while (1) {
        printf("Worker %d waiting for signal...\n", rank);
        MPI_Irecv(&signalBuf, 1, MPI_INT, 0, WORK_TAG, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        if (signalBuf == work_signal) {
            printf("Worker %d working...\n", rank);
            task_count++;

            int board_size = X_BOARD_SIZE_X * X_BOARD_SIZE_Y;
            MPI_Bcast(board, board_size, pieceType, 0, MPI_COMM_WORLD);

            printf("Worker %d received board\n", rank);

            signalBuf = rank;
            MPI_Isend(&signalBuf, 1, MPI_INT, 0, IDLE_TAG, MPI_COMM_WORLD, &req);
            MPI_Wait(&req, MPI_STATUS_IGNORE);
            printf("Worker %d sent idle signal\n", rank);

            while (1) {
                int piece[4];
                MPI_Irecv(piece, 4, MPI_INT, 0, 0, MPI_COMM_WORLD, &req);
                MPI_Wait(&req, MPI_STATUS_IGNORE);

                int from_row = piece[1];
                int from_col = piece[2];
                current_player = piece[3];

                printf("Worker %d received piece (%d, %d)\n", rank, from_row, from_col);

                int move[4] = { piece[0], -1, -1, -1 };
                int best_score = INT_MIN;
                for (int i = 0; i < X_BOARD_SIZE_X; i++) {
                    for (int j = 0; j < X_BOARD_SIZE_Y; j++) {
                        if (is_valid_move(from_row, from_col, i, j, current_player)) {
                            int score = evaluate_move(from_row, from_col, i, j, current_player, MAX_DEPTH);
                            if (score > best_score) {
                                best_score = score;
                                move[1] = i;
                                move[2] = j;
                                move[3] = score;
                            }
                        }
                    }
                }

                printf("Worker %d sending move for piece %d\n", rank, piece[0]);
                MPI_Isend(move, 4, MPI_INT, 0, 0, MPI_COMM_WORLD, &req);
                MPI_Wait(&req, MPI_STATUS_IGNORE);
                printf("Worker %d sent move for piece %d\n", rank, piece[0]);

                MPI_Isend(&rank, 1, MPI_INT, 0, IDLE_TAG, MPI_COMM_WORLD, &req);
                MPI_Wait(&req, MPI_STATUS_IGNORE);
                printf("Worker %d sent idle signal again\n", rank);
            }

        } else if (signalBuf == terminate_signal) {
            printf("Process %d terminating... %d tasks completed.\n", rank, task_count);
            break;
        }
    }

    MPI_Type_free(&pieceType);
}



int evaluate_move(int from_row, int from_col, int to_row, int to_col, P_Colour player, int depth)
{
    print_board();

    printf(" -->Evaluating move %d %d %d %d\n", from_row, from_col, to_row, to_col);

    int score = 0;
    
    // We don't need to remember any of these moves, we just need to pass back the score so that the first move maps
    // to the move tree with the best resulting score
    if (depth == 0)
    {
        printf(" -->Depth 0\n");
        return evaluate_board(player);
    }

    if (is_valid_move(from_row, from_col, to_row, to_col, player))
    {
        // Make a copy of the game board
        Piece *board_copy = malloc(X_BOARD_SIZE_X * X_BOARD_SIZE_Y * sizeof(Piece));
        if (board_copy == NULL) {
            fprintf(stderr, "Error: unable to allocate memory for board copy\n");
            return score;
        }
        memcpy(board_copy, board, X_BOARD_SIZE_X * X_BOARD_SIZE_Y * sizeof(Piece));

        // Make the move
        update_board(from_row, from_col, to_row, to_col, player);

        // Go one level deeper...
        for (int i = 0; i < X_BOARD_SIZE_X; i++) {
            for (int j = 0; j < X_BOARD_SIZE_Y; j++) {
                if (i != from_row || j != from_col) {
                    int next_score = evaluate_move(to_row, to_col, i, j, player, depth - 1);
                    if (next_score > score) {
                        score = next_score;
                    }
                }
            }
        }

        // Undo the move
        memcpy(board, board_copy, X_BOARD_SIZE_X * X_BOARD_SIZE_Y * sizeof(Piece));
        free(board_copy);
    }

    return score; 
}

static int evaluate_board(P_Colour player) {
    // Had to come up with something, so I just count the piece values (loosely based on what I read online)
    int score = 0;
    for (int i = 0; i < X_BOARD_SIZE_X; i++) {
        for (int j = 0; j < X_BOARD_SIZE_Y; j++) {
            if (board[i][j].colour == player) {
                switch (board[i][j].type) {
                    case BING: score += 1; break;   // 2 if passed the river... need code for that
                    case SHI: score += 2; break;
                    case XIANG: score += 3; break;
                    case MA: score += 4; break;
                    case PAO: score += 5; break;
                    case JU: score += 10; break;
                    case JIANG: score += 1000; break;
                    default: break;
                }
            }
        }
    }
    return score;
}