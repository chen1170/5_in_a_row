#ifndef BOARD_H
#define BOARD_H

#define BOARD_SIZE 15

typedef enum {
    EMPTY = 0,
    BLACK = 1,
    WHITE = 2,
    INVALID = 3
} PieceType;

void init_board();
void print_board();
int is_valid_move(int row, int col);
void update_board(int row, int col, PieceType piece);
PieceType get_piece(int row, int col);
int check_win(int row, int col, PieceType piece);
int parse_move(const char* move, int* row, int* col);
int is_board_full();

#endif // BOARD_H