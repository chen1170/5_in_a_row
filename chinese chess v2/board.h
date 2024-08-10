#ifndef BOARD_H
#define BOARD_H

#define BOARD_SIZE_X 10
#define BOARD_SIZE_Y 9

#define MAX_MOVES_PER_GAME 50
#define MAX_SIMULATED_WORK 100000
#define MAX_DEPTH 24
#define MAX_PLAYER RED
#define MIN_PLAYER BLACK

typedef enum {
    EMPTY = 0,
    RED = 1,
    BLACK = 2,
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

static int jiang_value[10][9] = {
    {0, 0, 0, -5, 10, -5, 0, 0, 0},
    {0, 0, 0, -15, -10, -15, 0, 0, 0},
    {0, 0, 0, -20, -20, -20, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static int shi_value[10][9] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 2, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static int xiang_value[10][9] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 4, 0},
    {-1, 0, 0, 0, 2, 0, 0, 0, -1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, -1, 0, 0, 0, -1, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static int ma_value[10][9] = {
    {0, -1, 0, 0, 0, 0, 0, -1, 0},
    {0, 0, 0, 0, -3, 0, 0, 0, 0},
    {0, 0, 1, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0},
    {0, 1, 1, 1, 0, 1, 1, 1, 0},
    {0, 0, 1, 0, 1, 0, 1, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 2, 2, 1, 2, 2, 0, 0},
    {0, 1, 2, 0, 0, 0, 2, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static int ju_value[10][9] = {
    {-3, 1, 0, 2, 0, 2, 0, 1, -3},
    {1, 1, 0, 2, 0, 2, 0, 1, 1},
    {0, 0, 0, 3, 0, 3, 0, 0, 0},
    {1, 1, 1, 3, 3, 3, 1, 1, 1},
    {0, 2, 2, 3, 3, 3, 2, 2, 0},
    {0, 2, 0, 3, 3, 3, 0, 2, 0},
    {2, 2, 2, 3, 3, 3, 2, 2, 2},
    {0, 1, 2, 3, 3, 3, 2, 1, 0},
    {1, 1, 2, 5, 3, 5, 2, 1, 1},
    {1, 1, 1, 3, 3, 3, 1, 1, 1}
};

static int pao_value[10][9] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, -1, 1, 2, 5, 2, 1, -1, 1},
    {0, 0, 0, 0, 5, 0, 0, 0, 0},
    {0, 0, 0, 0, 5, 0, 0, 0, 0},
    {0, 0, 0, 0, 5, 0, 0, 0, 0},
    {0, 1, 0, 0, 5, 0, 0, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 1, 0, 1, 0, 1, 0},
    {2, 2, 0, 0, 0, 0, 0, 2, 2}
};

static int bing_value[10][9] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {-1, 0,-1, 0, 0, 0, -1, 0, -1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 2, 2, 2, 2, 2, 1, 1},
    {1, 1, 3, 5, 5, 5, 3, 1, 1},
    {1, 1, 2, 8, 10, 8, 2, 1, 1},
    {-2, -1, 1, 1, 1, 1, 1, -1, -2}
};

static int jiang_value_black[10][9] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, -20, -20, -20, 0, 0, 0},
    {0, 0, 0, -15, -10, -15, 0, 0, 0},
    {0, 0, 0, -5, 10, -5, 0, 0, 0}
};

static int shi_value_black[10][9] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 2, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static int xiang_value_black[10][9] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, -1, 0, 0, 0, -1, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {-1, 0, 0, 0, 2, 0, 0, 0, -1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static int ma_value_black[10][9] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 2, 0, 0, 0, 2, 1, 0},
    {0, 0, 2, 2, 1, 2, 2, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 1, 0, 1, 0, 1, 0, 0},
    {0, 0, 1, 0, 1, 0, 1, 0, 0},
    {0, 1, 1, 1, 0, 1, 1, 1, 0},
    {0, 0, 1, 0, 1, 0, 1, 0, 0},
    {0, 0, 0, 0, -3, 0, 0, 0, 0},
    {0, -1, 0, 0, 0, 0, 0, -1, 0}
};

static int ju_value_black[10][9] = {
    {1, 1, 1, 3, 3, 3, 1, 1, 1},
    {1, 1, 2, 5, 3, 5, 2, 1, 1},
    {0, 1, 2, 3, 3, 3, 2, 1, 0},
    {2, 2, 2, 3, 3, 3, 2, 2, 2},
    {0, 2, 0, 3, 3, 3, 0, 2, 0},
    {0, 2, 2, 3, 3, 3, 2, 2, 0},
    {1, 1, 1, 3, 3, 3, 1, 1, 1},
    {0, 0, 0, 3, 0, 3, 0, 0, 0},
    {1, 1, 0, 2, 0, 2, 0, 1, 1},
    {-3, 1, 0, 2, 0, 2, 0, 1, -3}
};

static int pao_value_black[10][9] = {
    {2, 2, 0, 0, 0, 0, 0, 2, 2},
    {0, 1, 0, 1, 0, 1, 0, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 5, 0, 0, 1, 0},
    {0, 0, 0, 0, 5, 0, 0, 0, 0},
    {0, 0, 0, 0, 5, 0, 0, 0, 0},
    {0, 0, 0, 0, 5, 0, 0, 0, 0},
    {1, -1, 1, 2, 5, 2, 1, -1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static int bing_value_black[10][9] = {
    {-2, -1, 1, 1, 1, 1, 1, -1, -2},
    {1, 1, 2, 8, 10, 8, 2, 1, 1},
    {1, 1, 3, 5, 5, 5, 3, 1, 1},
    {1, 1, 2, 2, 2, 2, 2, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {-1, 0,-1, 0, 0, 0, -1, 0, -1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0}
};

void init_board();
void print_board();
int is_valid_move(int from_row, int from_col, int to_row, int to_col, P_Colour player);
void update_board(int from_row, int from_col, int to_row, int to_col, P_Colour player);
Piece get_piece(int row, int col);
int check_win();
int parse_move(const char* from, const char* to, P_Colour player);
int evaluate_move(int from_row, int from_col, int to_row, int to_col, P_Colour player, int depth);
static int evaluate_board(P_Colour player);

#endif // BOARD_H