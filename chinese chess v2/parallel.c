#include <mpi.h>
#include <omp.h>
// #include <limits.h>
#include "parallel.h"
#include "board.h"
// #include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// static int evaluate_board();
// static int is_game_over();
// static int count_consecutive(int row, int col, int dx, int dy, PieceType piece);
// static int evaluate_position(int row, int col, PieceType piece);

MPI_Request req;
MPI_Datatype pieceType;
int flag;
int signalBuf;
int work_signalBuf;
int terminate_signal = 0;
int board_signal = 1;
int work_signal = 2;
int WORK_TAG = 0;
int IDLE_TAG = 1;
int task_count = 0;
// char board_copy[BOARD_SIZE_X * BOARD_SIZE_Y];
Piece board_worker_copy[BOARD_SIZE_X][BOARD_SIZE_Y];

void init_parallel_env()
{
    omp_set_num_threads(omp_get_max_threads());
}

void cleanup_parallel_env()
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    // 如果需要清理并行环境，可以在这里添加代码
    // Send termination signals to other MPI processes to shutdown the workers
    signalBuf = 0;
    for (int i = 1; i < size; i++)
    {
        //printf("Sending term signal to p(%d)\n", i);
        MPI_Isend(&terminate_signal, 1, MPI_INT, i, IDLE_TAG, MPI_COMM_WORLD, &req);
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
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Status status;

    if (rank == 0)
    {
        MPI_Request reqs[size - 1];
        MPI_Request reqs2[size - 1];
        // printf("Parallel AI move... current player is %d\n", current_player);

        // 主进程逻辑

        // Send the board to all processes...
        int piece_size = sizeof(Piece);
        int board_size = BOARD_SIZE_X * BOARD_SIZE_Y;
        int board_size_bytes = board_size * piece_size;
        createPieceMPIType(&pieceType);

        MPI_Bcast(board, board_size, pieceType, 0, MPI_COMM_WORLD);
        int MPI_Abort(MPI_Comm comm, int errorcode);

        // Alert workers to receive the board!
        for (int i = 1; i < size; i++)
        {
            // printf("Master sending board to worker %d\n", i);
            // MPI_Send(&work_signal, 1, MPI_INT, 0, IDLE_TAG, MPI_COMM_WORLD);
            MPI_Recv(&signalBuf, 1, MPI_INT, i, IDLE_TAG, MPI_COMM_WORLD, &status);
            MPI_Send(&board_signal, 1, MPI_INT, i, IDLE_TAG, MPI_COMM_WORLD);
            // printf("Master sent board to worker %d\n", i);
        }

        // How many pieces for this player on the board
        int num_of_pieces_to_evaluate = 0;
        for (int row = 0; row < BOARD_SIZE_X; row++)
        {
            for (int col = 0; col < BOARD_SIZE_Y; col++)
            {
                if (get_piece(row, col).colour == current_player)
                {
                    num_of_pieces_to_evaluate++;
                }
            }
        }

        //int total_work = piece_count;

        // printf("Rank(%d) get_best_move_parallel\n", rank);

        // Index all of the pieces for this player.
        int current_piece_index = 0;
        //int piece_collection[num_of_pieces_to_evaluate][5];

        int **piece_collection = (int **)malloc(num_of_pieces_to_evaluate * sizeof(int *));
        for (int i = 0; i < num_of_pieces_to_evaluate; i++) {
            piece_collection[i] = (int *)malloc(5 * sizeof(int));
        }

        // [0] and [1] are the from_row and from_col
        // [2] and [3] will be the evaluated best move to_row and to_col
        // [4] will be the score (as projected after MAX_DEPTH moves down this path)
        for (int row = 0; row < BOARD_SIZE_X; row++)
        {
            for (int col = 0; col < BOARD_SIZE_Y; col++)
            {
                if (get_piece(row, col).colour == current_player)
                {
                    piece_collection[current_piece_index][0] = row;
                    piece_collection[current_piece_index][1] = col;
                    piece_collection[current_piece_index][2] = -1;
                    piece_collection[current_piece_index][3] = -1;
                    piece_collection[current_piece_index][4] = -1;
                    current_piece_index++;
                }
            }
        }

        // Check if current_piece_index is correct
        if (current_piece_index != num_of_pieces_to_evaluate)
        {
            // printf("Piece count mismatch.. ??\n");
            return 1;
        }

        // Set defaults for the return pointers
        *from_row = piece_collection[0][0];
        *from_col = piece_collection[0][1];
        *to_row = piece_collection[0][2];
        *to_col = piece_collection[0][3];

        // printf("Rank(%d) piece_count: %d\n", rank, piece_count);

        // Now wait for workers to report they are available to work

        int piece_index_count = 0;
        int work_started = 0;
        int work_completed = 0;
        // printf("Master waiting for workers to be ready...\n");
        while (piece_index_count < num_of_pieces_to_evaluate)
        {
            // printf("Master best move: %d %d %d %d\n", *from_row, *from_col, *to_row, *to_col);

            MPI_Recv(&signalBuf, 1, MPI_INT, MPI_ANY_SOURCE, IDLE_TAG, MPI_COMM_WORLD, &status);

            // check for errors using MPI
            int available_worker = status.MPI_SOURCE;
            //printf("Master received idle signal from worker %d, sendinfg piece %d \n", available_worker, piece_index_count);

            // Send work to the worker
            int * piece = (int *)malloc(4 * sizeof(int));
            piece[0] = piece_index_count;                 // The index of of the piece in the piece_index array
            piece[1] = piece_collection[piece_index_count][0]; // The row of the piece on the board
            piece[2] = piece_collection[piece_index_count][1]; // The col of the piece on the board
            piece[3] = current_player;                    // The number of pieces for this player on the board

            MPI_Send(&work_signal, 1, MPI_INT, available_worker, IDLE_TAG, MPI_COMM_WORLD);
            MPI_Send(piece, 4, MPI_INT, available_worker, WORK_TAG, MPI_COMM_WORLD);
            piece_index_count++;

            // After each send, check to see if anyone is returning completed work

            MPI_Iprobe(MPI_ANY_SOURCE, WORK_TAG, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
            if (flag)
            {
                int * move = (int *)malloc(4 * sizeof(int));
                move[0] = -2;
                move[1] = -2;
                move[2] = -2;
                move[3] = -2;
                MPI_Recv(move, 4, MPI_INT, MPI_ANY_SOURCE, WORK_TAG, MPI_COMM_WORLD, &status);

                free(move);

                // check the values of move:
                //printf("Master received move for piece at index %d: %d %d %d\n", move[0], move[1], move[2], move[3]);


                // Receieve results back from the workers!

                // [0] will be the piece index
                // [1] and [2] will be the to_row and to_col
                // [3] will be the score

                // printf("Master received move for piece at index %d\n", move[0]);
                //  Update piece_index with the best move found for that piece
                piece_collection[move[0]][2] = move[1];
                piece_collection[move[0]][3] = move[2];
                piece_collection[move[0]][4] = move[3];

                // Update best score and move
                int best_score = -1;
                for (int i = 0; i < current_piece_index; i++)
                {
                    if (piece_collection[i][4] > best_score)
                    {
                        // printf("Master updating best move for piece at index %d\n", i);
                        best_score = piece_collection[i][4];
                        *from_row = piece_collection[i][0];
                        *from_col = piece_collection[i][1];
                        *to_row = piece_collection[i][2];
                        *to_col = piece_collection[i][3];
                    }
                }
            }

            free(piece);
        }

        // print the int values of the best move
        // printf("Master best move: %d %d %d %d\n", *from_row, *from_col, *to_row, *to_col);

        // check that moves have been updated
        if (*to_row == -1 || *to_col == -1)
        {
            // printf("Master best move not updated\n");
            return 1;
        }

        for (int i = 0; i < num_of_pieces_to_evaluate; i++) {
            free(piece_collection[i]);
        }
        free(piece_collection);

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
    int signalBuf;
    int work_signal = 1;
    int terminate_signal = 0;
    int receive_board_signal = 1;
    int receive_work_signal = 2;

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    P_Colour current_player;

    // Wait for a signal to do work...

    createPieceMPIType(&pieceType);

    int piece_size = sizeof(Piece);
    int board_size = BOARD_SIZE_X * BOARD_SIZE_Y;
    int board_size_bytes = board_size * piece_size;

    task_count = 0;

    //int from_row, from_col, to_row, to_col;


    while (1)
    {
        // Announce ready to work!
        MPI_Send(&work_signal, 1, MPI_INT, 0, IDLE_TAG, MPI_COMM_WORLD);
        // printf("Worker %d sent idle signal...\n", rank);

        // printf("Worker %d waiting for work signal...\n", rank);
        MPI_Recv(&signalBuf, 1, MPI_INT, 0, IDLE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // printf("Worker %d received idle signal...%d\n", rank, signalBuf);

        //printf("Worker %d doing work...\n", rank);

        if (signalBuf == terminate_signal)
        {
            //printf("Process %d terminating... %d tasks completed.\n", rank, task_count);
            break;
        }
        // We are being asked to do something, lets examine the signalBuf
        if (signalBuf == receive_board_signal)
        {
            MPI_Bcast(board, board_size, pieceType, 0, MPI_COMM_WORLD);
            // printf("Process %d received board\n", rank);
        }

        if (signalBuf == receive_work_signal)
        {
            // if (task_count == 0)
            //{
            // printf("Worker %d working...\n", rank);

            task_count++;

            // Since we do not have a complex algorithm for playing chess
            // we make simulate some complexity by evaluating 1 million random
            // moves for each piece.
            int attempts = 0;
            do {
                int from_row = rand() % BOARD_SIZE_X;
                int from_col = rand() % BOARD_SIZE_Y;
                int to_row = rand() % BOARD_SIZE_X;
                int to_col = rand() % BOARD_SIZE_Y;
                attempts++;

                is_valid_move(from_row, from_col, to_row, to_col, current_player);

            }
            while (attempts < MAX_SIMULATED_WORK);
            //printf("Done simulated work...%d\n", attempts);

            //printf("Worker %d done working on simulated work...%d\n", rank, attempts);


            // Receive work to the worker
            // printf("Worker %d waiting for piece...\n", rank);
            int * piece = (int *)malloc(4 * sizeof(int));;
            MPI_Recv(piece, 4, MPI_INT, 0, WORK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            int piece_index = piece[0];
            int from_row = piece[1];
            int from_col = piece[2];
            current_player = piece[3];

            // Check for errors in the piece array
            int errors_detected = 0;

            if (from_row < 0 || from_row >= BOARD_SIZE_X)
            {
                //fprintf(stderr, "Error: from_row is out of valid range.\n");
                errors_detected++;
            }
            if (from_col < 0 || from_col >= BOARD_SIZE_Y)
            {
                //fprintf(stderr, "Error: from_col is out of valid range.\n");
                errors_detected++;
            }
            if (current_player != RED && current_player != BLACK)
            {
                //fprintf(stderr, "Error: Invalid current_player value.\n");
                errors_detected++;
            }

            // printf("Worker: %d, Piece: %d %d %d\n", rank, piece[0], piece[1], piece[2]);

            // Okay, now using the board, and this information as the starting move... play the game for MAX_DEPTH moves
            // and then evaluate the board to see how good the move was.

            int moves_left_to_evaluate = MAX_DEPTH;

            int best_to_row = -1;
            int best_to_col = -1;
            int best_score = -1;

            if (errors_detected == 0)
            {
                for (int to_row = 0; to_row < BOARD_SIZE_X; to_row++)
                {
                    for (int to_col = 0; to_col < BOARD_SIZE_Y; to_col++)
                    {
                        // Don't try moving to our own position!
                        if (to_row != from_row && to_col != from_col)
                        {
                            // printf("Worker %d evaluating pince as (%d, %d) -> (%d %d)\n", rank, from_row, from_col, to_row, to_col);
                            
                            if (is_valid_move(from_row, from_col, to_row, to_col, current_player)) {
                                //printf("Worker %d VALID MOVE %d %d %d %d\n", rank, from_row, from_col, to_row, to_col);
                            
                                // So, evaluate this move
                                int score = evaluate_move(from_row, from_col, to_row, to_col, current_player, MAX_DEPTH);
                            
                                // Check if evaluate_move returned a valid score
                                if (score == -1) {
                                    fprintf(stderr, "Error: Failed to evaluate move.\n");
                                    return;
                                }
                            
                                // printf("Worker %d score for move %d %d %d %d is %d\n", rank, from_row, from_col, to_row, to_col, score);
                            
                                // This is the only move we need to pass back to the master
                                if (score > best_score) {
                                    best_to_row = to_row;
                                    best_to_col = to_col;
                                    best_score = score;
                                    //printf("New best score\n");
                                }

                                // If this move generates the same score as the current best
                                // change to the new move 50% of the time.
                                if (score == best_score) {
                                    if (rand() % 3 == 0)
                                    {
                                        //printf("Took rand row\n");
                                        best_to_row = to_row;
                                        best_to_col = to_col;
                                        best_score = score;
                                    }
                                    else
                                    {
                                        //printf("Did not take rand\n");
                                    }
                                }
                            }

                            // printf("NOT VALID - Worker %d done evaluating move %d %d %d %d\n", rank, from_row, from_col, i, j);
                        }
                    }
                }
            }

            // printf("Worker %d sending move for piece at index %d\n", rank, move[0]);
            //  Send the move back to the master

            int * move = (int *)malloc(4 * sizeof(int));
            //int move[4];
            // [0] will be the piece index
            // [1] and [2] will be the to_row and to_col
            // [3] will be the score

            // Update piece_index with the best move found for that piece
            int p = piece[0];
            //printf("Worker %d sending move for piece at index %d: %d %d %d\n", rank, p, best_to_row, best_to_col, best_score);
            move[0] = p;
            move[1] = best_to_row;
            move[2] = best_to_col;
            move[3] = best_score;
            //printf("Worker %d sending move for piece at index %d: %d %d %d\n", rank, move[0], move[1], move[2], move[3]);
            MPI_Send(move, 4, MPI_INT, 0, WORK_TAG, MPI_COMM_WORLD);

            free(move);
            free(piece);
        }
    }

    MPI_Type_free(&pieceType);
}