#!/bin/bash

source ./scripts/performance-test-config.sh

touch temp.gnuplot
touch temp.dat

if [[ $RunMatrixTranspose -eq 1 ]] ;
then
  countRuns=${#MatrixTransposeSize[@]}
  
  echo "Running ${countRuns} different matrix transposes ${TestRepetitions} times"
  rm temp.dat
  
  for i in `seq 0 $((countRuns-1))`;
  do
    echo "Run $((i+1)) ..."
    echo "./bin/membrain-performancetests ${TestRepetitions} MatrixTranspose ${MatrixTransposeSize[$i]} ${MatrixTransposeMemory[$i]}"
    
    ./bin/membrain-performancetests ${TestRepetitions} MatrixTranspose ${MatrixTransposeSize[$i]} ${MatrixTransposeMemory[$i]}
    
    echo -n "${MatrixTransposeSize[$i]} ${MatrixTransposeMemory[$i]} " >> temp.dat
    while read line
    do
      if [[ ! "${line}" == \#* ]] ;
      then
	IFS='	' read -a array <<< "$line"
	
	arrLength=countRuns=${#array[@]}
	i=$((arrLength-2))
	echo -n "${array[$i]} " >> temp.dat
      fi
    done < "perftest_MatrixTranspose#${MatrixTransposeSize[$i]}#${MatrixTransposeMemory[$i]}"
    echo >> temp.dat

  done
  
  rm temp.gnuplot
  echo "set terminal png" >> temp.gnuplot
  echo "set output \"MatrixTranspose.png\"" >> temp.gnuplot
  echo "set xlabel \"Matrix size per dimension\"" >> temp.gnuplot
  echo "set ylabel \"Execution time [ms]\"" >> temp.gnuplot
  echo "set title \"Matrix transpose\"" >> temp.gnuplot
  echo "set log xy" >> temp.gnuplot
  echo "plot 'temp.dat' using 1:3 with lines title \"Allocation & Definition\", \\" >> temp.gnuplot
  echo "'temp.dat' using 1:4 with lines title \"Transposition\", \\" >> temp.gnuplot
  echo "'temp.dat' using 1:5 with lines title \"Deletion\", \\" >> temp.gnuplot
  echo "'temp.dat' using 1:(\$3+\$4+\$5) with lines title \"Total\"" >> temp.gnuplot
  gnuplot temp.gnuplot
  display MatrixTranspose.png
fi
