#ifndef GAME_H
#define GAME_H

#include "board.h"

// 游戏模式
typedef enum {
    HUMAN_VS_AI,
    AI_VS_AI,
    PARALLEL_AI,
    HUMAN_VS_PARALLEL_AI
} GameMode;

// 初始化游戏
void init_game(GameMode mode);

// 运行游戏主循环
void run_game();

#endif // GAME_H