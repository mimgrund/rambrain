#!/bin/bash

watch -p -n 1E-1 killall "$1" -s SIGUSR1
