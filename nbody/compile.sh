#!/bin/bash
source ../sourceloc.sh

g++ nbody.cpp -g -O0 -o nbody

g++ nbody_splitted.cpp -g -O0 -o nbody_splitted

#g++ nbody_rambrain.cpp -g -O0 -o nbody_rambrain -std=c++11 -lrambrain -lrt

#g++ nbody_omp.cpp -g -O0 -o nbody_omp -std=c++11 -lrambrain -lrt -fopenmp

#g++ nbody_ompsecure.cpp -g -O0 -o nbody_ompsecure -std=c++11 -lrambrain -lrt -fopenmp
