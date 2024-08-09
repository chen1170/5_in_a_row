// /* nondet.c */
#include <stdio.h>
#include "stdlib.h"

#include "sys/time.h"
// #include "time.h"

// #include "unistd.h"
#include <string.h>

#include <mpi.h>

#include "game.h"
#include "parallel.h"


struct timeval tv;

void run_performance_test(GameMode mode, int num_steps) {
    double start_time, end_time;
    double serial_time = 0.0, parallel_time = 0.0;
    
    start_time = MPI_Wtime();
    for (int i = 0; i < num_steps; i++) {
        printf("Running game %d of %d\n", i + 1, num_steps);
        
        // Runtime for init_game
        double init_start = MPI_Wtime();
        init_game(mode);
        double init_end = MPI_Wtime();
        serial_time += (init_end - init_start);
        
        // Runtime for parallel part
        double game_serial_time = 0.0, game_parallel_time = 0.0;
        run_game(0, &game_serial_time, &game_parallel_time);
        
        serial_time += game_serial_time;
        parallel_time += game_parallel_time;

    }
    end_time = MPI_Wtime();

    double total_time = end_time - start_time;
    printf("\n");
    printf("Performance test for mode %d took %f seconds\n", mode, total_time);
    if (parallel_time > 0) {
        printf("Total parallel time: %f seconds (%.2f%%)\n", parallel_time, (parallel_time / total_time) * 100);
    }
    printf("\n");
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
        GameMode mode = HUMAN_VS_AI;  
        int num_steps = 10;  

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
                
                cleanup_parallel_env();
                MPI_Finalize();
                return 0;
            }
        }

        double game_serial_time = 0.0, game_parallel_time = 0.0;  

        init_game(mode);
        run_game(0, &game_serial_time, &game_parallel_time);
        //printf("Rank(0) Done...\n");
        cleanup_parallel_env();
    }
    else
    {
        init_parallel_env();
        parallel_worker();  
    }

    MPI_Finalize();
    return 0;
}
