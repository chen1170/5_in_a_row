#include "game.h"
#include "board.h"
#include "parallel.h"
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

static GameMode current_mode;
static P_Colour current_player;

void init_game(GameMode mode)
{
    // Setup a new game
    current_mode = mode;
    current_player = RED;

    // Clear and setup the board
    init_board();
    
    // if (mode == PARALLEL_AI || mode == HUMAN_VS_PARALLEL_AI) {
    // init_parallel_env();
    //}
    // srand(time(NULL));
}

static void switch_player()
{
    // Change to the opposing player
    current_player = (current_player == RED) ? BLACK : RED;
}

static int get_ai_move_original()
{
    // generates a random move (a valid one!)
    int from_row, from_col, to_row, to_col;
    int attempts = 0;
    do
    {
        from_row = rand() % BOARD_SIZE_X;
        from_col = rand() % BOARD_SIZE_Y;
        to_row = rand() % BOARD_SIZE_X;
        to_col = rand() % BOARD_SIZE_Y;
        attempts++;

    } while (attempts < 10000 && !is_valid_move(from_row, from_col, to_row, to_col, current_player));

    if (attempts == 10000)
    {
        printf("AI cannot find a valid move.\n");
        return 1;
    }

    // printf("AI move: %c%d %c%d\n", 'a' + from_row, from_col + 1, 'a' + to_row, to_col + 1);

    update_board(from_row, from_col, to_row, to_col, current_player);
    return 0;
}

static int get_ai_move(int show_board)
{
    // Tries to generate a move that is the best move for the current player
    int from_row, from_col, to_row, to_col;

    // How many pieces for this player on the board?
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

    // Now that we know how many pieces we have to evaluate, we can
    // index all of the pieces for this player in a collection:
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
                // The piece at this position belongs to the current player
                piece_collection[current_piece_index][0] = row;
                piece_collection[current_piece_index][1] = col;
                piece_collection[current_piece_index][2] = -1;
                piece_collection[current_piece_index][3] = -1;
                piece_collection[current_piece_index][4] = -1;
                current_piece_index++;

                // printf("Piece %d: %d %d\n", current_piece_index, row, col);
            }
        }
    }

    // Check if current_piece_index is correct, meaning we found all the pieces
    if (current_piece_index != num_of_pieces_to_evaluate)
    {
        printf("Piece count mismatch.. ??\n");
        return 1;
    }

    // Set defaults for the final values
    int best_from_row = piece_collection[0][0];
    int best_from_col = piece_collection[0][1];
    int best_to_row = piece_collection[0][2];
    int best_to_col = piece_collection[0][3];
    int best_score = -1;

    // printf("Rank(%d) piece_count: %d\n", rank, piece_count);

    // Now, we will iterate ove the pieces looking for the move that is best
    // for the current player.
    int piece_index_count = 0;
    int work_started = 0;
    int work_completed = 0;

    // Loop until all pieces have been evaluated
    while (piece_index_count < num_of_pieces_to_evaluate)
    {
        from_row = piece_collection[piece_index_count][0];
        from_col = piece_collection[piece_index_count][1];
        int best_score = -1;

        // Now we iterate over the pieces and recursively play through a series
        // of moves to the MAX_DEPTH to find the route that leads to best
        // outcome for the current player. The best outcome is defined simply
        // as the best score for this player after MAX_DEPTH moves.

        // The score is determined by iterating over the pieces on the board
        // and summing their values to get a score for the current player that
        // represents their relative strength on the board.

        for (int to_row = 0; to_row < BOARD_SIZE_X; to_row++)
        {
            for (int to_col = 0; to_col < BOARD_SIZE_Y; to_col++)
            {
                // Don't try moving to our own position!
                if (to_row != from_row && to_col != from_col)
                {
                    if (is_valid_move(from_row, from_col, to_row, to_col, current_player))
                    {
                        // printf("Valid move!!!\n");
                        // So, evaluate this move
                        int score = evaluate_move(from_row, from_col, to_row, to_col, current_player, MAX_DEPTH);

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
            }
        }

        // Update piece_index with the best move found for that piece
        piece_collection[piece_index_count][2] = best_to_row;
        piece_collection[piece_index_count][3] = best_to_col;
        piece_collection[piece_index_count][4] = best_score;

        piece_index_count++;
    }

    // All pieces have explored their moves to MAX_DEPTH (or less)
    // and they have stored the move that leads to the best (maybe!) score
    // in MAX_DEPTH moves, if the game follows that tree...

    // Find the piece that yielded the best move:
    int piece_best_score = -1;
    for (int i = 0; i < num_of_pieces_to_evaluate; i++)
    {
        // printf("Best score is: %d\n", piece_collection[i][4]);
        if ((piece_collection[i][4] > piece_best_score && rand() % 100 < 80)
        || (piece_collection[i][4] == piece_best_score && rand() % 2 == 0))
        {
            piece_best_score = piece_collection[i][4];
            from_row = piece_collection[i][0];
            from_col = piece_collection[i][1];
            to_row = piece_collection[i][2];
            to_col = piece_collection[i][3];
        }
    }

    if (piece_best_score == -1 || to_row == -1 || to_col == -1)
    {
        // No piece found a valid move!
        printf("AI cannot find a valid move.\n");
        return 1;
    }

    else
    {
        // Show the move:
        if (show_board)
        {  
            if (current_player == RED)
                printf("AI RED move: %c%d %c%d\n", 'a' + from_row, from_col + 1, 'a' + to_row, to_col + 1);
            else
                printf("AI BLACK move: %c%d %c%d\n", 'a' + from_row, from_col + 1, 'a' + to_row, to_col + 1);
            // printf("AI move: %c%d %c%d\n", 'a' + from_row, from_col + 1, 'a' + to_row, to_col + 1);
        }
        // Finally make the move by updating the board!
        update_board(from_row, from_col, to_row, to_col, current_player);
        
        return 0;
    }
}

static int get_parallel_ai_move(int show_board)
{
    int from_row = 0;
    int from_col = 0;
    int to_row = 0;
    int to_col = 0;

    // printf("Current player is %d\n", current_player);

    Piece *board_pointer = board[0];
    // printf("Parallel AI move...\n");
    int p = get_best_parallel(show_board, current_player);
    if (p)
    {
        printf("Parallel AI cannot find a valid move.\n");
        return p;
    }

    return 0;
}

static void get_human_move()
{
    // Get a move from the human player
    char move_from[10];
    char move_to[10];
    // printf("Current player is %d\n", current_player);
    do
    {
        printf("Enter your move (e.g., e6 e8): ");
        scanf("%s", move_from);
        scanf("%s", move_to);
    } while (!parse_move(move_from, move_to, current_player));
}

void run_game(int show_board, double *serial_time, double *parallel_time)
{
    // We need some comments in here...
    
    int game_over = 0;
    int draw = 0;
    int total_moves = 0;

    while (!game_over)
    {
        if (show_board)
            print_board();

        double start_time, end_time;

        switch (current_mode)
        {
        case HUMAN_VS_AI:
            if (current_player == RED)
            {
                start_time = MPI_Wtime();
                get_human_move(); // serial
                end_time = MPI_Wtime();
                *serial_time += (end_time - start_time);
            }
            else
            {
                start_time = MPI_Wtime();
                draw = get_ai_move(show_board); // serial
                end_time = MPI_Wtime();
                *serial_time += (end_time - start_time);
            }
            break;

        case AI_VS_AI:
            start_time = MPI_Wtime();
            draw = get_ai_move(show_board); // serial
            end_time = MPI_Wtime();
            *serial_time += (end_time - start_time);
            break;

        case PARALLEL_AI:
            start_time = MPI_Wtime();
            draw = get_parallel_ai_move(show_board); // parallel
            end_time = MPI_Wtime();
            *parallel_time += (end_time - start_time);
            break;

        case HUMAN_VS_PARALLEL_AI:
            if (current_player == RED)
            {
                start_time = MPI_Wtime();
                get_human_move(); // serial
                end_time = MPI_Wtime();
                *serial_time += (end_time - start_time);
            }
            else
            {
                start_time = MPI_Wtime();
                draw = get_parallel_ai_move(show_board); // parallel
                end_time = MPI_Wtime();
                *parallel_time += (end_time - start_time);
            }
            break;
        }

        total_moves++;

        // -- printf("Moves = %d\n", total_moves);

        if (draw)
        {
            printf("Draw!\n");
            break;
        }

        if (total_moves >= MAX_MOVES_PER_GAME)
        {
            printf("Game over: max moves limit reached.. Draw!\n");
            break;
        }

        game_over = check_win();

        if (game_over)
        {
            // print_board();
            if (game_over == 1)
                printf("BLACK wins!\n");
            else
                printf("RED wins!\n");
        }
        else
        {
            switch_player();
        }
    }
}
