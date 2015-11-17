#!/bin/bash
source ../sourceloc.sh

g++ simple.cpp -g -O0 -o simple

g++ simple_rambrain_solution.cpp -g -O0 -o simple_rambrain_solution -std=c++11 -lrambrain -lrt

