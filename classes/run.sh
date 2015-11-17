#!/bin/bash

source ../sourceloc.sh

echo Compiling both programs

if ./compile.sh; then
	echo Compilation succeeded
else
	echo will not run since compilation failed.
fi

./classes > classical.txt
./classes_rambrain > rambrain.txt

gnuplot - <<EOF
set terminal png size 1000,400
set output "out.png"
set multiplot layout 1,2
plot 'classical.txt' matrix w image
plot 'rambrain.txt' matrix w image
EOF
