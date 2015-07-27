#!/bin/bash
cd /rambrain_compile
nohup /rambrain_compile/checkout_and_mail.sh $1 $2 &>/dev/null &
exit 0
