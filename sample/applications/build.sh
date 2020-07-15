#!/bin/bash

if [ "$#" -ne 1 ]; then
  echo "./build.sh target_application_name (e.g., ./build.sh libpng)"
  exit 1
fi

sudo sh -c 'echo core | sudo tee /proc/sys/kernel/core_pattern'
sudo sh -c 'cd /sys/devices/system/cpu; echo performance | sudo tee cpu*/cpufreq/scaling_governor; cd -'
sudo sh -c 'echo 1 | sudo tee /proc/sys/kernel/sched_child_runs_first'
sudo sysctl -w kernel.randomize_va_space=0

: ${PATHROOT:="$PWD"}
LLVM="$PATHROOT/../../LLVM/build/bin/"
ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
LLVM_DIR="${ROOT}/../../LLVM/build/bin"
LLVM_MAIN="$PATHROOT/../../LLVM/"
export LDFLAGS=""
export CFLAGS=""
export CXXFLAGS=""
ORG_PATH=$PATH

build_llvm () {
export LDFLAGS=""
export CFLAGS=""
export CXXFLAGS=""
export PATH=$ORG_PATH
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
cd $LLVM_MAIN
./build.sh $1
}

build_test_target () {
export PATH=$LLVM:$PATH
export CC=${ROOT}/../../afl/afl-2.52b/afl-clang-fast
export CXX=${ROOT}/../../afl/afl-2.52b/afl-clang-fast++
export AFL_PATH=${ROOT}/../../afl/afl-2.52b
export BUILD="shared"

[ ! -e ${ROOT}/testsuite/"$1" ] && echo "${1} is not a target app" && exit 1

mkdir -p ${ROOT}/$2/"$1"
pushd ${ROOT}/$2/"$1"

if [ $1 == "gif2png" ]; then
  cp ../../testsuite/"$1"/* .
  ./build.sh $3
fi
if [ $1 == "ffmpeg" ]; then
  ../../testsuite/"$1"/build.sh $3
else
../../testsuite/"$1"/build.sh $3
fi

popd
}

export SHM_STR="."
export SHM_INT="1"

###################
# Native mode build
###################
export FUZZAN_MODE=1
build_llvm 1

cd $PATHROOT
echo "build native"
export LDFLAGS="-lstdc++ -fuse-ld=bfd -U_FORTIFY_SOURCE -O2"
export CFLAGS="-lstdc++ -U_FORTIFY_SOURCE  -O2"
export CXXFLAGS="-U_FORTIFY_SOURCE  -O2"
build_test_target $1 "build_native" 0

#################
# ASan mode build
#################
export FUZZAN_MODE=1
build_llvm 1

cd $PATHROOT
echo "build asan"
export LDFLAGS="-lstdc++ -fuse-ld=bfd -U_FORTIFY_SOURCE -O2 -fsanitize=address"
export CFLAGS="-lstdc++ -U_FORTIFY_SOURCE  -O2 -fsanitize=address"
export CXXFLAGS="-U_FORTIFY_SOURCE  -O2 -fsanitize=address"
build_test_target $1 "build_asan" 0

################
# ASan-opt build
################
export FUZZAN_MODE=2
build_llvm 2

cd $PATHROOT
echo "build asan-opt"
export LDFLAGS="-lstdc++ -fuse-ld=bfd -U_FORTIFY_SOURCE -O2 -fsanitize=address"
export CFLAGS="-lstdc++ -U_FORTIFY_SOURCE -O2 -fsanitize=address"
export CXXFLAGS="-U_FORTIFY_SOURCE -O2 -fsanitize=address"
build_test_target $1 "build_asan_opt" 0

#######################
# RBTree-opt mode build
#######################
export FUZZAN_MODE=3
build_llvm 3

cd $PATHROOT
echo "build rbtree"
export LDFLAGS="-lstdc++ -fuse-ld=bfd -U_FORTIFY_SOURCE -O2 -fsanitize=address"
export CFLAGS="-lstdc++ -U_FORTIFY_SOURCE  -O2 -fsanitize=address"
export CXXFLAGS="-U_FORTIFY_SOURCE  -O2 -fsanitize=address"
build_test_target $1 "build_rbtree" 0

#######################
# Min-shadow-opt build
#######################
export FUZZAN_MODE=4
build_llvm 4
export MINMODE_ON=1

cd $PATHROOT
echo "build min"
export LDFLAGS="-lstdc++ -fuse-ld=bfd -U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
export CFLAGS="-lstdc++ -U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
export CXXFLAGS="-U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
build_test_target $1 "build_min" 1

#####################
# Sampling-mode build
#####################
export FUZZAN_MODE=8
build_llvm 8
export MINMODE_ON=1

# shadow memory setting
export CHECK_NUM="2"
export SHM_STR="."
export SHM_INT=7

cd $PATHROOT
echo "build fuzzan"
export LDFLAGS="-lstdc++ -fuse-ld=bfd -U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
export CFLAGS="-lstdc++ -U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
export CXXFLAGS="-U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
build_test_target $1 "build_fuzzan" 1

#########################
# Min-shadow-opt build 1G
#########################
export FUZZAN_MODE=4
build_llvm 4
export MINMODE_ON=1

cd $PATHROOT
echo "build min"
export LDFLAGS="-lstdc++ -fuse-ld=bfd -U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
export CFLAGS="-lstdc++ -U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
export CXXFLAGS="-U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
build_test_target $1 "build_min-1G" 1

#########################
# Min-shadow-opt build 4G
#########################
export FUZZAN_MODE=5
build_llvm 5
export MINMODE_ON=4

cd $PATHROOT
echo "build min"
export LDFLAGS="-lstdc++ -fuse-ld=bfd -U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
export CFLAGS="-lstdc++ -U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
export CXXFLAGS="-U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
build_test_target $1 "build_min-4G" 4

#########################
# Min-shadow-opt build 8G
#########################
export FUZZAN_MODE=6
build_llvm 6
export MINMODE_ON=8

cd $PATHROOT
echo "build min"
export LDFLAGS="-lstdc++ -fuse-ld=bfd -U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
export CFLAGS="-lstdc++ -U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
export CXXFLAGS="-U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
build_test_target $1 "build_min-8G" 8

##########################
# Min-shadow-opt build 16G
##########################
export FUZZAN_MODE=7
build_llvm 7
export MINMODE_ON=16

cd $PATHROOT
echo "build min"
export LDFLAGS="-lstdc++ -fuse-ld=bfd -U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
export CFLAGS="-lstdc++ -U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
export CXXFLAGS="-U_FORTIFY_SOURCE -O2 -fsanitize=address -mcmodel=small"
build_test_target $1 "build_min-16G" 16
