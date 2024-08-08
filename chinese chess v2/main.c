// /* nondet.c */
// #include <stdio.h>
#include "stdlib.h"

#include "sys/time.h"
// #include "time.h"

// #include "unistd.h"
#include <string.h>

#include <mpi.h>

#include "game.h"
#include "parallel.h"


struct timeval tv;

int main(int argc, char *argv[])
{
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec);

    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        // 主进程运行游戏逻辑
        GameMode mode = HUMAN_VS_AI;  // 默认模式

        if (argc > 1) {
            if (strcmp(argv[1], "ai") == 0) {
                mode = AI_VS_AI;
            } else if (strcmp(argv[1], "parallel") == 0) {
                mode = PARALLEL_AI;
            } else if (strcmp(argv[1], "human_parallel") == 0) {
                mode = HUMAN_VS_PARALLEL_AI;
            }
        }

        init_game(mode);
        run_game();
    }
    else
    {
        // 工作进程等待并执行并行任务
        init_parallel_env();
        parallel_worker();  // 初始玩家设为 BLACK，但在实际运行中会被正确的玩家替换
    }

    MPI_Finalize();
    return 0;
}