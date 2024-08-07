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
int work_signalBuf;
int work_signal = 1;
int terminate_signal = 0;
int WORK_TAG = 0;
int IDLE_TAG = 1;
int task_count = 0;
//char board_copy[X_BOARD_SIZE_X * X_BOARD_SIZE_Y];
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

int get_best_move_parallel(int *from_row, int *from_col, int *to_row, int *to_col, Piece *board, P_Colour current_player)
{
    if (rank == 0)
    {
        MPI_Request reqs[size];
        //printf("Parallel AI move... current player is %d\n", current_player);
        // Wake up workers
        signalBuf = 1;
        for (int i = 1; i < size; i++)
        {
            MPI_Isend(&signalBuf, 1, MPI_INT, i, WORK_TAG, MPI_COMM_WORLD, &reqs[i]);
        }

        // 主进程逻辑

        // Send the board to all processes...
        createPieceMPIType(&pieceType);
        int piece_size = sizeof(Piece);
        int board_size = X_BOARD_SIZE_X * X_BOARD_SIZE_Y;
        //MPI_Bcast(board, board_size, pieceType, 0, MPI_COMM_WORLD);

        int moves[X_BOARD_SIZE_X * X_BOARD_SIZE_Y][2];
        int move_count = 0;

        // How many pieces for this player on the board
        int piece_count = 0;
        for (int i = 0; i < X_BOARD_SIZE_X; i++)
        {
            for (int j = 0; j < X_BOARD_SIZE_Y; j++)
            {
                if (get_piece(i, j).colour == current_player)
                {
                    piece_count++;
                }
            }
        }

        //printf("Rank(%d) get_best_move_parallel\n", rank);

        // Index all of the pieces for this player.
        int pieces_indexed = 0;
        int piece_index[piece_count][5];
        // [0] and [1] are the from_row and from_col
        // [2] and [3] will be the to_row and to_col
        // [4] will be the score (as projected after MAX_DEPTH moves down this path)
        for (int i = 0; i < X_BOARD_SIZE_X; i++)
        {
            for (int j = 0; j < X_BOARD_SIZE_Y; j++)
            {
                if (get_piece(i, j).colour == current_player)
                {
                    piece_index[pieces_indexed][0] = i;
                    piece_index[pieces_indexed][1] = j;
                    pieces_indexed++;
                }
            }
        }

        //printf("Rank(%d) piece_count: %d\n", rank, piece_count);

        // Now wait for workers to report they are available to work
        int idle_workers = size - 1;
        int piece_index_count = 0;
        while (piece_index_count < pieces_indexed)
        {
            //printf("Master waiting for idle signal...piece_index_count = %d \n", piece_index_count);
            MPI_Recv(&signalBuf, 1, MPI_INT, MPI_ANY_SOURCE, IDLE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            //printf("Master received idle signal from %d\n", signalBuf);

            // Send work to the worker

            MPI_Send(board, board_size, pieceType, signalBuf, 0, MPI_COMM_WORLD);
            int piece[4];
            piece[0] = piece_index_count;                 // The index of of the piece in the piece_index array
            piece[1] = piece_index[piece_index_count][0]; // The row of the piece on the board
            piece[2] = piece_index[piece_index_count][1]; // The col of the piece on the board
            piece[3] = current_player;                       // The number of pieces for this player on the board
            //printf("Master sending piece: %d at (%d,%d) to worker %d\n", piece[0], piece[1], piece[2], signalBuf);
            MPI_Send(piece, 4, MPI_INT, signalBuf, 0, MPI_COMM_WORLD);
            piece_index_count++;
            piece_count--;
            idle_workers--;

            if (idle_workers == 0)
            {
                // Receieve results back from the workers!
                int move[4];
                // [0] will be the piece index
                // [1] and [2] will be the to_row and to_col
                // [3] will be the score

                MPI_Recv(move, 4, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                //printf("Master received move for piece at index %d\n", move[0]);
                // Update piece_index with the best move found for that piece
                piece_index[move[0]][2] = move[1];
                piece_index[move[0]][3] = move[2];
                piece_index[move[0]][4] = move[3];
                idle_workers++;

                // Update best score and move
                int best_score = INT_MIN;
                for (int i = 0; i < pieces_indexed; i++)
                {
                    if (piece_index[i][4] > best_score)
                    {
                        //printf("Master updating best move for piece at index %d\n", i);
                        best_score = piece_index[i][4];
                        *from_row = piece_index[i][0];
                        *from_col = piece_index[i][1];
                        *to_row = piece_index[i][2];
                        *to_col = piece_index[i][3];
                    }
                }

                int _signalBuf = 1;
                for (int i = 1; i < size; i++)
                {
                    MPI_Isend(&_signalBuf, 1, MPI_INT, i, WORK_TAG, MPI_COMM_WORLD, &reqs[i]);
                }
            }
        }

        //printf("Master done waiting for workers\n");

        //printf("Rank(%d) get_best_move_parallel done\n", rank);

        //MPI_Barrier(MPI_COMM_WORLD);
        MPI_Type_free(&pieceType);
        return 0;
    }
    else
    {
        printf("This should not be called by any process other than rank=0, calling rank is %d\n", rank);
        return 1;
    }
}

void parallel_worker()
{
    P_Colour current_player;

    // Wait for a signal to do work...

    createPieceMPIType(&pieceType);

    int piece_size = sizeof(Piece);

    int board_size = X_BOARD_SIZE_X * X_BOARD_SIZE_Y;
    
    int board_size_bytes = board_size * piece_size;

    task_count = 0;

    while (1)
    {
        //printf("Worker %d waiting for work signal...\n", rank);
        MPI_Recv(&work_signalBuf, 1, MPI_INT, 0, WORK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (work_signalBuf == work_signal)
        {
            //if (task_count == 0)
            //{
            //printf("Worker %d working...\n", rank);

            task_count++;

            // Receive the board from the master

            // printf("Worker %d waiting for board...\n", rank);
            //MPI_Bcast(board, board_size, pieceType, 0, MPI_COMM_WORLD);

            // printf("Worker %d received board\n", rank);
            //}

            // Now we need to play the game making up to MAX_DEPTH moves
            // Send idle signal to master
            signalBuf = rank;
            //printf("Worker %d sending idle signal\n", rank);
            MPI_Send(&signalBuf, 1, MPI_INT, 0, IDLE_TAG, MPI_COMM_WORLD);


            MPI_Recv(board, board_size, pieceType, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // Receive work to the worker
            // printf("Worker %d waiting for piece...\n", rank);
            int piece[4];
            MPI_Recv(piece, 4, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            int from_row = piece[1];
            int from_col = piece[2];
            current_player = piece[3];
            
            //printf("Worker: %d, Piece: %d %d %d\n", rank, piece[0], piece[1], piece[2]);
            
            // Okay, now using the board, and this information as the starting move... play the game for MAX_DEPTH moves
            // and then evaluate the board to see how good the move was.

            int moves_left_to_evaluate = MAX_DEPTH;

            int move[4];
            // [0] will be the piece index
            // [1] and [2] will be the to_row and to_col
            // [3] will be the score

            // Update piece_index with the best move found for that piece
            move[0] = piece[0];
            move[1] = -1;
            move[2] = -1;
            move[3] = -1;

            for (int i = 0; i < X_BOARD_SIZE_X; i++)
            {
                for (int j = 0; j < X_BOARD_SIZE_Y; j++)
                {
                    // Don't try moving to our own position!
                    //if (i != from_row && j != from_col)
                    //{
                        //printf("Worker %d evaluating pince as (%d, %d) -> (%d %d)\n", rank, from_row, from_col, i, j);

                        //if (is_valid_move(from_row, from_col, i, j, current_player))
                        //{
                            //printf("Worker %d VALID MOVE %d %d %d %d\n", rank, from_row, from_col, i, j);

                            // So, evaluate this move
                            int score = evaluate_move(from_row, from_col, i, j, current_player, MAX_DEPTH);

                            //printf("Worker %d score for move %d %d %d %d is %d\n", rank, from_row, from_col, i, j, score);

                            // This is the only move we need to pass back to the master
                            if (score > move[3])
                            {
                                move[1] = i;
                                move[2] = j;
                                move[3] = score;
                            }
                        //}

                        //printf("NOT VALID - Worker %d done evaluating move %d %d %d %d\n", rank, from_row, from_col, i, j);
                    //}
                }
            }
            
            //printf("Worker %d sending move for piece at index %d\n", rank, move[0]);
            // Send the move back to the master
            MPI_Send(move, 4, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }

        if (signalBuf == terminate_signal)
        {
            printf("Process %d terminating... %d tasks completed.\n", rank, task_count);
            break;
        }
    }

    MPI_Type_free(&pieceType);
}
int evaluate_move(int from_row, int from_col, int to_row, int to_col, P_Colour player, int depth)
{
    //print_board();

    //printf(" -->Evaluating move (%d %d) =>  (%d %d), depth = %d\n", from_row, from_col, to_row, to_col, depth);

    int score = 0;
    
    // We don't need to remember any of these moves, we just need to pass back the score so that the first move maps
    // to the move tree with the best resulting score
    if (depth == 0)
    {
        //printf(" -->Depth 0\n");
        return evaluate_board(player);
    }

    if (is_valid_move(from_row, from_col, to_row, to_col, player))
    {
        //printf(" -->Valid move\n");
        // Make a copy of the game board
        Piece * board_copy;
        board_copy = malloc(X_BOARD_SIZE_X * X_BOARD_SIZE_Y * sizeof(Piece));
        memcpy(board_copy, board, X_BOARD_SIZE_X * X_BOARD_SIZE_Y * sizeof(Piece));

        //printf(" -->Made copy\n");

        // Make the move
        update_board(from_row, from_col, to_row, to_col, player);

        //printf(" -->Updated board\n");

        for (int i = 0; i < X_BOARD_SIZE_X; i++)
        {
            for (int j = 0; j < X_BOARD_SIZE_Y; j++)
            {
                // Go one level deeper...
                if (player == BLACK)
                    score = evaluate_move(from_row, from_col, i, j, WHITE, --depth);
                else
                    score = evaluate_move(from_row, from_col, i, j, BLACK, --depth);
            }
        }

        //printf(" -->Done with depth\n");

        // Undo the move
        memcpy(board, board_copy, X_BOARD_SIZE_X * X_BOARD_SIZE_Y * sizeof(Piece));
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