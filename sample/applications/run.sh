#!/bin/bash

if [ "$#" -ne 3 ]; then
  echo "./run.sh targe_program unique_shadow_int_value afl_mode (e.g., ./run.sh objdump 3 2)"
  echo "- afl_mode (1: normal mode, 2: dynamic_switch mode)"
  echo "please copy this script into target testing folder (e.g., cp ./run.sh /build_asan)"
  exit 1
fi

mkdir -p ./fuzz/$1/output
rm -rf ./fuzz/$1/output/*

sudo sh -c 'echo core | sudo tee /proc/sys/kernel/core_pattern'
sudo sh -c 'cd /sys/devices/system/cpu; echo performance | sudo tee cpu*/cpufreq/scaling_governor; cd -'
sudo sh -c 'echo 1 | sudo tee /proc/sys/kernel/sched_child_runs_first'
sudo sysctl -w kernel.randomize_va_space=0

export CHECK_NUM="1"
export SHM_STR="."
export SHM_INT=$2

prefix_path="$(pwd)/"

export SAMPLE_PATH="$(pwd)/binutils/prelink-$1/$1"
export ASAN_PATH="$(pwd)/../build_asan_opt/binutils/$1"
export MINSHADOW_PATH="$(pwd)/../build_min-1G/binutils/prelink-$1/$1"
export MINSHADOW_PATH_4G="$(pwd)/../build_min-4G/binutils/prelink-$1/$1"
export MINSHADOW_PATH_8G="$(pwd)/../build_min-8G/binutils/prelink-$1/$1"
export MINSHADOW_PATH_16G="$(pwd)/../build_min-16G/binutils/prelink-$1/$1"
export RBTREE_PATH="$(pwd)/../build_rbtree/binutils/$1"

export MINMODE_ON="0"
export MIN_SCRIPT_PATH="/home/jeon41/FuZZan/etc/libshrink/"

if [ $1 == "tcpdump" ]
then
  export ASAN_PATH="$(pwd)/../build_asan_opt/$1/$1"
  export RBTREE_PATH="$(pwd)/../build_rbtree/$1/$1"
  export SAMPLE_PATH="$(pwd)/$1/prelink-$1/$1"
  export MINSHADOW_PATH="$(pwd)/../build_min-1G/$1/prelink-$1/$1"
  export MINSHADOW_PATH_4G="$(pwd)/../build_min-4G/$1/prelink-$1/$1"
  export MINSHADOW_PATH_8G="$(pwd)/../build_min-8G/$1/prelink-$1/$1"
  export MINSHADOW_PATH_16G="$(pwd)/../build_min-16G/$1/prelink-$1/$1"
if [ $3 == 2 ]; then
  export MINMODE_ON="1"
  ../../../afl/afl-2.52b/afl-fuzz -t 1000 -i ../full-seeds/$1 -o ./fuzz/$1/output -m none -- ./$1/prelink-$1/$1 -n -e -r  @@
else
  export MINMODE_ON="0"
  ../../../afl/afl-2.52b-native/afl-fuzz -t 1000 -i ../full-seeds/$1 -o ./fuzz/$1/output -m none -- ./$1/$1 -n -e -r  @@
fi

elif [ $1 == "gif2png" ]
then
  export ASAN_PATH="$(pwd)/../build_asan_opt/gif2png/$1"
  export RBTREE_PATH="$(pwd)/../build_rbtree/gif2png/$1"
  export SAMPLE_PATH="$(pwd)/gif2png/prelink-$1/$1"
  export MINSHADOW_PATH="$(pwd)/../build_min-1G/gif2png/prelink-$1/$1"
  export MINSHADOW_PATH_4G="$(pwd)/../build_min-4G/gif2png/prelink-$1/$1"
  export MINSHADOW_PATH_8G="$(pwd)/../build_min-8G/gif2png/prelink-$1/$1"
  export MINSHADOW_PATH_16G="$(pwd)/../build_min-16G/gif2png/prelink-$1/$1"
if [ $3 == 2 ]; then
  export MINMODE_ON="1"
  ../../../afl/afl-2.52b/afl-fuzz -t 1000 -i ../full-seeds/$1 -o ./fuzz/$1/output -m none -- ./gif2png/prelink-$1/$1  @@
else
  export MINMODE_ON="0"
  ../../../afl/afl-2.52b-native/afl-fuzz -t 1000 -i ../full-seeds/$1 -o ./fuzz/$1/output -m none -- ./gif2png/$1  @@
fi

elif [ $1 == "pngfix" ]
then
  export ASAN_PATH="$(pwd)/../build_asan_opt/libpng/build/bin/$1"
  export RBTREE_PATH="$(pwd)/../build_rbtree/libpng/build/bin/$1"
  export SAMPLE_PATH="$(pwd)/libpng/prelink-pngfix/$1"
  export MINSHADOW_PATH="$(pwd)/../build_min-1G/libpng/prelink-pngfix/$1"
  export MINSHADOW_PATH_4G="$(pwd)/../build_min-4G/libpng/prelink-pngfix/$1"
  export MINSHADOW_PATH_8G="$(pwd)/../build_min-8G/libpng/prelink-pngfix/$1"
  export MINSHADOW_PATH_16G="$(pwd)/../build_min-16G/libpng/prelink-pngfix/$1"
if [ $3 == 2 ]; then
  export MINMODE_ON="1"
  ../../../afl/afl-2.52b/afl-fuzz -t 1000 -i ../full-seeds/$1 -o ./fuzz/$1/output -m none -- ./libpng/prelink-$1/$1  @@
else
  export MINMODE_ON="0"
  ../../../afl/afl-2.52b-native/afl-fuzz -t 1000 -i ../full-seeds/$1 -o ./fuzz/$1/output -m none -- ./libpng/build/bin/$1  @@
fi

elif [ $1 == "ffmpeg" ]
then
  export ASAN_PATH="$(pwd)/../build_asan_opt/ffmpeg/ffmpeg_AV_CODEC_ID_MPEG4_fuzzer"
  export RBTREE_PATH="$(pwd)/../build_rbtree/ffmpeg/ffmpeg_AV_CODEC_ID_MPEG4_fuzzer"
  export SAMPLE_PATH="$(pwd)/ffmpeg/ffmpeg_AV_CODEC_ID_MPEG4_fuzzer"
  export MINSHADOW_PATH="$(pwd)/../build_min-1G/ffmpeg/ffmpeg_AV_CODEC_ID_MPEG4_fuzzer"
  export MINSHADOW_PATH_4G="$(pwd)/../build_min-4G/ffmpeg/ffmpeg_AV_CODEC_ID_MPEG4_fuzzer"
  export MINSHADOW_PATH_8G="$(pwd)/../build_min-8G/ffmpeg/ffmpeg_AV_CODEC_ID_MPEG4_fuzzer"
  export MINSHADOW_PATH_16G="$(pwd)/../build_min-16G/ffmpeg/ffmpeg_AV_CODEC_ID_MPEG4_fuzzer"
if [ $3 == 2 ]; then
  export MINMODE_ON="1"
  ../../../afl/afl-2.52b/afl-fuzz -t 5000 -i ../full-seeds/$1 -o ./fuzz/$1/output -m none -- ./ffmpeg/ffmpeg_AV_CODEC_ID_MPEG4_fuzzer @@
else
  export MINMODE_ON="0"
  ../../../afl/afl-2.52b-native/afl-fuzz -t 5000 -i ../full-seeds/$1 -o ./fuzz/$1/output -m none -- ./ffmpeg/ffmpeg_AV_CODEC_ID_MPEG4_fuzzer @@
fi

elif [ $1 == "file" ]
then
  export ASAN_PATH="$(pwd)/../build_asan_opt/file/$1"
  export RBTREE_PATH="$(pwd)/../build_rbtree/file/$1"
  export SAMPLE_PATH="$(pwd)/$1/prelink-$1/$1"
  export MINSHADOW_PATH="$(pwd)/../build_min-1G/$1/prelink-$1/$1"
  export MINSHADOW_PATH_4G="$(pwd)/../build_min-4G/$1/prelink-$1/$1"
  export MINSHADOW_PATH_8G="$(pwd)/../build_min-8G/$1/prelink-$1/$1"
  export MINSHADOW_PATH_16G="$(pwd)/../build_min-16G/$1/prelink-$1/$1"
if [ $3 == 2 ]; then
  export MINMODE_ON="1"
  ../../../afl/afl-2.52b/afl-fuzz -t 1000 -i ../full-seeds/$1 -o ./fuzz/$1/output -m none -- ./$1/prelink-$1/$1 -m ./$1/magic/magic.mgc @@
else
  export MINMODE_ON="0"
  ../../../afl/afl-2.52b-native/afl-fuzz -t 1000 -i ../full-seeds/$1 -o ./fuzz/$1/output -m none -- ./$1/file -m ./$1/magic/magic.mgc @@
fi
elif [ $1 == "objdump" ]
then
if [ $3 == 2 ]; then
  export MINMODE_ON="1"
  ../../../afl/afl-2.52b/afl-fuzz -t 1000 -i ../full-seeds/$1 -o ./fuzz/$1/output -m none -- ./binutils/prelink-$1/$1 -a @@
else
  export MINMODE_ON="0"
  ../../../afl/afl-2.52b-native/afl-fuzz -t 1000 -i ../full-seeds/$1 -o ./fuzz/$1/output -m none -- ./binutils/$1 -a @@
fi
elif [ $1 == "c++filt" ]
then
if [ $3 == 2 ]; then
  export MINMODE_ON="1"
  ../../../afl/afl-2.52b/afl-fuzz -t 1000 -i ../full-seeds/$1 -o ./fuzz/$1/output -m none -- ./binutils/prelink-$1/$1 -n
else
  export MINMODE_ON="0"
  ../../../afl/afl-2.52b-native/afl-fuzz -t 1000 -i ../full-seeds/$1 -o ./fuzz/$1/output -m none -- ./binutils/$1 -n
fi
else
if [ $3 == 2 ]; then
  export MINMODE_ON="1"
  ../../../afl/afl-2.52b/afl-fuzz -t 1000 -i ../full-seeds/$1 -o ./fuzz/$1/output -m none -- ./binutils/prelink-$1/$1 @@
else
  export MINMODE_ON="0"
  ../../../afl/afl-2.52b-native/afl-fuzz -t 1000 -i ../full-seeds/$1 -o ./fuzz/$1/output -m none -- ./binutils/$1 @@
fi
fi
