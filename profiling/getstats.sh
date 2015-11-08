#!/bin/bash

echo -e "swap_out_scheduled\ŧswap_out\tswap_out - swap_out_last\tswap_in_scheduled\tswap_in\tswap_in - swap_in_last\thits / miss\tmemory_used\tmemory_used / memory_max\tswap_used\tswap_used / swap_max"
killall -s USR1 endless

echo -e "free_space\tfree_space at end\tfree_space fragmented\tfree_space / (sum of the three)\t(swap_max - sum of the three) / swap_max\tsanity check"
killall -s USR2 endless
