#!/bin/bash

source ./scripts/performance-test-config.sh

if [[ $RunMatrixTranspose -eq 1 ]] ;
then
  countRuns=${#MatrixTransposeSize[@]}
  
  echo "Running ${countRuns} different matrix transposes ${TestRepetitions} times"
  
  for i in `seq 0 $((countRuns-1))`;
  do
    echo "Run $((i+1)) ..."
    echo "./bin/membrain-performancetests ${TestRepetitions} MatrixTranspose ${MatrixTransposeSize[$i]} ${MatrixTransposeMemory[$i]}"
    
    ./bin/membrain-performancetests ${TestRepetitions} MatrixTranspose ${MatrixTransposeSize[$i]} ${MatrixTransposeMemory[$i]}
  done
fi
