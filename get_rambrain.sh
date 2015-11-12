#!/bin/bash
echo Your hostname is:
usexp=""
if hostname|grep cip-ws-; then
        echo "Custom config for CIP PC"
	usexp="-DUSE_XPRESSIVE=on"
        cat - >~/.rambrain.conf<<EOF
[default]
swapfiles=/tmp/rambrain-%d-%d
EOF
fi


echo "Cloning rambrain from github... "
git clone https://github.com/mimgrund/rambrain.git rambrain
echo "Done"


cd rambrain/build
echo "Building rambrain..."
cmake .. $usexp
make -j 8
echo "Done"
cd ..
sourcedir=$(pwd)/empty
libdir=$(pwd)/lib
mkdir empty
ln -s $(pwd)/src empty/rambrain
ln -s $(pwd)/lib/rambrainDefinitionsHeader.h src/
echo "Library directory is $libdir"

echo "Writing script to load lib path"
cd ..
echo "echo \"Changing library path...\"" > sourceloc.sh
echo "export LD_LIBRARY_PATH=$LD_LBRARY_PATH:$libdir" >> sourceloc.sh
echo "export LIBRARY_PATH=$LD_LBRARY_PATH:$libdir" >> sourceloc.sh
echo "export CPLUS_INCLUDE_PATH=$LD_LBRARY_PATH:$sourcedir" >> sourceloc.sh
echo 'echo "New libary path should contain rambrain library: $LD_LIBRARY_PATH"' >> sourceloc.sh

echo "sourceloc.sh written"
echo "Before compiling or running programs with rambrain please execute following command once in the respective shell:"
echo "source sourceloc.sh"

echo "Done, exiting"
