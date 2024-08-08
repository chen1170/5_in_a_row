#include "board.h"
#include <mpi.h>

#define min(a, b) ((a) < (b) ? (a) : (b))

int rank, size;
MPI_Status status;

struct Job {
    int alpha; // Alpha value for alpha-beta pruning
    PlayerType player; // Current player's symbol
    GameBoard board; // Current state of the board
};

// Function prototypes
int evaluate_position(GameBoard*, int, PlayerType);
int simulate_move(GameBoard*, PlayerType, int, int, int);

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    PlayerType result; // Result after all moves are made
    PlayerType current_player = PLAYER_X; // Starting symbol
    GameBoard* board = create_board(); // Initialize the game board
    int score; // Score of a move
    PlayerType done_player = 2; // Symbol indicating a process should terminate

    struct Job* job = malloc(sizeof(struct Job)); // Job structure for MPI communication

    if (rank == 0) { // Master process
        int i, n, best_score_index = 0, best_score;
        GameMove** moves;
        int finished[size]; // Array to check if a process is finished
        int current_move[size]; // Track moves sent to each process

        memset(finished, 0, sizeof(finished));

        while (1) {
            best_score = -9999; // Reset the best score for each round
            job->alpha = best_score;

            printf("Player %i to move next\n", (int)current_player);

            moves = generate_possible_moves(board, &n); // Get all possible moves

            // Send a job to each available process
            for (i = 0; i < min(n, size - 1); i++) {
                printf("send move %i to %i\n", i, i + 1);

                place_player(board, current_player, moves[i]);

                job->board = *board;
                job->player = toggle_player(current_player);

                MPI_Send(job, sizeof(struct Job), MPI_BYTE, i + 1, 1, MPI_COMM_WORLD);
                remove_player(board, moves[i]);

                current_move[i + 1] = i;
            }

            // Receive scores and update best score
            for (i = 0; i < min(n, size - 1); i++) {
                MPI_Recv(&score, 1, MPI_INT, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, &status);
                if (score > best_score) {
                    best_score = score;
                    best_score_index = current_move[status.MPI_SOURCE];
                }
                printf("received score %i from %i\n", score, status.MPI_SOURCE);
            }

            place_player(board, current_player, moves[best_score_index]); // Make the best move

            display_board(board); // Print the current state of the board

            // Free memory for moves
            for (i = 0; i < n; i++) {
                free(moves[i]);
            }
            free(moves);

            result = check_winner(board); // Check if there is a winner
            if (result != PLAYER_EMPTY) {
                break; // Exit if we have a winner
            }
            current_player = toggle_player(current_player); // Switch player
        }

        // Signal all processes to terminate
        job->player = done_player;
        for (i = 1; i < size; i++) {
            if (finished[i]) break;

            printf("finishing %i\n", i);
            MPI_Send(job, sizeof(struct Job), MPI_BYTE, i, 1, MPI_COMM_WORLD);
            finished[i] = 1;
        }

        printf("Winner: %i\n", (int)result); // Announce the winner
    } else {
        // Worker process
        while (1) {
            MPI_Recv(job, sizeof(struct Job), MPI_BYTE, 0, 1, MPI_COMM_WORLD, &status);
            if (job->player == done_player) {
                break; // Exit if done signal is received
            }
            score = -simulate_move(&job->board, job->player, 0, -9999, -job->alpha);
            MPI_Send(&score, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize(); // Finalize MPI
    return 0;
}

// Heuristic function to get score of the board
int evaluate_position(GameBoard* board, int depth, PlayerType player) {
    PlayerType result = check_winner(board);

    if (result == player) {
        return BOARD_SIZE * BOARD_SIZE + 10 - depth;
    } else if (result != PLAYER_EMPTY && result != PLAYER_NO_WINNER) {
        return -(BOARD_SIZE * BOARD_SIZE) - 10 + depth;
    } else if (result == PLAYER_NO_WINNER) {
        return 1;
    }

    return 0;
}

// Recursive function to simulate moves and return the best score
int simulate_move(GameBoard* board, PlayerType player, int depth, int alpha, int beta) {
    int move_count, i;
    int score = evaluate_position(board, depth, player);

    if (score != 0) {
        return score;
    }

    GameMove** moves = generate_possible_moves(board, &move_count);
    for (i = 0; i < move_count; i++) {
        place_player(board, player, moves[i]);
        score = -simulate_move(board, toggle_player(player), depth + 1, -beta, -alpha);
        remove_player(board, moves[i]);

        if (score > alpha) {
            alpha = score;
        }

        if (alpha >= beta) {
            break;
        }
    }

    for (i = 0; i < move_count; i++) {
        free(moves[i]);
    }
    free(moves);

    return alpha;
}
