#!/bin/bash
/membrain_compile/autotest.sh $1 > $1.txt
stxt=$(cat $1_success.txt)
cat $1.txt| mailx -a $1.log -s "$stxt  -- autotest of $1" $2
