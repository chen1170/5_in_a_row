#ifndef GAME_H
#define GAME_H

typedef enum {
    HUMAN_VS_AI,
    AI_VS_AI,
    PARALLEL_AI,
    HUMAN_VS_PARALLEL_AI
} GameMode;

void init_game(GameMode mode);

void run_game(int show_board, double *serial_time, double *parallel_time);

#endif // GAME_H