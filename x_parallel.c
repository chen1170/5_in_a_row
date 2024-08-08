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

typedef struct {
    int from_row;
    int from_col;
    int to_row;
    int to_col;
    int score;
} Move;

P_Colour current_player;  // Ensure current_player is declared

void init_parallel_env() {
    omp_set_num_threads(omp_get_max_threads());
}

void cleanup_parallel_env() {
    // Send termination signals to other MPI processes to shutdown the workers
    signalBuf = terminate_signal;
    for (int i = 1; i < size; i++) {
        printf("Sending term signal to p(%d)\n", i);
        MPI_Isend(&signalBuf, 1, MPI_INT, i, WORK_TAG, MPI_COMM_WORLD, &req);
    }
}

void createPieceMPIType(MPI_Datatype *newtype) {
    int lengths[4] = {1, 1, 1, 1};
    const MPI_Aint displacements[4] = {
        offsetof(Piece, type),
        offsetof(Piece, colour),
        offsetof(Piece, code),
        offsetof(Piece, board_code)
    };
    MPI_Datatype types[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_CHAR};
    MPI_Type_create_struct(4, lengths, displacements, types, newtype);
    MPI_Type_commit(newtype);
}

int evaluate_board(P_Colour player) {
    int score = 0;
    for (int i = 0; i < X_BOARD_SIZE_X; i++) {
        for (int j = 0; j < X_BOARD_SIZE_Y; j++) {
            if (board[i][j].colour == player) {
                // Add piece value to score (values are arbitrary for example)
                switch (board[i][j].type) {
                    case JU: score += 9; break;
                    case MA: score += 5; break;
                    case XIANG: score += 3; break;
                    case SHI: score += 2; break;
                    case JIANG: score += 100; break;
                    case PAO: score += 4; break;
                    case BING: score += 1; break;
                    default: break;
                }
            }
        }
    }
    return score;
}

int evaluate_move(int from_row, int from_col, int to_row, int to_col, P_Colour player, int depth) {
    if (depth == 0) return evaluate_board(player);

    Piece temp_board[X_BOARD_SIZE_X][X_BOARD_SIZE_Y];
    memcpy(temp_board, board, sizeof(temp_board));
    update_board(from_row, from_col, to_row, to_col, player);
    
    int best_score = (player == MAX_PLAYER) ? INT_MIN : INT_MAX;
    P_Colour next_player = (player == MAX_PLAYER) ? MIN_PLAYER : MAX_PLAYER;

    #pragma omp parallel for collapse(2)
    for (int i = 0; i < X_BOARD_SIZE_X; i++) {
        for (int j = 0; j < X_BOARD_SIZE_Y; j++) {
            if (board[i][j].colour == next_player) {
                for (int k = 0; k < X_BOARD_SIZE_X; k++) {
                    for (int l = 0; l < X_BOARD_SIZE_Y; l++) {
                        if (is_valid_move(i, j, k, l, next_player)) {
                            int score = evaluate_move(i, j, k, l, next_player, depth - 1);
                            #pragma omp critical
                            {
                                if (player == MAX_PLAYER) {
                                    if (score > best_score) best_score = score;
                                } else {
                                    if (score < best_score) best_score = score;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    memcpy(board, temp_board, sizeof(temp_board));
    return best_score;
}

int get_best_move_parallel(int *from_row, int *from_col, int *to_row, int *to_col, Piece board[X_BOARD_SIZE_X][X_BOARD_SIZE_Y], P_Colour current_player) {
    if (rank == 0) {
        int piece_count = 0;
        for (int i = 0; i < X_BOARD_SIZE_X; i++) {
            for (int j = 0; j < X_BOARD_SIZE_Y; j++) {
                if (board[i][j].colour == current_player) {
                    piece_count++;
                }
            }
        }

        int worker_count = size - 1;
        int pieces_sent = 0;
        int pieces_received = 0;
        Move best_move;
        best_move.score = (current_player == MAX_PLAYER) ? INT_MIN : INT_MAX;

        MPI_Request request;
        MPI_Status status;

        // 分发初始任务
        for (int i = 0; i < X_BOARD_SIZE_X && pieces_sent < piece_count && pieces_sent < worker_count; i++) {
            for (int j = 0; j < X_BOARD_SIZE_Y && pieces_sent < piece_count && pieces_sent < worker_count; j++) {
                if (board[i][j].colour == current_player) {
                    Move move = {i, j, -1, -1, 0};
                    int worker_id = pieces_sent % worker_count + 1;
                    MPI_Isend(&move, sizeof(Move), MPI_BYTE, worker_id, WORK_TAG, MPI_COMM_WORLD, &request);
                    MPI_Wait(&request, MPI_STATUS_IGNORE);
                    printf("Sent initial move (%d, %d) to worker %d\n", i, j, worker_id);
                    pieces_sent++;
                }
            }
        }

        // 接收和处理任务
        while (pieces_received < piece_count) {
            int idle_signal;
            Move move;

            // 接收空闲信号
            MPI_Recv(&idle_signal, 1, MPI_INT, MPI_ANY_SOURCE, IDLE_TAG, MPI_COMM_WORLD, &status);
            int worker_id = status.MPI_SOURCE;
            printf("Received idle signal from worker %d\n", worker_id);

            // 接收结果
            MPI_Recv(&move, sizeof(Move), MPI_BYTE, worker_id, WORK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("Received move result from worker %d with score %d\n", worker_id, move.score);

            // 更新最佳棋步
            if ((current_player == MAX_PLAYER && move.score > best_move.score) || 
                (current_player == MIN_PLAYER && move.score < best_move.score)) {
                best_move = move;
            }

            pieces_received++;

            // 分发新任务
            if (pieces_sent < piece_count) {
                for (int i = 0; i < X_BOARD_SIZE_X && pieces_sent < piece_count; i++) {
                    for (int j = 0; j < X_BOARD_SIZE_Y && pieces_sent < piece_count; j++) {
                        if (board[i][j].colour == current_player) {
                            Move new_move = {i, j, -1, -1, 0};
                            MPI_Isend(&new_move, sizeof(Move), MPI_BYTE, worker_id, WORK_TAG, MPI_COMM_WORLD, &request);
                            MPI_Wait(&request, MPI_STATUS_IGNORE);
                            printf("Sent move (%d, %d) to worker %d\n", i, j, worker_id);
                            pieces_sent++;
                        }
                    }
                }
            }
        }

        // 确保所有工作进程完成任务后再发送终止信号
        for (int i = 1; i <= worker_count; i++) {
            Move move = {-1, -1, -1, -1, 0};
            MPI_Isend(&move, sizeof(Move), MPI_BYTE, i, WORK_TAG, MPI_COMM_WORLD, &request);
            MPI_Wait(&request, MPI_STATUS_IGNORE);
            printf("Sent termination signal to worker %d\n", i);
        }

        *from_row = best_move.from_row;
        *from_col = best_move.from_col;
        *to_row = best_move.to_row;
        *to_col = best_move.to_col;

        return 0;
    } else {
        // parallel_worker();
        return 0;
    }
}


void parallel_worker() {
    while (1) {
        // 接收广播的棋盘
        MPI_Bcast(board, X_BOARD_SIZE_X * X_BOARD_SIZE_Y * sizeof(Piece), MPI_BYTE, 0, MPI_COMM_WORLD);
        printf("Worker %d received board broadcast\n", rank);

        while (1) {
            Move move;
            MPI_Status status;
            MPI_Request request;
            // 非阻塞接收任务
            MPI_Irecv(&move, sizeof(Move), MPI_BYTE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
            MPI_Wait(&request, &status);
            printf("Worker %d received move (%d, %d) with tag %d\n", rank, move.from_row, move.from_col, status.MPI_TAG);

            // 如果接收到的任务是终止信号，则退出循环
            if (move.from_row == -1) {
                printf("Worker %d received termination signal\n", rank);
                break;
            }

            // 处理任务
            move.score = evaluate_move(move.from_row, move.from_col, move.to_row, move.to_col, current_player, MAX_DEPTH);
            printf("Worker %d evaluated move (%d, %d) -> (%d, %d) with score %d\n", rank, move.from_row, move.from_col, move.to_row, move.to_col, move.score);

            // 非阻塞发送结果
            MPI_Isend(&move, sizeof(Move), MPI_BYTE, 0, WORK_TAG, MPI_COMM_WORLD, &request);
            MPI_Wait(&request, MPI_STATUS_IGNORE);
            printf("Worker %d sent result of move (%d, %d) -> (%d, %d) with score %d\n", rank, move.from_row, move.from_col, move.to_row, move.to_col, move.score);

            // 通知主进程该工作进程空闲
            int idle_signal = 1;
            MPI_Isend(&idle_signal, 1, MPI_INT, 0, IDLE_TAG, MPI_COMM_WORLD, &request);
            MPI_Wait(&request, MPI_STATUS_IGNORE);
            printf("Worker %d sent idle signal\n", rank);
        }
    }
}
