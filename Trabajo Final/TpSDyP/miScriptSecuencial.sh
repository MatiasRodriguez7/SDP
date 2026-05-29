#!/bin/bash

#SBATCH -N 1

#SBATCH --exclusive

#SBATCH -o ./output18reinas_secuencial.txt

#SBATCH -e ./errors18reinas_secuencial.txt

#SBATCH --time=00:10:00 #Tiempo límite (HH:MM:SS)

./nreinas 18
