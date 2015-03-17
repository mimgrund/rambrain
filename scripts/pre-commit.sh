#!/bin/sh
pwd
echo "git diff:"
diff="$(git diff --cached --name-only -- '*.cpp' '*.c' '*.inl' '*.hpp' '*.h')"

if [ -z "$diff" ];
then
  echo "No diff found"
else
    echo "Formatting files:"
    for file in $diff; do
	    echo $file
	    git show :$file | astyle --style=kr --attach-inlines --indent=spaces=4 --suffix=none --pad-paren >.beautyformatter
	    mv $file .beautyback
            mv .beautyformatter $file
	    git add $file
            mv .beautyback $file
    done
fi
