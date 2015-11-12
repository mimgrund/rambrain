#!/bin/bash
source ../sourceloc.sh

if g++ classes.cpp -g -O0 -o classes -std=c++11; then
	echo Compilation of classical code successful
else
	echo Compilation of classical code failed
	exit 1
fi

if g++ classes_rambrain.cpp -g -O0 -o classes_rambrain -std=c++11 -lrambrain -lrt; then
        echo Compilation of rambrainified code successful
else
        echo Compilation of rambrainified code failed
        exit 1
fi

