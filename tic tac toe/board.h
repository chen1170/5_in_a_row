#ifndef __BOARD_DEF
#define __BOARD_DEF

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PLAYER_X 1
#define PLAYER_EMPTY 10
#define PLAYER_NO_WINNER 20

#define BOARD_SIZE 4 // Define the size of the board

typedef unsigned char PlayerType; // Type to represent players on the board

typedef struct {
    PlayerType slots[BOARD_SIZE][BOARD_SIZE]; // Matrix to hold the board's state
    unsigned short empty_count; // Number of empty positions on the board
} GameBoard;

typedef struct {
    unsigned short row, col; // Indices for a move on the board
} GameMove;

// Function prototypes
GameBoard* create_board();
void place_player(GameBoard*, PlayerType, GameMove*);
void remove_player(GameBoard*, GameMove*);
PlayerType check_winner(GameBoard*);
void display_board(GameBoard*);
GameMove** generate_possible_moves(GameBoard*, int*);
PlayerType toggle_player(PlayerType);

#endif // __BOARD_DEF
