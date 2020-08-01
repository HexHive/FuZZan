#!/bin/bash

make clean
make -j

sudo sh -c 'echo core | sudo tee /proc/sys/kernel/core_pattern'
sudo sh -c 'cd /sys/devices/system/cpu; echo performance | sudo tee cpu*/cpufreq/scaling_governor; cd -'
sudo sh -c 'echo 1 | sudo tee /proc/sys/kernel/sched_child_runs_first'
sudo sysctl -w kernel.randomize_va_space=0

export ASAN_OPTIONS=detect_leaks=0

export FUZZAN_MODE=4
: ${PATHROOT:="$PWD"}
echo $PATHROOT

../../LLVM/build/bin/clang++ ctor.cpp -o test -mcmodel=small -fsanitize=address -g -O2
