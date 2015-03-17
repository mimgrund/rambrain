#!/bin/bash
pwd
astyle --style=kr --attach-inlines --indent=spaces=4 --pad-paren --suffix=none *.h
astyle --style=kr --attach-inlines --indent=spaces=4 --pad-paren --suffix=none *.cpp
astyle --style=kr --attach-inlines --indent=spaces=4 --pad-paren --suffix=none *.c
astyle --style=kr --attach-inlines --indent=spaces=4 --pad-paren --suffix=none *.inl
astyle --style=kr --attach-inlines --indent=spaces=4 --pad-paren --suffix=none *.hpp
