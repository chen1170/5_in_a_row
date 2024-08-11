#!/bin/bash
#SBATCH --time=00:30:00
#SBATCH --account=mcs

for np in 2 3 5 9 17; do
    for games in 1 10 20; do
        mpirun -np $np -mca btl ^openib ./main.x test $games
    done
done
