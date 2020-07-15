#!/bin/bash

echo "./run.sh unique_shadow_int_value afl_mode (e.g., ./run.sh 3 10)"
echo "- afl_mode (1: normal mode, 10: dynamic_switch mode)"

mkdir -p ./fuzz/objdump/output
rm -rf ./fuzz/objdump/output/*

sudo sh -c 'echo core | sudo tee /proc/sys/kernel/core_pattern'
sudo sh -c 'cd /sys/devices/system/cpu; echo performance | sudo tee cpu*/cpufreq/scaling_governor; cd -'
sudo sh -c 'echo 1 | sudo tee /proc/sys/kernel/sched_child_runs_first'
sudo sysctl -w kernel.randomize_va_space=0

export CHECK_NUM="1"
export SHM_STR="."
export SHM_INT=$1

prefix_path="$(pwd)/"

export SAMPLE_PATH="$(pwd)/binutils/objdump"
export ASAN_PATH="$(pwd)/../build_asan/binutils/objdump"
export MINSHADOW_PATH="$(pwd)/../build_min/binutils/objdump"
export RBTREE_PATH="$(pwd)/../build_rbtree/binutils/objdump"

../../../afl/afl-2.52b_$2/afl-fuzz -t 1000 -i ../full-seeds/objdump -o ./fuzz/objdump/output  -m none -- ./binutils/objdump -a @@
