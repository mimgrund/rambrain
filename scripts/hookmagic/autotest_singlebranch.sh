#!/bin/bash

repodir="/var/gits/membrain"
branch=$1
options=(PARENTAL_CONTROL OPTIMISE_COMPILATION)
processes=8
cd /rambrain_compile
git clone $repodir $branch
fail=$?
if [ $fail -ne 0 ]; then
    >&2 echo "Git failed to clone repository, aborting script..."
    exit 2
fi
unset GIT_DIR
cd $branch
    echo "Checking out branch $branch"
    git checkout $branch
    echo "This autotest is based on:"
    git show -s --pretty=oneline --format="%aD: %aN - %s" $branch
    echo -e "\n\n"
    chmod -R 0770 .
    chown -R $(whoami):gituser .

#override gengit.sh script
cat - >scripts/gengit.sh<<EOF
echo const unsigned char gitCommit [] = {0x00}\;>/rambrain_compile/$branch/src/git_info.h
echo const unsigned char gitDiff [] = {0x00}\;>>/rambrain_compile/$branch/src/git_info.h
EOF

good=0
    cd build


    max=$(awk "BEGIN{print 2 ** ${#options[@]}}")
    max=$((max-1))
    for i in `seq 0 $max`; do
        opts=""
        j=0
        for option in ${options[@]}; do
            opts+=" -D$option="
            k=$((($i >> $j) % 2))
            if [ $k -eq 1 ]; then
                opts+="ON"
            else
                opts+="OFF"
            fi
            ((j++))
        done
        echo "Building with options: $opts"

        outname="../../test_${branch}_${i}.out"
        echo "Saving results to $outname"
        echo -e "Branch $branch; options $opts\n\n" > "$outname"

        rm -rf ../build/*
        cmake .. $opts >> "${outname}" 2>&1
        echo -e "\n\n" >> "$outname"

        make -j $processes >> "${outname}" 2>&1

        fail=$?
        if [ $fail -ne 0 ]; then
            echo "Make exited with error code ${fail}, ATTENTION NEEDED!"
            >&2 grep FAILED ${outname}
        else
            echo "Running tests..."
            echo -e "\n\n" >> "$outname"
            timeout --kill-after=10 300 ../bin/rambrain-tests >> "${outname}" 2>&1

            fail=$?
            if [ $fail -ne 0 ]; then
                echo "${fail} tests failed in this run, ATTENTION NEEDED!"
                >&2 grep FAILED ${outname}
            else
		echo "test succeeded."
		((good++))
	    fi 
        fi
	echo -e "\n-------------------------------------------------\n\n\n\n\n\n" >>"$outname"
        
    done
cd /rambrain_compile
if [ $good -eq $(($max+1)) ]; then
	echo SUCESSFULL >${branch}_success.txt
else
	echo FAIL! $(($max+1-$good)) / $(($max+1)) need attention >${branch}_success.txt
fi
rm -rf $(whoami)
mv $branch $(whoami)
cat test_$branch* >$branch.log
