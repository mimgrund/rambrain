#!/bin/bash
source ../sourceloc.sh

g++ endless.cpp -g -O0 -o endless -std=c++11 -lrambrain -lrt
