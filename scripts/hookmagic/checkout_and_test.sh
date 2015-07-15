#!/bin/bash
cd /membrain_compile
nohup /membrain_compile/checkout_and_mail.sh $1 $2 &>/dev/null &
exit 0
