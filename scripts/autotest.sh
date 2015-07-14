#!/bin/bash

user=$1
repodir="ssh://${user}@schedar.usm.uni-muenchen.de/var/gits/membrain"
branches=(master aio_merger)
options=(PARENTAL_CONTROL) #what about dma? what about optimisation?
processes=8

rm -rf cleanbrain
git clone "$repodir" cleanbrain
cd cleanbrain/build

for branch in ${branches[@]}; do
    echo "Checking out branch $branch"
    git checkout "$branch"

    max=$(awk "BEGIN{print 2 ** ${#options[@]}}")
    for i in `seq 1 $max`; do
        opts=""
        j=1
        for option in ${options[@]}; do
            opts+=" -D$option="
            k=$(($i / $j % 2))
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

        rm -rf *
        cmake .. $opts >> "${outname}" 2>&1
        echo -e "\n\n" >> "$outname"
        make -j 8 >> "${outname}" 2>&1

        fail=$?
        if [ $fail -ne 0 ]; then
            >&2 echo "Make exited with error code ${fail}, ATTENTION NEEDED!"
        else
            echo "Running tests..."
            echo -e "\n\n" >> "$outname"
            ../bin/membrain-tests >> "${outname}" 2>&1

            fail=$?
            if [ $fail -ne 0 ]; then
                >&2 echo "${fail} tests failed in this run, ATTENTION NEEDED!"
            fi
        fi
    done
done
