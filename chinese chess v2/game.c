#include "game.h"
#include "board.h"
#include "parallel.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static GameMode current_mode;
static P_Colour current_player;

void init_game(GameMode mode) {
    current_mode = mode;
    current_player = BLACK;
    init_board();
    //if (mode == PARALLEL_AI || mode == HUMAN_VS_PARALLEL_AI) {
        //init_parallel_env();
    //}
    srand(time(NULL));
}

static void switch_player() {
    current_player = (current_player == BLACK) ? WHITE : BLACK;
}

static int get_ai_move() {
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

    printf("AI move: %c%d %c%d\n", 'a' + from_row, from_col + 1, 'a' + to_row, to_col + 1);

    update_board(from_row, from_col, to_row, to_col, current_player);
    return 0;    
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
    if (current_player == BLACK)
        printf("Parallel AI BLACK move: %c%d %c%d\n", 'a' + from_row, from_col + 1, 'a' + to_row, to_col + 1);
    else
        printf("Parallel AI WHITE move: %c%d %c%d\n", 'a' + from_row, from_col + 1, 'a' + to_row, to_col + 1);

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

void run_game() {
    int game_over = 0;
    int draw = 0;

    while (!game_over) {
        print_board();

        switch (current_mode) {
            case HUMAN_VS_AI:
                if (current_player == BLACK) {
                    get_human_move();
                } else {
                    draw = get_ai_move();
                    //printf("AI move: %c%d\n", 'a' + col, row + 1);
                }
                break;
            case AI_VS_AI:
                draw = get_ai_move();
                //printf("AI move: %c%d\n", 'a' + col, row + 1);
                break;
            case PARALLEL_AI:
                draw = get_parallel_ai_move();
                //printf("Parallel AI move: %c%d\n", 'a' + col, row + 1);
                break;
            case HUMAN_VS_PARALLEL_AI:
                if (current_player == BLACK) {
                    get_human_move();
                } else {
                    draw = get_parallel_ai_move();
                    //printf("Parallel AI move: %c%d\n", 'a' + col, row + 1);
                }
                break;
        }

        //update_board(row, col, current_player);

        if (draw) {
            printf("Draw!\n");
            break;
        }
        
        game_over = check_win();

        if (game_over) {
            print_board();
            if (game_over == 1)
                printf("White wins!\n");
            else
                printf("Black wins!\n");
        }
        else
        {
            //printf("Switching players...\n");
            switch_player();
        }
    }

    //if (current_mode == PARALLEL_AI || current_mode == HUMAN_VS_PARALLEL_AI) {
        cleanup_parallel_env();
    //}
}