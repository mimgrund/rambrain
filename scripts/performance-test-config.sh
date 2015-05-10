#!/bin/bash

TestRepetitions=10


RunMatrixTranspose1=1
#MatrixTransposeSize1=(1000 2000 3000 4000 5000)
#MatrixTransposeMemory=(10 10 10 10 10)
declare -a MatrixTransposeSize1
declare -a MatrixTransposeMemory1
for i in `seq 0 30`;
do
  MatrixTransposeSize1[$i]=$(echo "" | awk "END {print 10.0 ^ ($i * 4.0 / 30.0 + 1.0)}")
  MatrixTransposeSize1[$i]=${MatrixTransposeSize1[$i]%.*}
  MatrixTransposeMemory1[$i]=10
done


RunMatrixTranspose2=1
declare -a MatrixTransposeSize2
declare -a MatrixTransposeMemory2
for i in `seq 0 30`;
do
  MatrixTransposeSize2[$i]=8000
  MatrixTransposeMemory2[$i]=$(echo "" | awk "END {print 10.0 ^ ($i * 3.0 / 30.0 + 1.0)}")
  MatrixTransposeMemory2[$i]=${MatrixTransposeMemory2[$i]%.*}
done
