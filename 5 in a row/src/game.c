#include "game.h"
#include "parallel.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static GameMode current_mode;
static PieceType current_player;

void init_game(GameMode mode) {
    current_mode = mode;
    current_player = BLACK;
    init_board();
    //if (mode == PARALLEL_AI || mode == HUMAN_VS_PARALLEL_AI) {
        init_parallel_env();
    //}
    srand(time(NULL));
}

static void switch_player() {
    current_player = (current_player == BLACK) ? WHITE : BLACK;
}

static void get_ai_move(int* row, int* col) {
    // 简单AI：随机选择一个合法位置
    do {
        *row = rand() % BOARD_SIZE;
        *col = rand() % BOARD_SIZE;
    } while (!is_valid_move(*row, *col));
}

static void get_parallel_ai_move(int* row, int* col) {
    get_best_move_parallel(row, col, current_player);
}

static void get_human_move(int* row, int* col) {
    char move[10];
    do {
        printf("Enter your move (e.g., e8): \n");
        scanf("%s", move);
    } while (!parse_move(move, row, col));
}

void run_game() {
    int row, col;
    int game_over = 0;

    while (!game_over) {
        print_board();

        switch (current_mode) {
            case HUMAN_VS_AI:
                if (current_player == BLACK) {
                    get_human_move(&row, &col);
                } else {
                    get_ai_move(&row, &col);
                    printf("AI move: %c%d\n", 'a' + col, row + 1);
                }
                break;
            case AI_VS_AI:
                get_ai_move(&row, &col);
                printf("AI move: %c%d\n", 'a' + col, row + 1);
                break;
            case PARALLEL_AI:
                get_parallel_ai_move(&row, &col);
                printf("Parallel AI move: %c%d\n", 'a' + col, row + 1);
                break;
            case HUMAN_VS_PARALLEL_AI:
                if (current_player == BLACK) {
                    get_human_move(&row, &col);
                } else {
                    get_parallel_ai_move(&row, &col);
                    printf("Parallel AI move: %c%d\n", 'a' + col, row + 1);
                }
                break;
        }

        update_board(row, col, current_player);

        if (check_win(row, col, current_player)) {
            print_board();
            printf("Player %c wins!\n", (current_player == BLACK) ? 'X' : 'O');
            game_over = 1;
        } else if (is_board_full()) {
            print_board();
            printf("The game is a draw!\n");
            game_over = 1;
        } else {
            switch_player();
        }
    }

    //if (current_mode == PARALLEL_AI || current_mode == HUMAN_VS_PARALLEL_AI) {
        cleanup_parallel_env();
    //}
}