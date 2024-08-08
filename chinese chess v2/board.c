#include "board.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Piece board[BOARD_SIZE_X][BOARD_SIZE_Y];
Piece empty;

//Piece board[BOARD_SIZE_X][BOARD_SIZE_Y];
//Piece empty;

// Setup a board for Chinese Chess
void init_board()
{
    for (int row = 0; row < BOARD_SIZE_X; row++)
    {
        for (int col = 0; col < BOARD_SIZE_Y; col++)
        {
            // Place the correct pieces on the board for each i, j position
            // by creating a Piece struct and assigning it to the board

            // Init with an empty piece
            //Piece empty;
            empty.type = None;
            empty.colour = EMPTY;
            empty.code = N;
            empty.board_code = '.';
            board[row][col] = empty;

            // Black pieces

            // Chariots
            if (row == 0 && (col == 0 || col == 8))
            {
                Piece ju;
                ju.type = JU;
                ju.colour = BLACK;
                ju.code = R;
                ju.board_code = 'R';
                board[row][col] = ju;
            }

            // Horses
            if (row == 0 && (col == 1 || col == 7))
            {
                Piece ma;
                ma.type = MA;
                ma.colour = BLACK;
                ma.code = H;
                ma.board_code = 'H';
                board[row][col] = ma;
            }

            // Elephants
            if (row == 0 && (col == 2 || col == 6))
            {
                Piece xiang;
                xiang.type = XIANG;
                xiang.colour = BLACK;
                xiang.code = E;
                xiang.board_code = 'E';
                board[row][col] = xiang;
            }

            // Advisors
            if (row == 0 && (col == 3 || col == 5))
            {
                Piece shi;
                shi.type = SHI;
                shi.colour = BLACK;
                shi.code = A;
                shi.board_code = 'A';
                board[row][col] = shi;
            }

            // General
            if (row == 0 && col == 4)
            {
                Piece jiang;
                jiang.type = JIANG;
                jiang.colour = BLACK;
                jiang.code = G;
                jiang.board_code = 'G';
                board[row][col] = jiang;
            }

            // Cannons
            if (row == 2 && (col == 1 || col == 7))
            {
                Piece pao;
                pao.type = PAO;
                pao.colour = BLACK;
                pao.code = C;
                pao.board_code = 'C';
                board[row][col] = pao;
            }

            // Soldiers
            if (row == 3 && (col % 2 == 0))
            {
                Piece bing;
                bing.type = BING;
                bing.colour = BLACK;
                bing.code = S;
                bing.board_code = 'S';
                board[row][col] = bing;
            }

            // White pieces

            // Chariots
            if (row == 9 && (col == 0 || col == 8))
            {
                Piece ju;
                ju.type = JU;
                ju.colour = WHITE;
                ju.code = R;
                ju.board_code = 'r';
                board[row][col] = ju;
            }

            // Horses
            if (row == 9 && (col == 1 || col == 7))
            {
                Piece ma;
                ma.type = MA;
                ma.colour = WHITE;
                ma.code = H;
                ma.board_code = 'h';
                board[row][col] = ma;
            }

            // Elephants
            if (row == 9 && (col == 2 || col == 6))
            {
                Piece xiang;
                xiang.type = XIANG;
                xiang.colour = WHITE;
                xiang.code = E;
                xiang.board_code = 'e';
                board[row][col] = xiang;
            }

            // Advisors
            if (row == 9 && (col == 3 || col == 5))
            {
                Piece shi;
                shi.type = SHI;
                shi.colour = WHITE;
                shi.code = A;
                shi.board_code = 'a';
                board[row][col] = shi;
            }

            // General
            if (row == 9 && col == 4)
            {
                Piece jiang;
                jiang.type = JIANG;
                jiang.colour = WHITE;
                jiang.code = G;
                jiang.board_code = 'g';
                board[row][col] = jiang;
            }

            // Cannons
            if (row == 7 && (col == 1 || col == 7))
            {
                Piece pao;
                pao.type = PAO;
                pao.colour = WHITE;
                pao.code = C;
                pao.board_code = 'c';
                board[row][col] = pao;
            }

            // Soldiers
            if (row == 6 && (col % 2 == 0))
            {
                Piece bing;
                bing.type = BING;
                bing.colour = WHITE;
                bing.code = S;
                bing.board_code = 's';
                board[row][col] = bing;
            }
        }
    }
}

void print_board()
{
    printf("\n");
    printf("   1 2 3 4 5 6 7 8 9\n");
    for (int row = 0; row < BOARD_SIZE_X; row++)
    {
        printf("%2c ",'a' + row);
        for (int col = 0; col < BOARD_SIZE_Y; col++)
        {
            char piece = board[row][col].board_code;
            printf("%c ", piece);
        }
        printf("\n");
    }
    printf("\n");
}

int is_valid_move_for_general(int from_row, int from_col, int to_row, int to_col)
{
    // Generals must stay within the palace
    // First, we check that the black general is staying within the palace
    if (board[from_row][from_col].colour == WHITE)
    {
        if (to_row < 7 || to_col < 3 || to_col > 5)
        {
            return 0;
        }
    }

    // Next, we check that the white general is staying within the palace
    if (board[from_row][from_col].colour == BLACK)
    {
        if (to_row > 2 || to_col < 3 || to_col > 5)
        {
            return 0;
        }
    }

    // Next, check that the general is only moving one space
    if (board[from_row][from_col].type == JIANG)
    {
        if (abs(to_row - from_row) > 1 || abs(to_col - from_col) > 1)
        {
            return 0;
        }
    }

    // Check to make sure there are no peices of the same color in the destination
    if (board[from_row][from_col].colour == board[to_row][to_col].colour)
    {
        return 0;
    }

    // Check to see if there is a piece of the opposite color in the destination
    if (board[from_row][from_col].colour != board[to_row][to_col].colour)
    {
        // This is a capture!
        return 1;
    }

    // If we get here, the move is valid
    return 1;
}

int is_valid_move_for_advisor(int from_row, int from_col, int to_row, int to_col)
{
    // Advisors must stay in the palac
    // First check the black advisor
    if (board[from_row][from_col].type == SHI)
    {
        if (to_row < 7 || to_col < 3 || to_col > 5)
        {
            return 0;
        }
    }

    // Next, check the white advisor
    if (board[from_row][from_col].type == SHI)
    {
        if (to_row > 2 || to_col < 3 || to_col > 5)
        {
            return 0;
        }
    }

    // Check to make sure the advisor is only moving diagonally
    if (board[from_row][from_col].type == SHI)
    {
        if (abs(to_row - from_row) != 1 || abs(to_col - from_col) != 1)
        {
            return 0;
        }
    }

    // Check to make sure there are no peices of the same color in the destination
    if (board[from_row][from_col].colour == board[to_row][to_col].colour)
    {
        return 0;
    }

    // Check to see if there is a piece of the opposite color in the destination
    if (board[from_row][from_col].colour != board[to_row][to_col].colour)
    {
        // This is a capture!
        return 1;
    }

    // If we get here, the move is valid
    return 1;
}

int is_valid_move_for_elephant(int from_row, int from_col, int to_row, int to_col)
{
    // Elephants must stay within their own half of the board
    // First check the black elephant
    if (board[from_row][from_col].type == XIANG)
    {
        if (to_row < 5)
        {
            return 0;
        }
    }

    // Next, check the white elephant
    if (board[from_row][from_col].type == XIANG)
    {
        if (to_row > 4)
        {
            return 0;
        }
    }

    // Check to make sure the elephant is moving in an X shape
    if (board[from_row][from_col].type == XIANG)
    {
        if (abs(to_row - from_row) != 2 || abs(to_col - from_col) != 2)
        {
            return 0;
        }
    }

    // Elephants can't jump over other pieces, check all four possible moves
    if (board[from_row][from_col].type == XIANG)
    {
        // Check the top left move
        if (to_row - from_row == 2 && to_col - from_col == 2)
        {
            if (board[from_row + 1][from_col + 1].type != None)
            {
                return 0;
            }
        }
        // Check the top right move
        if (to_row - from_row == 2 && to_col - from_col == -2)
        {
            if (board[from_row + 1][from_col - 1].type != None)
            {
                return 0;
            }
        }
        // Check the bottom left move
        if (to_row - from_row == -2 && to_col - from_col == 2)
        {
            if (board[from_row - 1][from_col + 1].type != None)
            {
                return 0;
            }
        }
        // Check the bottom right move
        if (to_row - from_row == -2 && to_col - from_col == -2)
        {
            if (board[from_row - 1][from_col - 1].type != None)
            {
                return 0;
            }
        }
    }

    // Check to make sure there are no peices of the same color in the destination
    if (board[from_row][from_col].colour == board[to_row][to_col].colour)
    {
        return 0;
    }

    // Check to see if there is a piece of the opposite color in the destination
    if (board[from_row][from_col].colour != board[to_row][to_col].colour)
    {
        // This is a capture!
        return 1;
    }

    // If we get here, the move is valid
    return 1;
}

int is_valid_move_horse(int from_row, int from_col, int to_row, int to_col)
{
    // Horses can move in an L shape
    // Check to make sure the horse is moving in an L shape
    if (board[from_row][from_col].type == MA)
    {
        if (abs(to_row - from_row) == 2 && abs(to_col - from_col) == 1)
        {
            if (board[from_row + (to_row - from_row) / 2][from_col].type != None)
            {
                return 0;
            }
        }
        else if (abs(to_row - from_row) == 1 && abs(to_col - from_col) == 2)
        {
            if (board[from_row][from_col + (to_col - from_col) / 2].type != None)
            {
                return 0;
            }
        }
        else
        {
            // The horse is not moving in an L shape
            return 0;
        }
    }

    // Check that the horse is not blocked by a piece
    if (board[from_row][from_col].type == MA)
    {
        // Moving vertically
        if (abs(to_row - from_row) == 2)
        {
            int block_row = from_row + (to_row - from_row) / 2;
            if (board[block_row][from_col].type != None)
            {
                return 0; // Blocked by a piece
            }
        }
        // Moving horizontally
        else if (abs(to_col - from_col) == 2)
        {
            int block_col = from_col + (to_col - from_col) / 2;
            if (board[from_row][block_col].type != None)
            {
                return 0; // Blocked by a piece
            }
        }
    }

    // Check to make sure there are no peices of the same color in the destination
    if (board[from_row][from_col].colour == board[to_row][to_col].colour)
    {
        return 0;
    }

    // Check to see if there is a piece of the opposite color in the destination
    if (board[from_row][from_col].colour != board[to_row][to_col].colour)
    {
        // This is a capture!
        return 1;
    }

    // If we get here, the move is valid
    return 1;
}

int is_valid_move_for_chariot(int from_row, int from_col, int to_row, int to_col)
{
    // Chariots can move horizontally or vertically
    // Check to make sure the chariot is moving in a straight line
    if (board[from_row][from_col].type == JU)
    {
        if (from_row != to_row && from_col != to_col)
        {
            return 0;
        }
    }

    // Charoits can't jump over other pieces
    if (board[from_row][from_col].type == JU)
    {
        // Moving horizontally
        if (from_row == to_row)
        {
            int start = (from_col < to_col) ? from_col : to_col;
            int end = (from_col < to_col) ? to_col : from_col;
            for (int i = start + 1; i < end; i++)
            {
                if (board[from_row][i].type != None)
                {
                    return 0;
                }
            }
        }
        // Moving vertically
        if (from_col == to_col)
        {
            int start = (from_row < to_row) ? from_row : to_row;
            int end = (from_row < to_row) ? to_row : from_row;
            for (int i = start + 1; i < end; i++)
            {
                if (board[i][from_col].type != None)
                {
                    return 0;
                }
            }
        }
    }

    // Check to make sure there are no peices of the same color in the destination
    if (board[from_row][from_col].colour == board[to_row][to_col].colour)
    {
        return 0;
    }

    // Check to see if there is a piece of the opposite color in the destination
    if (board[from_row][from_col].colour != board[to_row][to_col].colour)
    {
        // This is a capture!
        return 1;
    }

    // If we get here, the move is valid
    return 1;
}

int is_valid_move_for_cannon(int from_row, int from_col, int to_row, int to_col)
{
    // Cannons can only move vertically or horizontally in straigt lines
    // Check to make sure the cannon is moving in a straight line
    if (board[from_row][from_col].type == PAO)
    {
        // One of the coordinates must be the same!
        if (from_row != to_row && from_col != to_col)
        {
            return 0;
        }
    }

    // Cannons can't jump over other pieces (unless they are taking a piece!)
    // Let's see if there are any pieces in the way
    if (board[from_row][from_col].type == PAO)
    {
        // Moving horizontally
        int count = 0;
        if (from_row == to_row)
        {
            int start = (from_col < to_col) ? from_col : to_col;
            int end = (from_col < to_col) ? to_col : from_col;
            for (int i = start + 1; i < end; i++)
            {
                if (board[from_row][i].type != None)
                {
                    count++;
                }
            }
            if (count > 1)
            {
                return 0;
            }
        }
        // Moving vertically
        if (from_col == to_col)
        {
            int start = (from_row < to_row) ? from_row : to_row;
            int end = (from_row < to_row) ? to_row : from_row;
            for (int i = start + 1; i < end; i++)
            {
                if (board[i][from_col].type != None)
                {
                    count++;
                }
            }
            if (count > 1)
            {
                return 0;
            }
        }

        // If the count is 1, let's see if there is a piece to capture at the destination
        if (count == 1)
        {
            if (board[to_row][to_col].type == None)
            {
                return 0;
            }
            // Otherwise, see if the piece is of the opposite color
            if (board[from_row][from_col].colour == board[to_row][to_col].colour)
            {
                return 0;
            }
            // This is a capture!
            return 1;
        }
    }

    // If we get here, the destination has to be empty
    if (board[to_row][to_col].type != None)
    {
        return 0;
    }

    // If we get here, the move is valid
    return 1;
}

int is_valid_move_for_soldier(int from_row, int from_col, int to_row, int to_col)
{
    // Soldiers can only move forward
    // Check to make sure the soldier is moving forward
    if (board[from_row][from_col].type == BING)
    {
        if (board[from_row][from_col].colour == BLACK)
        {
            if (to_row <= from_row)
            {
                return 0;
            }
        }
        if (board[from_row][from_col].colour == WHITE)
        {
            if (to_row >= from_row)
            {
                return 0;
            }
        }        
    }

    // Soldiers can move horizontally once they cross the river
    // Check to make sure the soldier is allowed to move horizontally if they are past the river
    if (board[from_row][from_col].type == BING)
    {
        if (board[from_row][from_col].colour == BLACK)
        {
            if (to_row >= 5)
            {
                if (abs(to_col - from_col) > 1)
                {
                    return 0;
                }
            }
        }
        if (board[from_row][from_col].colour == WHITE)
        {
            if (to_row <= 4)
            {
                if (abs(to_col - from_col) > 1)
                {
                    return 0;
                }
            }
        }
    }

    // Soldiers can only move one space
    if (board[from_row][from_col].type == BING)
    {
        if (abs(to_row - from_row) > 1 || abs(to_col - from_col) > 1)
        {
            return 0;
        }
    }

    // Check to make sure there are no peices of the same color in the destination
    if (board[from_row][from_col].colour == board[to_row][to_col].colour)
    {
        return 0;
    }

    // Check to see if there is a piece of the opposite color in the destination
    if (board[from_row][from_col].colour != board[to_row][to_col].colour)
    {
        // This is a capture!
        return 1;
    }

    // If we get here, the move is valid
    return 1;
}

int is_valid_move(int from_row, int from_col, int to_row, int to_col, P_Colour player)
{

    // printf("Validating move...\n");
    // printf("Current player: %d\n", player);

    // printf("From: %d %d\n", from_row, from_col);
    // printf("To: %d %d\n", to_row, to_col);

    // printf("Piece: %d\n", board[from_row][from_col].type);
    // printf("Colour: %d\n", board[from_row][from_col].colour);
    // printf("Destination: %d\n", board[to_row][to_col].type);
    // printf("Destination colour: %d\n", board[to_row][to_col].colour);

    // Make sure the player is moving their own piece
    if (board[from_row][from_col].colour != player)
    {
        //printf("You must move your own piece\n");
        return 0;
    }

    // Check that we are not moving onto ourselves
    if (from_row == to_row && from_col == to_col)
    {
        //printf("You must move to a different position\n");
        return 0;
    }

    // Check if the move is within the board
    if (from_row < 0 || from_row >= BOARD_SIZE_X || from_col < 0 || from_col >= BOARD_SIZE_Y)
    {
        //printf("You must move within the board\n");
        return 0;
    }

    if (to_row < 0 || to_row >= BOARD_SIZE_X || to_col < 0 || to_col >= BOARD_SIZE_Y)
    {
        //printf("You must move within the board\n");
        return 0;
    }

    // Generals
    if (board[from_row][from_col].type == JIANG)
    {
        //printf("Validating general move...\n");
        return is_valid_move_for_general(from_row, from_col, to_row, to_col);
    }

    // Advisors
    if (board[from_row][from_col].type == SHI)
    {
        //printf("Validating advisor move...\n");
        return is_valid_move_for_advisor(from_row, from_col, to_row, to_col);
    }

    // Elephants
    if (board[from_row][from_col].type == XIANG)
    {
        //printf("Validating elephant move...\n");
        return is_valid_move_for_elephant(from_row, from_col, to_row, to_col);
    }

    // Horses
    if (board[from_row][from_col].type == MA)
    {
        //printf("Validating horse move...\n");
        return is_valid_move_horse(from_row, from_col, to_row, to_col);
    }

    // Chariots
    if (board[from_row][from_col].type == JU)
    {
        //printf("Validating chariot move...\n");
        return is_valid_move_for_chariot(from_row, from_col, to_row, to_col);
    }

    // Cannons
    if (board[from_row][from_col].type == PAO)
    {
        //printf("Validating cannon move...\n");
        return is_valid_move_for_cannon(from_row, from_col, to_row, to_col);
    }

    // Soldiers
    if (board[from_row][from_col].type == BING)
    {
        //printf("Validating soldier move...\n");
        return is_valid_move_for_soldier(from_row, from_col, to_row, to_col);
    }

    //printf("Invalid move\n");
    // If we get here, the move is not valid, either we don't have a piece or something else went wrong
    return 0;
}

void update_board(int from_row, int from_col, int to_row, int to_col, P_Colour player)
{
    // printf("Updating board...\n");
    // printf("From: %d %d\n", from_row, from_col);
    // printf("To: %d %d\n", to_row, to_col);
    // printf("Piece: %d\n", board[from_row][from_col].type);
    // printf("Colour: %d\n", board[from_row][from_col].colour);
    // printf("Destination: %d\n", board[to_row][to_col].type);
    // printf("Destination colour: %d\n", board[to_row][to_col].colour);

    if (is_valid_move(from_row, from_col, to_row, to_col, player))
    //{
        // Move the piece
        board[to_row][to_col] = board[from_row][from_col];

        // Clear the old position
        // Init with an empty piece
        Piece empty;
        empty.type = None;
        empty.colour = EMPTY;
        empty.code = N;
        empty.board_code = '.';
            
        board[from_row][from_col]= empty;
    //}
}

int check_win()
{
    // Check if the game is over
    // The game is over if one of the generals is captured
    int black_general = 0;
    int white_general = 0;

    for (int i = 0; i < BOARD_SIZE_X; i++)
    {
        for (int j = 0; j < BOARD_SIZE_Y; j++)
        {
            if (board[i][j].type == JIANG)
            {
                if (board[i][j].colour == BLACK)
                {
                    black_general = 1;
                }
                if (board[i][j].colour == WHITE)
                {
                    white_general = 1;
                }
            }
        }
    }

    if (black_general == 0)
    {
        return 1;
    }
    if (white_general == 0)
    {
        return 2;
    }

    return 0;
}

int parse_move(const char *from, const char *to, P_Colour player)
{
    //printf("Parse move...\n");
    // Check if the input length is valid for "a4 a8" format
    // For now, I don't care about supporting more than one space...
    if (strlen(from) != 2 || strlen(to) != 2)
    {
        printf("You must enter moves in the format 'a4 a8'\n");
        return 0;
    }

    //printf("Good length...\n");
    //printf("From: %s\n", from);
    //printf("To: %s\n", to);

    // Parse from position
    int from_row = from[0] - 'a'; // Convert column from letter to index
    int from_col = from[1] - '1'; // Convert row from char to index

    // Parse to position
    int to_row = to[0] - 'a'; // Convert column from letter to index, skipping the space
    int to_col = to[1] - '1'; // Convert row from char to index

    //printf("Parsed...\n");

    //printf("From: %d %d\n", from_row, from_col);
    //printf("To: %d %d\n", to_row, to_col);
    
    if (is_valid_move(from_row, from_col, to_row, to_col, player))
    {
        update_board(from_row, from_col, to_row, to_col, player);
        return 1;
    }
    else
        return 0;
}

Piece get_piece(int row, int col) {
    if (row >= 0 && row < BOARD_SIZE_X && col >= 0 && col < BOARD_SIZE_Y) {
        return board[row][col];
    }
    return empty;
}