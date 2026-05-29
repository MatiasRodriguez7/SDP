#!/bin/bash

#SBATCH -N 2

#SBATCH --exclusive

#SBATCH --tasks-per-node=1

#SBATCH -o ./output18reinas_2hilosporNodos.txt

#SBATCH -e ./errors18reinas_2hilosporNodos.txt
#SBATCH --time=00:08:00 #Tiempo límite (HH:MM:SS)

mpirun --bind-to none nreinas_hibrido 18 2
