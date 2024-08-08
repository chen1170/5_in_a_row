/* nondet.c */
#include <stdio.h>
#include "stdlib.h"

#include "sys/time.h"
#include "time.h"

#include "unistd.h"
#include <string.h>

#include <mpi.h>

#include "game.h"
#include "parallel.h"


struct timeval tv;

void run_performance_test(GameMode mode, int num_steps) {
    double start_time, end_time;
    init_game(mode);
    start_time = MPI_Wtime();
    for (int i = 0; i < num_steps; i++) {
        run_game();
        if (mode == PARALLEL_AI || mode == HUMAN_VS_PARALLEL_AI) {
            cleanup_parallel_env();
            init_parallel_env();
        }
    }

    end_time = MPI_Wtime();
    printf("Performance test for mode %d took %f seconds\n", mode, end_time - start_time);
}

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
        int num_steps = 10;  // 默认运行的游戏步骤数

        if (argc > 1) {
            if (strcmp(argv[1], "ai") == 0) {
                mode = AI_VS_AI;
            } else if (strcmp(argv[1], "parallel") == 0) {
                mode = PARALLEL_AI;
            } else if (strcmp(argv[1], "human_parallel") == 0) {
                mode = HUMAN_VS_PARALLEL_AI;
            } else if (strcmp(argv[1], "test") == 0) {
                if (argc > 2) {
                    num_steps = atoi(argv[2]);
                }
                printf("Running performance tests...\n");
                // printf("Testing non-parallel AI...\n");
                run_performance_test(AI_VS_AI, num_steps);

                printf("Testing parallel AI...\n");
                run_performance_test(PARALLEL_AI, num_steps);

                MPI_Finalize();
                return 0;
            }
        }

        init_game(mode);
        //print_board();
        run_game();
        //printf("Rank(0) Done...\n");
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
