#!/bin/bash

TestRepetitions=10

RunMatrixTranspose=1
#MatrixTransposeSize=(1000 2000 3000 4000 5000)
#MatrixTransposeMemory=(10 10 10 10 10)
declare -a MatrixTransposeSize
declare -a MatrixTransposeMemory
for i in `seq 0 30`;
do
  MatrixTransposeSize[$i]=$(echo "" | awk "END {print 10.0 ^ ($i * 4.0 / 30.0 + 1.0)}")
  MatrixTransposeSize[$i]=${MatrixTransposeSize[$i]%.*}
  MatrixTransposeMemory[$i]=10
done
