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
            Piece empty;
            empty.type = None;
            empty.colour = EMPTY;
            empty.code = N;
            empty.board_code = '.';
            board[row][col] = empty;

            // RED pieces

            // Chariots
            if (row == 0 && (col == 0 || col == 8))
            {
                Piece ju;
                ju.type = JU;
                ju.colour = RED;
                ju.code = R;
                ju.board_code = 'R';
                board[row][col] = ju;
            }

            // Horses
            if (row == 0 && (col == 1 || col == 7))
            {
                Piece ma;
                ma.type = MA;
                ma.colour = RED;
                ma.code = H;
                ma.board_code = 'H';
                board[row][col] = ma;
            }

            // Elephants
            if (row == 0 && (col == 2 || col == 6))
            {
                Piece xiang;
                xiang.type = XIANG;
                xiang.colour = RED;
                xiang.code = E;
                xiang.board_code = 'E';
                board[row][col] = xiang;
            }

            // Advisors
            if (row == 0 && (col == 3 || col == 5))
            {
                Piece shi;
                shi.type = SHI;
                shi.colour = RED;
                shi.code = A;
                shi.board_code = 'A';
                board[row][col] = shi;
            }

            // General
            if (row == 0 && col == 4)
            {
                Piece jiang;
                jiang.type = JIANG;
                jiang.colour = RED;
                jiang.code = G;
                jiang.board_code = 'G';
                board[row][col] = jiang;
            }

            // Cannons
            if (row == 2 && (col == 1 || col == 7))
            {
                Piece pao;
                pao.type = PAO;
                pao.colour = RED;
                pao.code = C;
                pao.board_code = 'C';
                board[row][col] = pao;
            }

            // Soldiers
            if (row == 3 && (col % 2 == 0))
            {
                Piece bing;
                bing.type = BING;
                bing.colour = RED;
                bing.code = S;
                bing.board_code = 'S';
                board[row][col] = bing;
            }

            // BLACK pieces

            // Chariots
            if (row == 9 && (col == 0 || col == 8))
            {
                Piece ju;
                ju.type = JU;
                ju.colour = BLACK;
                ju.code = R;
                ju.board_code = 'r';
                board[row][col] = ju;
            }

            // Horses
            if (row == 9 && (col == 1 || col == 7))
            {
                Piece ma;
                ma.type = MA;
                ma.colour = BLACK;
                ma.code = H;
                ma.board_code = 'h';
                board[row][col] = ma;
            }

            // Elephants
            if (row == 9 && (col == 2 || col == 6))
            {
                Piece xiang;
                xiang.type = XIANG;
                xiang.colour = BLACK;
                xiang.code = E;
                xiang.board_code = 'e';
                board[row][col] = xiang;
            }

            // Advisors
            if (row == 9 && (col == 3 || col == 5))
            {
                Piece shi;
                shi.type = SHI;
                shi.colour = BLACK;
                shi.code = A;
                shi.board_code = 'a';
                board[row][col] = shi;
            }

            // General
            if (row == 9 && col == 4)
            {
                Piece jiang;
                jiang.type = JIANG;
                jiang.colour = BLACK;
                jiang.code = G;
                jiang.board_code = 'g';
                board[row][col] = jiang;
            }

            // Cannons
            if (row == 7 && (col == 1 || col == 7))
            {
                Piece pao;
                pao.type = PAO;
                pao.colour = BLACK;
                pao.code = C;
                pao.board_code = 'c';
                board[row][col] = pao;
            }

            // Soldiers
            if (row == 6 && (col % 2 == 0))
            {
                Piece bing;
                bing.type = BING;
                bing.colour = BLACK;
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
    // Check that the general stays within the palace boundaries
    if ((board[from_row][from_col].colour == BLACK && (to_row < 7 || to_col < 3 || to_col > 5)) ||
        (board[from_row][from_col].colour == RED && (to_row > 2 || to_col < 3 || to_col > 5)))
    {
        return 0; // General must stay within the palace for respective color
    }

    // Check that the general moves only one space and only vertically or horizontally
    if (!((abs(to_row - from_row) == 1 && to_col == from_col) ||
          (abs(to_col - from_col) == 1 && to_row == from_row)))
    {
        return 0; // Invalid move if the general tries to move diagonally or more than one space
    }

    // Check to make sure there are no pieces of the same color in the destination
    if (board[from_row][from_col].colour == board[to_row][to_col].colour)
    {
        return 0; // Cannot capture own pieces
    }

    // If we get here, the move is valid if it's either a capture or to an empty spot
    return 1;
}

int is_valid_move_for_advisor(int from_row, int from_col, int to_row, int to_col)
{
    if (board[from_row][from_col].type != SHI)
    {
        return 0; // Not an advisor
    }

    // Advisors must stay in the palace
    int palace_top = (board[from_row][from_col].colour == RED) ? 0 : 7;
    int palace_bottom = (board[from_row][from_col].colour == RED) ? 2 : 9;
    if (to_row < palace_top || to_row > palace_bottom || to_col < 3 || to_col > 5)
    {
        return 0; // Advisor must stay within the palace
    }

    // Check to make sure the advisor is only moving diagonally by one space
    if (abs(to_row - from_row) != 1 || abs(to_col - from_col) != 1)
    {
        return 0; // Advisors move only one step diagonally
    }

    // Ensure the destination does not contain a piece of the same color
    if (board[from_row][from_col].colour == board[to_row][to_col].colour)
    {
        return 0; // Cannot move to a location with a piece of the same color
    }

    // Valid move, could be capture or to an empty spot
    return 1;
}

int is_valid_move_for_elephant(int from_row, int from_col, int to_row, int to_col)
{
    if (board[from_row][from_col].type != XIANG)
    {
        // printf("Not an elephant\n");
        return 0; // Only apply rules to elephants
    }

    // Elephants must stay within their own half of the board
    if ((board[from_row][from_col].colour == BLACK && to_row < 5) ||
        (board[from_row][from_col].colour == RED && to_row > 4))
    {
        // printf("Elephant must stay on their side of the board\n");
        return 0; // Elephants must stay on their side of the river
    }

    // Ensure elephant is moving in an X shape exactly two steps away
    if (abs(to_row - from_row) != 2 || abs(to_col - from_col) != 2)
    {
        // printf("Not an X move\n");
        return 0; // Not a valid "X" move
    }

    // Check for blocking pieces at the crossing point ("elephant's eye")
    int mid_row = (from_row + to_row) / 2;
    int mid_col = (from_col + to_col) / 2;
    if (board[mid_row][mid_col].type != None)
    {
        // printf("Blocked by a piece at the elephant's eye\n");
        return 0; // Blocked by a piece at the elephant's eye
    }

    // Check to make sure there are no pieces of the same color in the destination
    if (board[from_row][from_col].colour == board[to_row][to_col].colour)
    {
        // printf("Cannot capture own pieces\n");
        return 0; // Cannot move to a location with a piece of the same color
    }

    // All checks passed, valid move
    return 1; // This includes captures or moving to an empty spot
}

int is_valid_move_horse(int from_row, int from_col, int to_row, int to_col)
{
    // Check if the piece is a horse
    if (board[from_row][from_col].type != MA)
    {
        return 0;
    }

    // Check the L-shape movement
    if ((abs(to_row - from_row) == 2 && abs(to_col - from_col) == 1) ||
        (abs(to_row - from_row) == 1 && abs(to_col - from_col) == 2))
    {
        // Calculate the position of the "leg" of the horse to check for blocking
        int block_row = from_row + (to_row - from_row) / 2;
        int block_col = from_col + (to_col - from_col) / 2;

        // Check if the leg position is blocked
        if (board[block_row][block_col].type != None)
        {
            return 0; // Move is blocked
        }
    }
    else
    {
        return 0; // Not a valid L-shape move
    }

    // Ensure destination does not contain a piece of the same color
    if (board[from_row][from_col].colour == board[to_row][to_col].colour)
    {
        return 0; // Cannot move to a location with a piece of the same color
    }

    // Valid move if it's either a capture or moving to an empty spot
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
    // Check if the piece is a soldier
    if (board[from_row][from_col].type != BING)
    {
        return 0;
    }

    int vertical_direction = board[from_row][from_col].colour == RED ? 1 : -1;

    // Check if not moving forward one step
    if (to_row != from_row + vertical_direction || to_col != from_col)
    {
        return 0; // Invalid forward move
    }

    // Check for horizontal move past the river
    if ((board[from_row][from_col].colour == RED && from_row >= 5) ||
        (board[from_row][from_col].colour == BLACK && from_row <= 4))
    {
        if (to_row == from_row && abs(to_col - from_col) == 1)
        {
            return 0; // Invalid horizontal move past the river
        }
    }

    // Check if the destination has a piece of the same color
    if (board[to_row][to_col].colour == board[from_row][from_col].colour)
    {
        return 0; // Cannot capture own pieces
    }

    return 0; // All other moves are invalid
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
    {
        // Move the piece
        printf("Moving piece: (%d, %d) to (%d, %d)\n", from_row, from_col, to_row, to_col);

        board[to_row][to_col] = board[from_row][from_col];

        // Clear the old position
        // Init with an empty piece
        Piece empty;
        empty.type = None;
        empty.colour = EMPTY;
        empty.code = N;
        empty.board_code = '.';

        board[from_row][from_col] = empty;
    }
}

int check_win()
{
    // Check if the game is over
    // The game is over if one of the generals is captured
    int RED_general = 0;
    int BLACK_general = 0;

    for (int i = 0; i < BOARD_SIZE_X; i++)
    {
        for (int j = 0; j < BOARD_SIZE_Y; j++)
        {
            if (board[i][j].type == JIANG)
            {
                if (board[i][j].colour == RED)
                {
                    RED_general = 1;
                }
                if (board[i][j].colour == BLACK)
                {
                    BLACK_general = 1;
                }
            }
        }
    }

    if (RED_general == 0)
    {
        return 1;
    }
    if (BLACK_general == 0)
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

int evaluate_move(int from_row, int from_col, int to_row, int to_col, P_Colour player, int depth)
{
    //printf(" -->Evaluating move (%d %d) =>  (%d %d), depth = %d\n", from_row, from_col, to_row, to_col, depth);

    // We don't need to remember any of these moves, we just need to pass back the score so that the first move maps
    // to the move tree with the best resulting score

    if (depth == 0)
    {
        //printf(" -->Depth 0\n");
        return evaluate_board(player);
    }

    else
    {
        --depth;
        int score = 0;

        if (is_valid_move(from_row, from_col, to_row, to_col, player))
        {
            //printf(" -->Valid move\n");
            // Make a copy of the game board
            Piece *board_copy;
            board_copy = malloc(BOARD_SIZE_X * BOARD_SIZE_Y * sizeof(Piece));
            if (board_copy == NULL)
            {
                //printf("Failed to allocate memory for board_copy\n");
                return -1;
            }

            memcpy(board_copy, board, BOARD_SIZE_X * BOARD_SIZE_Y * sizeof(Piece));

            //printf(" -->Made copy\n");

            // Make the move
            //update_board(from_row, from_col, to_row, to_col, player);

            //printf(" -->Updated board\n");

            for (int i = 0; i < BOARD_SIZE_X; i++)
            {
                for (int j = 0; j < BOARD_SIZE_Y; j++)
                {
                    // Go one level deeper...
                    if (player == RED)
                        score = evaluate_move(from_row, from_col, i, j, BLACK, depth);
                    else
                        score = evaluate_move(from_row, from_col, i, j, RED, depth);
                }
            }

            // printf(" -->Done with depth\n");

            // Undo the move
            memcpy(board, board_copy, BOARD_SIZE_X * BOARD_SIZE_Y * sizeof(Piece));

            free(board_copy);
            //printf("Score returned: %d\n", score);
            return score;

        }
        //printf("Invalid move...\n");
        return evaluate_board(player);
    }
}

static int evaluate_board(P_Colour player)
{
    //printf("Player: %d\n", player);
    //print_board();
    // Print which player we are evaluating the board for:
    //printf("Evaluating board for player %d\n", player);

    // Had to come up with something, so I just count the piece values (loosely based on what I read online)
    int black_score = 0;
    int red_score = 0;
    int score = 0;
    if (player == BLACK || player == RED) {
        for (int row = 0; row < BOARD_SIZE_X; row++)
        {
            for (int col = 0; col < BOARD_SIZE_Y; col++)
            {
                if (board[row][col].colour == player)
                {
                    switch (board[row][col].type)
                    {
                    case BING:
                        score = 1;
                        break; // 2 if passed the river... need code for that
                    case SHI:
                        score = 2;
                        break;
                    case XIANG:
                        score = 3;
                        break;
                    case MA:
                        score = 4;
                        break;
                    case PAO:
                        score = 5;
                        break;
                    case JU:
                        score = 10;
                        break;
                    case JIANG:
                        score = 1000;
                        break;
                    default:
                        break;
                    }
                }

                if (player == BLACK)
                {
                    black_score += score;
                }
                else
                {
                    red_score += score;
                }
            }
        }
    }
    else
    {
        printf("Invalid player\n");
    }

    // Return the score differential - this calculates, not just the players
    // score, but their advantage (or disadvantage) over the other player
    // in terms of their score. This has the effect of favouring moves
    // that provide not just a higer score than another move, but a larger
    // advantage over the other player.

    // Eg. If after a move, the score is 100 for Black and 90 for Red, the score
    // Differential for Black would be 10. However, if another move provides a
    // score of 95 for Black, but captures a higher value piece from Red so that
    // Red's score is is 80, the score differential would be 15, and would favour
    // this move, even though the actual score for Black is less.

    //printf("Scores: Black: %d, Red: %d\n", black_score, red_score);
    if (player == BLACK)
    {
        return black_score ; //- red_score;
    }
    else
    {
        return red_score ; //- black_score;
    }
}