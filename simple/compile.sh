#!/bin/bash
source ../sourceloc.sh

g++ simple.cpp -g -O0 -o simple

g++ simple_rambrain.cpp -g -O0 -o simple_rambrain -std=c++11 -lrambrain -lrt

