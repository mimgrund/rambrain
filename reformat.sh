#!/bin/bash

astyle --style=kr --attach-inlines --indent=spaces=4 --suffix=none *.h
astyle --style=kr --attach-inlines --indent=spaces=4 --suffix=none *.cpp
