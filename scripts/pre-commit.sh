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
	    cat $file | astyle --style=kr --attach-inlines --indent=spaces=4 --suffix=none --pad-paren --pad-oper --pad-header --align-pointer=name --align-reference=name --add-brackets >.beautyback
	    git show :$file | astyle --style=kr --attach-inlines --indent=spaces=4 --suffix=none --pad-paren --pad-oper --pad-header --align-pointer=name --align-reference=name --add-brackets >.beautyformatter
            mv .beautyformatter $file
	    git add $file
            mv .beautyback $file
    done
fi
