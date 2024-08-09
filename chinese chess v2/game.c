#include "game.h"
#include "board.h"
#include "parallel.h"
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

static GameMode current_mode;
static P_Colour current_player;

void init_game(GameMode mode) {
    current_mode = mode;
    current_player = RED;
    init_board();
    //if (mode == PARALLEL_AI || mode == HUMAN_VS_PARALLEL_AI) {
        //init_parallel_env();
    //}
    //srand(time(NULL));
}

static void switch_player() {
    current_player = (current_player == RED) ? BLACK : RED;
}

static int get_ai_move_original() {
    // 简单AI：随机选择一个合法位置
    int from_row, from_col, to_row, to_col;
    int attempts = 0;
    do {
        from_row = rand() % BOARD_SIZE_X;
        from_col = rand() % BOARD_SIZE_Y;
        to_row = rand() % BOARD_SIZE_X;
        to_col = rand() % BOARD_SIZE_Y;
        attempts++;

    }
    while (attempts < 10000 && !is_valid_move(from_row, from_col, to_row, to_col, current_player));

    if (attempts == 10000) {
        printf("AI cannot find a valid move.\n");
        return 1;
    }

    //printf("AI move: %c%d %c%d\n", 'a' + from_row, from_col + 1, 'a' + to_row, to_col + 1);

    update_board(from_row, from_col, to_row, to_col, current_player);
    return 0;
}

static int get_ai_move() {

    int from_row, from_col, to_row, to_col;

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

    // Index all of the pieces for this player.
    int current_piece_index = 0;
    int piece_collection[num_of_pieces_to_evaluate][5];
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
    int best_from_row = piece_collection[0][0];
    int best_from_col = piece_collection[0][1];
    int best_to_row = piece_collection[0][2];
    int best_to_col = piece_collection[0][3];

    // printf("Rank(%d) piece_count: %d\n", rank, piece_count);

    // Now wait for workers to report they are available to work

    int piece_index_count = 0;
    int work_started = 0;
    int work_completed = 0;
    
    while (piece_index_count < current_piece_index)
    {
        //printf("***** Here... *****\n");
        int piece[4];
        piece[0] = piece_index_count;                 // The index of of the piece in the piece_index array
        piece[1] = piece_collection[piece_index_count][0]; // The row of the piece on the board
        piece[2] = piece_collection[piece_index_count][1]; // The col of the piece on the board
        piece[3] = current_player;                    // The number of pieces for this player on the board
        //printf("***** Here... *****\n");
        piece_index_count++;

        int piece_index = piece[0];
        from_row = piece[1];
        from_col = piece[2];
        to_row = -1;
        to_col = -1;
        current_player = piece[3];

        int move[4];
        move[0] = -2;
        move[1] = -2;
        move[2] = -2;
        move[3] = -2;
        // [0] will be the piece index
        // [1] and [2] will be the to_row and to_col
        // [3] will be the score

        for (int to_row = 0; to_row < BOARD_SIZE_X; to_row++)
        {
            for (int to_col = 0; to_col < BOARD_SIZE_Y; to_col++)
            {
                // Don't try moving to our own position!
                if (to_row != from_row && to_col != from_col)
                {
                    if (is_valid_move(from_row, from_col, to_row, to_col, current_player)) {
                        // So, evaluate this move
                        int score = evaluate_move(from_row, from_col, to_row, to_col, current_player, MAX_DEPTH);
                    
                        // Check if evaluate_move returned a valid score
                        if (score == -1) {
                            fprintf(stderr, "Error: Failed to evaluate move.\n");
                            return 1;
                        }
                 
                        // This is the only move we need to pass back to the master
                        if (score > move[3]) {
                            move[1] = to_row;
                            move[2] = to_col;
                            move[3] = score;
                            //printf("New best score\n");
                        }

                        // If this move generates the same score as the current best
                        // change to the new move 50% of the time.
                        if (score == move[3]) {
                            if (rand() % 3 == 0)
                            {
                                //printf("took rand\n");
                                move[1] = to_row;
                                move[2] = to_col;
                                move[3] = score;
                            }
                            else
                            {
                                //printf("did not take rand\n");
                            }
                        }
                    }
                }
            }
        }        

        //  Update piece_index with the best move found for that piece
        piece_collection[move[0]][2] = move[1];
        piece_collection[move[0]][3] = move[2];
        piece_collection[move[0]][4] = move[3];
    }

    // Update best score and move
    int best_score = -1;
    for (int i = 0; i < current_piece_index; i++)
    {
        if (piece_collection[i][4] > best_score)
        {
            best_score = piece_collection[i][4];
            from_row = piece_collection[i][0];
            from_col = piece_collection[i][1];
            to_row = piece_collection[i][2];
            to_col = piece_collection[i][3];
        }
    }

    if (best_score == -1 || best_to_row == -1 || best_to_col == -1)
    {
        printf("AI cannot find a valid move.\n");
        return 1;
    }

    else
    {   
        update_board(from_row, from_col, to_row, to_col, current_player);
        return 0;
    }
}

static int get_parallel_ai_move() {
    int from_row;
    int from_col;
    int to_row;
    int to_col;
    
    //printf("Current player is %d\n", current_player);

    Piece * board_pointer = board[0];
    //printf("Parallel AI move...\n");
    int p = get_best_move_parallel(&from_row, &from_col, &to_row, &to_col, board_pointer, current_player);
    if (p) {
        printf("Parallel AI cannot find a valid move.\n");
        return p;
    }

    if (current_player == RED)
        printf("Parallel AI RED move: %c%d %c%d\n", 'a' + from_row, from_col + 1, 'a' + to_row, to_col + 1);
    else
        printf("Parallel AI BLACK move: %c%d %c%d\n", 'a' + from_row, from_col + 1, 'a' + to_row, to_col + 1);

    update_board(from_row, from_col, to_row, to_col, current_player);

    return 0;
}

static void get_human_move() {
    char move_from[10];
    char move_to[10];
    //printf("Current player is %d\n", current_player);
    do {
        printf("Enter your move (e.g., e6 e8): \n");
        scanf("%s", move_from);
        scanf("%s", move_to);
    } while (!parse_move(move_from, move_to, current_player));    
}

void run_game(int show_board, double *serial_time, double *parallel_time) {
    int game_over = 0;
    int draw = 0;
    int total_moves = 0;

    while (!game_over) {
        if (show_board)
            print_board();

        double start_time, end_time;

        switch (current_mode) {
            case HUMAN_VS_AI:
                if (current_player == RED) {
                    start_time = MPI_Wtime();
                    get_human_move();  // serial
                    end_time = MPI_Wtime();
                    *serial_time += (end_time - start_time);
                } else {
                    start_time = MPI_Wtime();
                    draw = get_ai_move();  // serial
                    end_time = MPI_Wtime();
                    *serial_time += (end_time - start_time);
                }
                break;

            case AI_VS_AI:
                start_time = MPI_Wtime();
                draw = get_ai_move();  // serial
                end_time = MPI_Wtime();
                *serial_time += (end_time - start_time);
                break;

            case PARALLEL_AI:
                start_time = MPI_Wtime();
                draw = get_parallel_ai_move();  // parallel
                end_time = MPI_Wtime();
                *parallel_time += (end_time - start_time);
                break;

            case HUMAN_VS_PARALLEL_AI:
                if (current_player == RED) {
                    start_time = MPI_Wtime();
                    get_human_move();  // serial
                    end_time = MPI_Wtime();
                    *serial_time += (end_time - start_time);
                } else {
                    start_time = MPI_Wtime();
                    draw = get_parallel_ai_move();  // parallel
                    end_time = MPI_Wtime();
                    *parallel_time += (end_time - start_time);
                }
                break;
        }

        total_moves++;

        //update_board(row, col, current_player);

        if (draw) {
            printf("Draw!\n");
            break;
        }

        if (total_moves >= MAX_MOVES_PER_GAME) {
            printf("Game over: max moves limit reached.. Draw!\n");
            break;
        }   
        
        game_over = check_win();

        if (game_over) {
            print_board();
            if (game_over == 1)
                printf("BLACK wins!\n");
            else
                printf("RED wins!\n");
        } else {
            switch_player();
        }
    }
}
