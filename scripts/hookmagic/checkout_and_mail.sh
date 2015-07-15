#!/bin/bash
/membrain_compile/autotest.sh $1 > $1.txt
cat $1.txt| mailx -a $1.log -s "autotest of $1" $2
