#!/bin/bash

source ./scripts/performance-test-config.sh

touch temp.gnuplot
touch temp.dat


if [[ $RunMatrixTranspose1 -eq 1 ]] ;
then
  countRuns=${#MatrixTransposeSize1[@]}
  
  echo "Running ${countRuns} different matrix transposes ${TestRepetitions} times"
  rm temp.dat
  
  for i in `seq 0 $((countRuns-1))`;
  do
    echo "Run $((i+1)) ..."
    echo "./bin/membrain-performancetests ${TestRepetitions} MatrixTranspose ${MatrixTransposeSize1[$i]} ${MatrixTransposeMemory1[$i]}"
    
    ./bin/membrain-performancetests ${TestRepetitions} MatrixTranspose ${MatrixTransposeSize1[$i]} ${MatrixTransposeMemory1[$i]}
    
    echo -n "${MatrixTransposeSize1[$i]} ${MatrixTransposeMemory1[$i]} " >> temp.dat
    while read line
    do
      if [[ ! "${line}" == \#* ]] ;
      then
	IFS='	' read -a array <<< "$line"
	
	arrLength=countRuns=${#array[@]}
	i=$((arrLength-2))
	echo -n "${array[$i]} " >> temp.dat
      fi
    done < "perftest_MatrixTranspose#${MatrixTransposeSize1[$i]}#${MatrixTransposeMemory1[$i]}"
    echo >> temp.dat

  done
  
  rm temp.gnuplot
  echo "set terminal postscript eps enhanced color 'Helvetica,10'" >> temp.gnuplot
  echo "set output \"MatrixTranspose1.eps\"" >> temp.gnuplot
  echo "set xlabel \"Matrix size per dimension\"" >> temp.gnuplot
  echo "set ylabel \"Execution time [ms]\"" >> temp.gnuplot
  echo "set title \"Matrix transpose\"" >> temp.gnuplot
  echo "set log xy" >> temp.gnuplot
  echo "plot 'temp.dat' using 1:3 with lines title \"Allocation & Definition\", \\" >> temp.gnuplot
  echo "'temp.dat' using 1:4 with lines title \"Transposition\", \\" >> temp.gnuplot
  echo "'temp.dat' using 1:5 with lines title \"Deletion\", \\" >> temp.gnuplot
  echo "'temp.dat' using 1:(\$3+\$4+\$5) with lines title \"Total\"" >> temp.gnuplot
  gnuplot temp.gnuplot
  convert -density 300 -resize 1920x MatrixTranspose1.eps -flatten MatrixTranspose1.png
  display MatrixTranspose1.png &
fi


if [[ $RunMatrixTranspose2 -eq 1 ]] ;
then
  countRuns=${#MatrixTransposeSize2[@]}
  
  echo "Running ${countRuns} different matrix transposes ${TestRepetitions} times"
  rm temp.dat
  
  for i in `seq 0 $((countRuns-1))`;
  do
    echo "Run $((i+1)) ..."
    echo "./bin/membrain-performancetests ${TestRepetitions} MatrixTranspose ${MatrixTransposeSize2[$i]} ${MatrixTransposeMemory2[$i]}"
    
    ./bin/membrain-performancetests ${TestRepetitions} MatrixTranspose ${MatrixTransposeSize2[$i]} ${MatrixTransposeMemory2[$i]}
    
    echo -n "${MatrixTransposeSize2[$i]} ${MatrixTransposeMemory2[$i]} " >> temp.dat
    while read line
    do
      if [[ ! "${line}" == \#* ]] ;
      then
	IFS='	' read -a array <<< "$line"
	
	arrLength=countRuns=${#array[@]}
	i=$((arrLength-2))
	echo -n "${array[$i]} " >> temp.dat
      fi
    done < "perftest_MatrixTranspose#${MatrixTransposeSize2[$i]}#${MatrixTransposeMemory2[$i]}"
    echo >> temp.dat

  done
  
  rm temp.gnuplot
  echo "set terminal postscript eps enhanced color 'Helvetica,10'" >> temp.gnuplot
  echo "set output \"MatrixTranspose2.eps\"" >> temp.gnuplot
  echo "set xlabel \"Matrix rows in main memory\"" >> temp.gnuplot
  echo "set ylabel \"Execution time [ms]\"" >> temp.gnuplot
  echo "set title \"Matrix transpose\"" >> temp.gnuplot
  echo "set log xy" >> temp.gnuplot
  echo "plot 'temp.dat' using 2:3 with lines title \"Allocation & Definition\", \\" >> temp.gnuplot
  echo "'temp.dat' using 2:4 with lines title \"Transposition\", \\" >> temp.gnuplot
  echo "'temp.dat' using 2:5 with lines title \"Deletion\", \\" >> temp.gnuplot
  echo "'temp.dat' using 2:(\$3+\$4+\$5) with lines title \"Total\"" >> temp.gnuplot
  gnuplot temp.gnuplot
  convert -density 300 -resize 1920x MatrixTranspose2.eps -flatten MatrixTranspose2.png
  display MatrixTranspose2.png &
fi


rm temp.gnuplot
rm temp.dat
