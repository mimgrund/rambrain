#!/bin/bash

red='\033[0;31m'
green='\033[0;32m'
nocolor='\033[0m'

echo "Running all tests wrapped in valgrind..."
echo ""

valgrind --leak-check=full --log-file=.valgrind bin/membrain-tests
noleak=$(grep -c "All heap blocks were freed -- no leaks are possible" .valgrind)

if [ $noleak -eq 1 ]
then
  echo ""
  echo -e "${green}No memory leaks found!${nocolor}"
else
  >&2 echo ""
  >&2 echo -e "${red}There were memory leaks, outputting valgrind result:${nocolor}"
  >&2 cat .valgrind
fi

rm .valgrind
