#include <mpi.h>
#include <omp.h>
#include "parallel.h"
#include "board.h"
// #include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

Piece board_worker_copy[BOARD_SIZE_X][BOARD_SIZE_Y];

void init_parallel_env()
{
    // Initialize OMP
    omp_set_num_threads(omp_get_max_threads());
}

void cleanup_parallel_env()
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Send termination signals to other MPI processes to shutdown the workers
    signalBuf = 0;
    for (int i = 1; i < size; i++)
    {
        // printf("Sending term signal to p(%d)\n", i);
        MPI_Isend(&terminate_signal, 1, MPI_INT, i, IDLE_TAG, MPI_COMM_WORLD, &req);
    }
}

void createPieceMPIType(MPI_Datatype *new_mpi_type)
{
    // Create a custom MPI data type for our piece struct, make life a little easier!
    // The piece struct is defined as three enums (ints) and a char as follows:

    // typedef struct {
    // P_Type type;
    // P_Colour colour;
    // P_TypeCode code;
    // char board_code;
    // } Piece;
    
    const int items = 4;
    int blocklengths[4] = {1, 1, 1, 1};
    MPI_Datatype types[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_CHAR};
    MPI_Aint offsets[4];

    offsets[0] = offsetof(Piece, type);
    offsets[1] = offsetof(Piece, colour);
    offsets[2] = offsetof(Piece, code);
    offsets[3] = offsetof(Piece, board_code);

    MPI_Type_create_struct(items, blocklengths, offsets, types, new_mpi_type);
    MPI_Type_commit(new_mpi_type);
}

int get_best_parallel(int show_board, P_Colour current_player)
{
    int from_row, from_col, to_row, to_col;

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Status status;

    if (rank == 0)
    {
        // Step 1: Initialize workers with a copy of the board

        // -- printf("Parallel AI move starting... current player is %d\n", current_player);
 
        // Alert workers to prepare to receive the board!
        for (int i = 1; i < size; i++)
        {
            // this tells the worker to get ready to work!
            MPI_Recv(&signalBuf, 1, MPI_INT, i, IDLE_TAG, MPI_COMM_WORLD, &status);
            
            // This sends a signal indicating what work they will need to do, in this
            // they are being asked to receiving the board:
            MPI_Send(&board_signal, 1, MPI_INT, i, IDLE_TAG, MPI_COMM_WORLD);
        }

        // Now that the workers are notified to be ready to receive the board...
        
        // First, get the board size
        int board_size = BOARD_SIZE_X * BOARD_SIZE_Y;

        // Create the custom MPI data type for the Piece struct:
        createPieceMPIType(&pieceType);

        // Now broadcast the board:
        MPI_Bcast(board, board_size, pieceType, 0, MPI_COMM_WORLD);

        // Step 2 - Figure out what pieces need to be evaluated

        // How many pieces for this player on the board?
        int num_of_pieces_to_evaluate = 0;

        // Loop over the board and count how pieces this player has left
        #pragma omp parallel for collapse(2)
        for (int row = 0; row < BOARD_SIZE_X; row++)
        {
            for (int col = 0; col < BOARD_SIZE_Y; col++)
            {
                // printf("Checking piece at %d %d\n", row, col);
                if (get_piece(row, col).colour == current_player)
                {
                    #pragma omp critical
                    num_of_pieces_to_evaluate++;
                }
            }
        }

        //p rintf("Number of pieces to evaluate: %d\n", num_of_pieces_to_evaluate);

        // Now that we know how many pieces we have to evaluate, we can
        // index all of the pieces for this player in a collection:
        int current_piece_index = 0;

        // Allocate memory for the piece collection
        int **piece_collection = (int **)malloc(num_of_pieces_to_evaluate * sizeof(int *));
        for (int i = 0; i < num_of_pieces_to_evaluate; i++) {
            piece_collection[i] = (int *)malloc(5 * sizeof(int));
        }

        // printf("Allocated piece collection\n");

        // [0] and [1] are the from_row and from_col
        // [2] and [3] will be the evaluated best move to_row and to_col
        // [4] will be the score (as projected after MAX_DEPTH moves down this path)
        
        #pragma omp parallel for collapse(2)
        for (int row = 0; row < BOARD_SIZE_X; row++)
        {
            for (int col = 0; col < BOARD_SIZE_Y; col++)
            {
                if (get_piece(row, col).colour == current_player)
                {
                    // The piece at this position belongs to the current player
                    piece_collection[current_piece_index][0] = row;
                    piece_collection[current_piece_index][1] = col;
                    piece_collection[current_piece_index][2] = -1;
                    piece_collection[current_piece_index][3] = -1;
                    piece_collection[current_piece_index][4] = -1;

                    #pragma omp critical
                    current_piece_index++;
                }
            }
        }

        // printf("Number of pieces to evaluate: %d\n", current_piece_index);

        // Check if current_piece_index is correct, meaning we found all the pieces
        if (current_piece_index != num_of_pieces_to_evaluate)
        {
            printf("Piece count mismatch.. ??\n");
            return 1;
        }

        // Step 3: Send work to the workers as they report they are available
        //         and we have work for them to complete...

        // Set defaults for the final values
        from_row = -1;
        from_col = -1;
        to_row = -1;
        to_col = -1;
        int best_score = -1;

        // printf("Rank(%d) piece_count: %d\n", rank, piece_count);

        // Now wait for workers to report they are available to work

        int piece_index_count = 0;
        int work_started = 0;
        int work_completed = 0;
        int piece_best_score = -1;
        int idle_workers = size - 1;

        // printf("Master waiting for workers to be ready...\n");

        // Loop until all pieces have been evaluated
        while(1)
        {
            // First check to see if we need and can to send any pieces
            // If piece_index_count is less than the number of pieces to evaluate
            // we still need someone to evaluate a piece.
            // If idle_workers is greater than 0, we have workers available to do work!
            if (piece_index_count < num_of_pieces_to_evaluate && idle_workers > 0)
            {
                // printf("Checking piece at index %d...\n", piece_index_count);
                int best_score = -1;

                // printf("Master best move: %d %d %d %d\n", *from_row, *from_col, *to_row, *to_col);

                // Available workers send an idle signal after they complete work
                // Since we have work to be done, and there are idle workers, we can receive a signal
                // from an availalbe worker o the IDLE_TAG...
                MPI_Recv(&signalBuf, 1, MPI_INT, MPI_ANY_SOURCE, IDLE_TAG, MPI_COMM_WORLD, &status);

                // Get the rank of the available worker:
                int available_worker = status.MPI_SOURCE;
                // printf("Master received idle signal from worker %d, sendinfg piece %d \n", available_worker, piece_index_count);

                // Send work to the worker:
                // Allocate memory for the piece array structure we want to send:
                int * piece = (int *)malloc(4 * sizeof(int));
                piece[0] = piece_index_count;                 // The index of of the piece in the piece_index array
                piece[1] = piece_collection[piece_index_count][0]; // The row of the piece on the board
                piece[2] = piece_collection[piece_index_count][1]; // The col of the piece on the board
                piece[3] = current_player;                    // The number of pieces for this player on the board

                // Send a work signal to the worker to let them know work is coming:
                MPI_Send(&work_signal, 1, MPI_INT, available_worker, IDLE_TAG, MPI_COMM_WORLD);

                // Send the work on the WORK_TAG with the piece as the data:
                MPI_Send(piece, 4, MPI_INT, available_worker, WORK_TAG, MPI_COMM_WORLD);
                
                // Increment the piece_index_count
                piece_index_count++;

                // Decrease the idel work count, we will increment this
                // when we get the work back from the worker
                idle_workers--;

                // Free the piece array that was allocated
                free(piece);
            }

            // Check to see if all of the work has been completed
            // This skips the MPI_Iprobe call that follows if 
            // all work is done...
            if (work_completed == num_of_pieces_to_evaluate)
            {
                //printf("Master done evaluating all pieces\n");
                break;
            }

            // After each send, check to see if anyone is returning completed work
            // We don't want to sit waiting for work, since we may have more
            // pieces to send to other workers, so we just probe to see if
            // worker is sending a WORK_TAG back:
            MPI_Iprobe(MPI_ANY_SOURCE, WORK_TAG, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
            if (flag)
            {
                // We do have someone trying to send work back!

                // Allocate memory for the move array structure we want to receive:
                int * move = (int *)malloc(4 * sizeof(int));
                // [0] will be the piece index
                // [1] and [2] will be the to_row and to_col
                // [3] will be the score

                // Receive the completed work:
                MPI_Recv(move, 4, MPI_INT, MPI_ANY_SOURCE, WORK_TAG, MPI_COMM_WORLD, &status);

                // printf("Master received move for piece at index %d\n", move[0]);
                
                // Update the piece_collection with the move data:
                piece_collection[move[0]][2] = move[1];
                piece_collection[move[0]][3] = move[2];
                piece_collection[move[0]][4] = move[3];

                // Free the move array that was allocated
                free(move);

                // Now that we have a new result, let's iterate over the piece_collection
                // and update the best known move so far:
                #pragma omp parallel for
                for (int i = 0; i < num_of_pieces_to_evaluate; i++)
                {
                    if ((piece_collection[i][4] > piece_best_score && rand() % 100 < 80)
                        || (piece_collection[i][4] == piece_best_score && rand() % 2 == 0))
                    {
                        #pragma omp critical
                        {
                            // printf("Master updating best move for piece at index %d\n", i);
                            piece_best_score = piece_collection[i][4];
                            from_row = piece_collection[i][0];
                            from_col = piece_collection[i][1];
                            to_row = piece_collection[i][2];
                            to_col = piece_collection[i][3];
                        }
                    }
                }

                // Increment the work_completed count
                work_completed++;

                // Increment the idle_workers count since this worker is now free for new work!
                idle_workers++;
            }
        }

        // End of the work loop!

        // Step 4: Cleanup and return the best move

        // print the int values of the best move
        // printf("Master best move: %d %d %d %d\n", *from_row, *from_col, *to_row, *to_col);

        // Did we find a valid move?
        if (piece_best_score == -1 || to_row == -1 || to_col == -1)
        {
            printf("AI cannot find a valid move.\n");
            return 1;
        }

        // Free memory for the piece collection
        for (int i = 0; i < num_of_pieces_to_evaluate; i++) {
            free(piece_collection[i]);
        }

        // Free the piece collection
        free(piece_collection);

        // Free the MPI datatype
        MPI_Type_free(&pieceType);

        // Print the move:
        if (show_board)
        {
            if (current_player == RED)
                printf("Parallel AI RED move: %c%d %c%d\n", 'a' + from_row, from_col + 1, 'a' + to_row, to_col + 1);
            else
                printf("Parallel AI BLACK move: %c%d %c%d\n", 'a' + from_row, from_col + 1, 'a' + to_row, to_col + 1);
        }
        
        // Update the board with the move!
        update_board(from_row, from_col, to_row, to_col, current_player);

        return 0;
    }
    else
    {
        printf("This should not be called by any process other than rank=0, calling rank is %d\n", rank);
        return 1;
    }
}

// This is the code that is launched if the process is > rank 0
void parallel_worker()
{
    // Initialize the worker
    int signalBuf;
    int work_signal = 1;
    int terminate_signal = 0;
    int receive_board_signal = 1;
    int receive_work_signal = 2;

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    P_Colour current_player;

    // Create the custom MPI data type for the Piece struct:
    createPieceMPIType(&pieceType);

    // Calculate the size of the board
    int piece_size = sizeof(Piece);
    int board_size = BOARD_SIZE_X * BOARD_SIZE_Y;
    int board_size_bytes = board_size * piece_size;

    // Keep a task count to report how much this worker has done
    // when terminating!
    task_count = 0;

    int from_row, from_col, to_row, to_col;

    while (1)
    {
        // Announce ready to work!
        MPI_Send(&work_signal, 1, MPI_INT, 0, IDLE_TAG, MPI_COMM_WORLD);

        // printf("Worker %d sent idle signal...\n", rank);

        // printf("Worker %d waiting for work signal...\n", rank);
        
        // Now wait for the controller to send a signal on the IDLE_TAG about what
        // to do next:
        MPI_Recv(&signalBuf, 1, MPI_INT, 0, IDLE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // printf("Worker %d received idle signal...%d\n", rank, signalBuf);

        // printf("Worker %d doing work...\n", rank);

        // Let's examine the IDLE_TAG signal to learn what we are asked to do:
        
        
        // Time to terminate?
        if (signalBuf == terminate_signal)
        {
            printf("Process %d terminating... %d tasks completed.\n", rank, task_count);
            break;
        }

        // Receive a game board?
        if (signalBuf == receive_board_signal)
        {   

            // Receive the game board
            MPI_Bcast(board, board_size, pieceType, 0, MPI_COMM_WORLD);
            // printf("Process %d received board\n", rank);
        }

        // Work on a piece?
        if (signalBuf == receive_work_signal)
        {
            // Increment the task counter
            task_count++;

            // Allocate memory for the piece array structure we want to receive:
            int * piece = (int *)malloc(4 * sizeof(int));;

            // printf("Worker %d waiting for piece...\n", rank);

            // Receive work to the worker
            MPI_Recv(piece, 4, MPI_INT, 0, WORK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            // Get the values from the piece array:
            int piece_index = piece[0];
            from_row = piece[1];
            from_col = piece[2];
            current_player = piece[3];

            // Free the piece array that was allocated
            free(piece);

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

            // Now we iterate over the moves and recursively play through a series
            // of moves to the MAX_DEPTH to find the route that leads to best
            // outcome for the current player. The best outcome is defined simply
            // as the best score for this player after MAX_DEPTH moves.

            // The score is determined by iterating over the pieces on the board
            // and summing their values to get a score for the current player that
            // represents their relative strength on the board.
            int moves_left_to_evaluate = MAX_DEPTH;

            int best_to_row = -1;
            int best_to_col = -1;
            int best_score = -1;

            if (errors_detected == 0)
            {
                #pragma omp parallel for collapse(2)
                for (int to_row = 0; to_row < BOARD_SIZE_X; to_row++)
                {
                    for (int to_col = 0; to_col < BOARD_SIZE_Y; to_col++)
                    {
                        // Don't try moving to our own position!
                        if (to_row != from_row && to_col != from_col)
                        {
                            // printf("Worker %d evaluating pince as (%d, %d) -> (%d %d)\n", rank, from_row, from_col, to_row, to_col);
                            
                            if (is_valid_move(from_row, from_col, to_row, to_col, current_player)) {
                                // printf("Worker %d VALID MOVE %d %d %d %d\n", rank, from_row, from_col, to_row, to_col);
                                #pragma omp critical
                                {
                                    // So, evaluate this move
                                    int score = evaluate_move(from_row, from_col, to_row, to_col, current_player, MAX_DEPTH);
                                
                                    // Check if evaluate_move returned a valid score
                                    if (score == -1) {
                                        fprintf(stderr, "Error: Failed to evaluate move.\n");
                                    }
                                
                                    // printf("Worker %d score for move %d %d %d %d is %d\n", rank, from_row, from_col, to_row, to_col, score);
                                
                                    // Check to see if the score from this move is better than what we have seen
                                    // so far, if so... take it. (only 80% of the time, to make the games more random!)
                                    if (score > best_score && rand() % 100 < 80)
                                    {
                                        best_to_row = to_row;
                                        best_to_col = to_col;
                                        best_score = score;
                                        // printf("New best score\n");
                                    } else if (score == best_score)
                                    // If this move generates the same score as the current best
                                    // change to the new move 50% of the time.
                                    {
                                        if (rand() % 2 == 0)
                                        {
                                            // printf("took rand\n");
                                            best_to_row = to_row;
                                            best_to_col = to_col;
                                            best_score = score;
                                        }
                                        else
                                        {
                                            // printf("did not take rand\n");
                                        }
                                    }
                                }
                            }

                            // printf("NOT VALID - Worker %d done evaluating move %d %d %d %d\n", rank, from_row, from_col, i, j);
                        }
                    }
                }
            }

            // printf("Worker %d sending move for piece at index %d\n", rank, move[0]);
            
            // Send the move back to the master

            // Allocate memory for the move array structure we want to send:
            int * move = (int *)malloc(4 * sizeof(int));
            // [0] will be the piece index
            // [1] and [2] will be the to_row and to_col
            // [3] will be the score

            // printf("Worker %d sending move for piece at index %d: %d %d %d\n", rank, piece_index, best_to_row, best_to_col, best_score);
            move[0] = piece_index;
            move[1] = best_to_row;
            move[2] = best_to_col;
            move[3] = best_score;
            // printf("Worker %d sending move for piece at index %d: %d %d %d\n", rank, move[0], move[1], move[2], move[3]);
            MPI_Send(move, 4, MPI_INT, 0, WORK_TAG, MPI_COMM_WORLD);

            // Free the move array that was allocated
            free(move);
        }
    }

    // Free the MPI datatype
    MPI_Type_free(&pieceType);
}