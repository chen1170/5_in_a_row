#include "board.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static PieceType board[BOARD_SIZE][BOARD_SIZE];

void init_board() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = EMPTY;
        }
    }
}

void print_board() {
    printf("  a b c d e f g h i j k l m n o\n");
    for (int i = 0; i < BOARD_SIZE; i++) {
        printf("%2d ", i + 1);
        for (int j = 0; j < BOARD_SIZE; j++) {
            char piece = '.';
            if (board[i][j] == BLACK) piece = 'X';
            if (board[i][j] == WHITE) piece = 'O';
            printf("%c ", piece);
        }
        printf("\n");
    }
}

int is_valid_move(int row, int col) {
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && board[row][col] == EMPTY;
}

void update_board(int row, int col, PieceType piece) {
    if (is_valid_move(row, col)) {
        board[row][col] = piece;
    }
}

PieceType get_piece(int row, int col) {
    if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE) {
        return board[row][col];
    }
    return INVALID;
}

int check_win(int row, int col, PieceType piece) {
    const int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

    for (int d = 0; d < 4; d++) {
        int count = 1;
        for (int i = 1; i <= 4; i++) {
            int r = row + i * directions[d][0];
            int c = col + i * directions[d][1];
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE || board[r][c] != piece) break;
            count++;
        }
        for (int i = 1; i <= 4; i++) {
            int r = row - i * directions[d][0];
            int c = col - i * directions[d][1];
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE || board[r][c] != piece) break;
            count++;
        }
        if (count >= 5) return 1;
    }
    return 0;
}

int parse_move(const char* move, int* row, int* col) {
    if (strlen(move) < 2 || strlen(move) > 3) return 0;

    *col = move[0] - 'a';
    *row = atoi(move + 1) - 1;

    return is_valid_move(*row, *col);
}

int is_board_full() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == EMPTY) {
                return 0;
            }
        }
    }
    return 1;
}