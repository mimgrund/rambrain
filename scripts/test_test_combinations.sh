#/bin/sh

alltests=$(../bin/rambrain-tests --gtest_list_tests|grep -v "\."|cut -d " " -f3)
for f in $alltests; do
	for g in $alltests; do
		#repeat because gtest uses same sequence of tests always. then we get all combinations.
		../bin/rambrain-tests --gtest_repeat=2 --gtest_filter=*$g*:*$f*
		retval=$?
		if [ $retval -ne 0 ]; then
		echo $f $g>>failing
		fi

	done
done
