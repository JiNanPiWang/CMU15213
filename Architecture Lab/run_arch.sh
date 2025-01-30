#!/bin/bash
set -e
# /workspaces/CMU15213/run_arch.sh

# cd '/workspaces/CMU15213/Architecture Lab/archlab-handout/sim/misc'
# ./yas sum.ys
# ./yis sum.yo
# ./yas rsum.ys
# ./yis rsum.yo
# ./yas copy.ys
# ./yis copy.yo




# cd '/workspaces/CMU15213/Architecture Lab/archlab-handout/sim/seq'
# make clean
# make VERSION=full
# cd ../y86-code
# make clean
# make
# cd ../seq
# ./ssim -t ../y86-code/asumi.yo

# cd ../y86-code
# make testssim

# cd ../ptest
# make SIM=../seq/ssim

# cd ../ptest
# make SIM=../seq/ssim TFLAGS=-i




cd '/workspaces/CMU15213/Architecture Lab/archlab-handout/sim/misc'
make clean
make
cd '/workspaces/CMU15213/Architecture Lab/archlab-handout/sim/pipe'
make clean
make drivers
make psim VERSION=full
# ./check-len.pl < ncopy.yo

sleep_time=1

echo -e "\n\nrunning: ./check-len.pl < ncopy.yo"
sleep $sleep_time
../misc/yas ncopy.ys
./check-len.pl < ncopy.yo

# echo -e "\n\nrunning: ./psim -t sdriver.yo"
# sleep $sleep_time
# ./psim -t sdriver.yo

# echo -e "\n\nrunning: ./psim -t ldriver.yo"
# sleep $sleep_time
# ./psim -t ldriver.yo

# echo -e "\n\nrunning: ../misc/yis sdriver.yo"
# sleep $sleep_time
# ../misc/yis sdriver.yo

# echo -e "\n\nrunning: ../misc/yis ldriver.yo"
# sleep $sleep_time
# ../misc/yis ldriver.yo

echo -e "\n\nrunning: ./correctness.pl"
sleep $sleep_time
./correctness.pl

echo -e "\n\nrunning: ./benchmark.pl"
sleep $sleep_time
./benchmark.pl

# 验证修改的 pipe-full.hcl
# cd ../y86-code
# make clean
# echo -e "\n\nrunning: make testpsim"
# sleep $sleep_time
# make testpsim

# cd ../ptest
# echo -e "\n\nrunning: make SIM=../pipe/psim TFLAGS=-i"
# sleep $sleep_time
# make SIM=../pipe/psim TFLAGS=-i