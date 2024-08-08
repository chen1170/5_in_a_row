#include "board.h"

// Function to create a new game board
GameBoard* create_board() {
    int i, j;
    GameBoard* board = (GameBoard*) malloc(sizeof(GameBoard));
    for(i = 0; i < BOARD_SIZE; i++) {
        for(j = 0; j < BOARD_SIZE; j++) {
            board->slots[i][j] = PLAYER_EMPTY;
        }
    }
    board->empty_count = BOARD_SIZE * BOARD_SIZE;

    return board;
}

// Function to place a player symbol on the board
void place_player(GameBoard* board, PlayerType player, GameMove* move) {
    board->slots[move->row][move->col] = player;
    board->empty_count--;
}

// Function to remove a player symbol from the board
void remove_player(GameBoard* board, GameMove* move) {
    board->slots[move->row][move->col] = PLAYER_EMPTY;
    board->empty_count++;
}

// Function to check for a winner
PlayerType check_winner(GameBoard* board) {
    int i, j;
    PlayerType player;
    int equal;

    // Check rows
    for(i = 0; i < BOARD_SIZE; i++) {
        equal = 1;
        player = board->slots[i][0];
        if(player != PLAYER_EMPTY) {
            for(j = 1; j < BOARD_SIZE; j++) {
                if(board->slots[i][j] != player) {
                    equal = 0;
                    break;
                }
            }
            if(equal == 1) {
                return player;
            }
        }
    }

    // Check columns
    for(i = 0; i < BOARD_SIZE; i++) {
        equal = 1;
        player = board->slots[0][i];
        if(player != PLAYER_EMPTY) {
            for(j = 1; j < BOARD_SIZE; j++) {
                if(board->slots[j][i] != player) {
                    equal = 0;
                    break;
                }
            }
            if(equal == 1) {
                return player;
            }
        }
    }

    // Check main diagonal
    equal = 1;
    player = board->slots[0][0];
    if(player != PLAYER_EMPTY) {
        for(i = 1; i < BOARD_SIZE; i++) {
            if(board->slots[i][i] != player) {
                equal = 0;
                break;
            }
        }
        if(equal == 1) {
            return player;
        }
    }

    // Check secondary diagonal
    equal = 1;
    player = board->slots[0][BOARD_SIZE-1];
    if(player != PLAYER_EMPTY) {
        for(i = 1; i < BOARD_SIZE; i++) {
            if(board->slots[i][BOARD_SIZE-i-1] != player) {
                equal = 0;
                break;
            }
        }
        if(equal == 1) {
            return player;
        }
    }

    if(board->empty_count == 0) {
        return PLAYER_NO_WINNER;
    }

    return PLAYER_EMPTY;
}

// Function to print the board
void display_board(GameBoard* board) {
    int i, j;
    for(i = 0; i < BOARD_SIZE; i++) {
        for(j = 0; j < BOARD_SIZE; j++) {
            if(board->slots[i][j] == PLAYER_X) {
                printf("X ");
            } else if(board->slots[i][j] == 0) {
                printf("O ");
            } else {
                printf("- ");
            }
        }
        printf("\n");
    }
}

// Function to generate all possible moves
GameMove** generate_possible_moves(GameBoard* board, int* move_count) {
    int i, j;

    GameMove** list = (GameMove**) malloc(board->empty_count * sizeof(GameMove*));
    *move_count = 0;

    for(i = 0; i < BOARD_SIZE; i++) {
        for(j = 0; j < BOARD_SIZE; j++) {
            if(board->slots[i][j] == PLAYER_EMPTY) {
                list[(*move_count)] = (GameMove*) malloc(sizeof(GameMove));
                list[(*move_count)]->row = i;
                list[(*move_count)]->col = j;
                (*move_count)++;
            }
        }
    }
    return list;
}

// Function to toggle between players
PlayerType toggle_player(PlayerType player) {
    return 1 - player;
}
