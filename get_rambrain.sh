#!/bin/bash

echo "Cloning rambrain from github... "
git clone https://github.com/mimgrund/rambrain.git rambrain
echo "Done"

cd rambrain/build
echo "Building rambrain..."
cmake ..
make -j 8
echo "Done"

cd ../lib
libdir=$(pwd)
echo "Library directory is $libdir"

echo "Writing script to load lib path"
cd ../..
echo "echo \"Changing library path...\"" > sourceloc.sh
echo "export LD_LIBRARY_PATH=$LD_LBRARY_PATH:$libdir" >> sourceloc.sh
echo "echo \"New libary path should contain rambrain library: $LD_LIBRARY_PATH\"" >> sourceloc.sh

echo "sourceloc.sh written"
echo "Before compiling or running programs with rambrain please execute following command once in the respective shell:"
echo "source sourceloc.sh"

echo "Done, exiting"
