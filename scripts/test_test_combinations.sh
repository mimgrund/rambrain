#/bin/sh

alltests=$(../bin/rambrain-tests --gtest_list_tests|grep -v "\."|cut -d " " -f3)
for f in $alltests; do
	for g in $alltests; do
		../bin/rambrain-tests --gtest_filter=*$g*:*$f*
		retval=$?
		if [ $retval -ne 0 ]; then
		echo $f $g>>failing
		fi

	done
done
