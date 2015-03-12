#!/bin/sh

echo "git diff:"
diff="$(git diff --cached --name-only)"

if [ -z "$diff" ];
then
  echo "No diff found"
else
    echo "Files to format via astyle:"
    echo "$diff"
    astyle --style=kr --attach-inlines --indent=spaces=4 --suffix=none "$diff"
fi
