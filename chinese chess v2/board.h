#ifndef BOARD_H
#define BOARD_H

#define BOARD_SIZE_X 10
#define BOARD_SIZE_Y 9

typedef enum {
    EMPTY = 0,
    BLACK = 1,
    WHITE = 2,
    INVALID = 3
} P_Colour;

// Chinese chess pieces
typedef enum {
    None = 0,
    JU = 1,     // Chariot / Rook
    MA = 2,     // Horse / Knight
    XIANG = 3,  // Elephant / Bishop
    SHI = 4,    // Advisor / Guard
    JIANG = 5,  // General / King
    PAO = 6,    // Cannon
    BING = 7    // Soldier / Pawn
} P_Type;

typedef enum {
    N = 0,
    R = 1,     // Chariot / Rook
    H = 2,     // Horse / Knight
    E = 3,  // Elephant / Bishop
    A = 4,    // Advisor / Guard
    G = 5,  // General / King
    C = 6,    // Cannon
    S = 7    // Soldier / Pawn
} P_TypeCode;

typedef struct {
    P_Type type;
    P_Colour colour;
    P_TypeCode code;
    char board_code;
} Piece;

extern Piece board[BOARD_SIZE_X][BOARD_SIZE_Y];
extern Piece empty;

void init_board();
void print_board();
int is_valid_move(int from_row, int from_col, int to_row, int to_col, P_Colour player);
void update_board(int from_row, int from_col, int to_row, int to_col, P_Colour player);
Piece get_piece(int row, int col);
int check_win();
int parse_move(const char* from, const char* to, P_Colour player);
int is_board_full();

#endif // BOARD_H