#!/bin/bash

watch -p -n 1 killall "$1" -s SIGUSR1
