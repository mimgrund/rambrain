gitdiff=$(git diff|xxd -i -)
if [ "$gitdiff" ]
then 
gitdiff=$gitdiff
else
gitdiff="0x00"
fi
cat - >src/git_info.h <<EOF
const unsigned char gitCommit [] = {$(git log -1|head -1|xxd -i -),0x00};
const unsigned char gitDiff [] = {$gitdiff, 0x00};
EOF
