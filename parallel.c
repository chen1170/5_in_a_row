#include <mpi.h>
#include <omp.h>
#include <limits.h>
#include "parallel.h"
#include "board.h"

#include <stdio.h>

#define MAX_DEPTH 6
#define MAX_PLAYER BLACK
#define MIN_PLAYER WHITE

static int evaluate_board();
static int is_game_over();
static int count_consecutive(int row, int col, int dx, int dy, PieceType piece);
static int evaluate_position(int row, int col, PieceType piece);

extern int rank, size;  // Declared in main

MPI_Request req;
int flag;
int signalBuf;
int work_signal = 1;
int terminate_signal = 0;
int WORK_TAG = 0;
int IDLE_TAG = 1;
int task_count = 0;

void init_parallel_env() {
    omp_set_num_threads(omp_get_max_threads());
}

void cleanup_parallel_env() {
    // 如果需要清理并行环境，可以在这里添加代码
    // Send termination signals to other MPI processes to shutdown the workers
        signalBuf = 0;
        for (int i = 1; i < size; i++) {
            printf("Sending term signal to p(%d)\n", i);
            MPI_Isend(&signalBuf, 1, MPI_INT, i, WORK_TAG, MPI_COMM_WORLD, &req);
        }
}

void get_best_move_parallel(int *row, int *col, PieceType current_player) {
    if (rank == 0) {
        // Wake up workers
        signalBuf = 1;
        for (int i = 1; i < size; i++) {
            MPI_Isend(&signalBuf, 1, MPI_INT, i, WORK_TAG, MPI_COMM_WORLD, &req);
        }

        // 主进程逻辑
        int moves[BOARD_SIZE * BOARD_SIZE][2];
        int move_count = 0;

        // 生成所有可能的移动
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                if (is_valid_move(i, j)) {
                    moves[move_count][0] = i;
                    moves[move_count][1] = j;
                    move_count++;
                }
            }
        }

        // 分配移动给工作进程
        int moves_per_process = move_count / (size - 1);
        for (int i = 1; i < size; i++) {
            int start = (i - 1) * moves_per_process;
            int end = (i == size - 1) ? move_count : start + moves_per_process;
            MPI_Send(&moves[start], (end - start) * 2, MPI_INT, i, 0, MPI_COMM_WORLD);
        }

        // 接收结果
        int best_score = (current_player == MAX_PLAYER) ? INT_MIN : INT_MAX;
        int best_move[2];
        for (int i = 1; i < size; i++) {
            int score;
            int move[2];
            MPI_Recv(&score, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(move, 2, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if ((current_player == MAX_PLAYER && score > best_score) ||
                (current_player == MIN_PLAYER && score < best_score)) {
                best_score = score;
                best_move[0] = move[0];
                best_move[1] = move[1];
            }
        }

        *row = best_move[0];
        *col = best_move[1];
    } else {
        // Don't launch wokers here... they are launched from main.
        //parallel_worker(current_player);
    }
}

void parallel_worker(PieceType current_player) {
    // Wait for a signal to do work...
    while (1)
        {
            MPI_Recv(&signalBuf, 1, MPI_INT, 0, WORK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if (signalBuf == work_signal)
            {
                task_count++;

                int moves[BOARD_SIZE * BOARD_SIZE][2];
                MPI_Status status;
                MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
                int move_count;
                MPI_Get_count(&status, MPI_INT, &move_count);
                move_count /= 2;
                MPI_Recv(moves, move_count * 2, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                int best_score = (current_player == MAX_PLAYER) ? INT_MIN : INT_MAX;
                int best_move[2];
                
                #pragma omp parallel for schedule(dynamic)
                for (int i = 0; i < move_count; i++) {
                    int score = evaluate_move(moves[i][0], moves[i][1], current_player, MAX_DEPTH);
                    #pragma omp critical
                    {
                        if ((current_player == MAX_PLAYER && score > best_score) ||
                            (current_player == MIN_PLAYER && score < best_score)) {
                            best_score = score;
                            best_move[0] = moves[i][0];
                            best_move[1] = moves[i][1];
                        }
                    }
                }

                // 发送最佳结果给主进程
                MPI_Send(&best_score, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Send(best_move, 2, MPI_INT, 0, 0, MPI_COMM_WORLD);

            }

            if (signalBuf == terminate_signal)
            {
                printf("Process %d terminating... %d tasks completed.\n", rank, task_count);
                break;
            }
        }
}

int evaluate_move(int row, int col, PieceType player, int depth) {
    update_board(row, col, player);
    int score = parallel_minimax(depth - 1, INT_MIN, INT_MAX, (player == MAX_PLAYER) ? MIN_PLAYER : MAX_PLAYER);
    update_board(row, col, EMPTY);
    return score;
}

int parallel_minimax(int depth, int alpha, int beta, PieceType player) {
    if (depth == 0 || is_game_over()) {
        return evaluate_board();
    }

    int best_score = (player == MAX_PLAYER) ? INT_MIN : INT_MAX;

    #pragma omp parallel for reduction(max:best_score) if(depth > 2)
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (is_valid_move(i, j)) {
                update_board(i, j, player);
                int score = parallel_minimax(depth - 1, alpha, beta, (player == MAX_PLAYER) ? MIN_PLAYER : MAX_PLAYER);
                update_board(i, j, EMPTY);

                if (player == MAX_PLAYER) {
                    best_score = (score > best_score) ? score : best_score;
                    alpha = (alpha > best_score) ? alpha : best_score;
                } else {
                    best_score = (score < best_score) ? score : best_score;
                    beta = (beta < best_score) ? beta : best_score;
                }

                /*if (beta <= alpha) {
                    return best_score;
                }*/
            }
        }
    }

    return best_score;
}

static int evaluate_board() {
    int score = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            PieceType piece = get_piece(i, j);
            if (piece != EMPTY) {
                score += evaluate_position(i, j, piece) * (piece == BLACK ? 1 : -1);
            }
        }
    }
    return score;
}

static int evaluate_position(int row, int col, PieceType piece) {
    int score = 0;
    const int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

    for (int d = 0; d < 4; d++) {
        int count = count_consecutive(row, col, directions[d][0], directions[d][1], piece);
        switch (count) {
            case 5: score += 1000000; break;  // 五子连珠
            case 4: score += 10000; break;    // 活四或冲四
            case 3: score += 1000; break;     // 活三
            case 2: score += 100; break;      // 活二
            default: break;
        }
    }

    return score;
}

static int count_consecutive(int row, int col, int dx, int dy, PieceType piece) {
    int count = 0;
    for (int i = -4; i <= 4; i++) {
        int r = row + i * dx;
        int c = col + i * dy;
        if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE) continue;
        if (get_piece(r, c) == piece) {
            count++;
            if (count == 5) return count;
        } else {
            count = 0;
        }
    }
    return (count >= 5) ? 5 : count;
}

static int is_game_over() {
    // 检查是否有玩家获胜
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            PieceType piece = get_piece(i, j);
            if (piece != EMPTY) {
                if (check_win(i, j, piece)) {
                    return 1;
                }
            }
        }
    }

    // 检查是否还有空位
    return is_board_full();
}