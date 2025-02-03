#!/bin/bash
set -e
# /workspaces/CMU15213/run_cache.sh

cd '/workspaces/CMU15213/Cache Lab/cachelab-handout'
make clean
make -j
# ./csim -s 4 -E 1 -b 4 -t traces/yi.trace
# ./csim -s 1 -E 1 -b 1 -t traces/yi2.trace
# ./test-csim
./driver.py