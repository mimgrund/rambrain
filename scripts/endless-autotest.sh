#!/bin/bash

user=$1
repodir="ssh://${user}@schedar.usm.uni-muenchen.de/var/gits/membrain"
branches=(master)
options=(PARENTAL_CONTROL OPTIMISE_COMPILATION)
processes=$2
stopfile="STOP"
pwd=$(pwd)

if [ -z "$user" ]; then
    echo "Please supply a username via command line parameter, aborting script..."
    exit 1
fi

if [ -z "$processes" ]; then
    processes=1
fi

rm -rf "$stopfile"


iteration=0
totalfails=0

while [ ! -f "$stopfile" ]; do

    ((iteration++))
    echo "Iteration $iteration; Please touch file $stopfile in folder $pwd to stop the endless autotest\n"

    cd $pwd
    rm -rf cleanbrain
    rm -rf "SUCCESS_*.out"
    git clone $repodir cleanbrain

    fail=$?
    if [ $fail -ne 0 ]; then
	>&2 echo "Git failed to clone repository, aborting script..."
	exit 2
    fi

    cd cleanbrain/build

    for branch in ${branches[@]}; do
	echo "Checking out branch $branch"
	git checkout $branch
	#patch CMakeLists to include clang (not recommended in normal use)
	cat - ../CMakeLists.txt >../CMakeLists.txt_new<<EOF
option (USE_CLANG "Use Clang Compiler" OFF)

if(USE_CLANG)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

endif()
EOF
	mv ../CMakeLists.txt_new ../CMakeLists.txt

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

	    rm -rf *
	    cmake .. $opts >> "${outname}" 2>&1
	    echo -e "\n\n" >> "$outname"
	    make -j $processes >> "${outname}" 2>&1

	    fail=$?
	    if [ $fail -ne 0 ]; then
		>&2 echo "Make exited with error code ${fail}, ATTENTION NEEDED!"
		((totalfails++))
		mv "$outname" "FAIL_${totalfails}_${outname}"
	    else
		echo "Running tests..."
		echo -e "\n\n" >> "$outname"
		timeout --kill-after=10 300 ../bin/rambrain-tests >> "${outname}" 2>&1

		fail=$?
		if [ $fail -ne 0 ]; then
		    >&2 echo "${fail} tests failed in this run, ATTENTION NEEDED!"
		    ((totalfails++))
		    mv "$outname" "FAIL_${totalfails}_${outname}"
		fi
	    fi
	    
	    echo "Test successfull"
	    mv "$outname" "SUCCESS_${totalfails}_${outname}"
	done
    done
done

echo "Endless autotest stopped after iteration $iteration - Total fails encountered: $totalfails"
