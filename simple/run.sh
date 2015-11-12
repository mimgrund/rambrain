#!/bin/bash

echo Compiling both programs

if ./compile.sh; then
	echo Compilation succeeded
else
	echo will not run since compilation failed.
fi

./simple > temp.txt

gnuplot - <<EOF
set terminal png
set output "out.png"
plot 'temp.txt' matrix w image
EOF
