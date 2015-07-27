#!/bin/bash

vars=$(echo $1 | tr ";" "\n")
file="rambrainDefinitionsHeader.h"
wd=$(pwd)

echo "Creating definitions header file $file in $wd"

touch $file
rm $file

for var in $vars
do
	echo "Found definition: $var"
	
	if [[ "$var" == __GIT_VERSION=* ]] ;
	then
		echo "Skipping"
	else
		echo "#define $var" >> $file
	fi
done

echo "Done"
