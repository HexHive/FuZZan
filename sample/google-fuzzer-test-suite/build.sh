#!/bin/bash

rm ./fts-* -rf

: ${PATHROOT:="$PWD"}
LLVM="$PATHROOT/../../LLVM/build/bin/"
LLVM_MAIN="$PATHROOT/../../LLVM/"
export LDFLAGS=""
export CFLAGS=""
export CXXFLAGS=""
ORG_PATH=$PATH

cp ./base_template/build_target_program.sh .
mkdir ./base_template/fts-asan/FTS && cp ./base_template/fuzzer-test-suite ./base_template/fts-asan/FTS -r
mkdir ./base_template/fts-min/FTS && cp ./base_template/fuzzer-test-suite ./base_template/fts-min/FTS -r
mkdir ./base_template/fts-fuzzan/FTS && cp ./base_template/fuzzer-test-suite ./base_template/fts-fuzzan/FTS -r
mkdir ./base_template/fts-native/FTS && cp ./base_template/fuzzer-test-suite ./base_template/fts-native/FTS -r

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

# Parameter 1: target directory
# Parameter 2: afl mode
# Parameter 3: min mode
build_fts () {
export PATH=$LLVM:$PATH
export CC=$PATHROOT/../../afl/afl-2.52b/afl-clang-fast
export CXX=$PATHROOT/../../afl/afl-2.52b/afl-clang-fast++
export BUILD="shared"

cd $1
./setup.sh $2
./build_target_program.sh $3
}

######################
# Native mode build
######################
export FUZZAN_MODE=1
build_llvm 1

cd $PATHROOT
echo "build native"
cp ./base_template/fts-native ./1-fts-native -r
build_fts "1-fts-native" "3" "0"

#####################
# ASan mode build
#####################
export FUZZAN_MODE=1
build_llvm 1

cd $PATHROOT
echo "build asan"
cp ./base_template/fts-asan ./2-fts-asan -r
build_fts "2-fts-asan" "3" "0"

##############################
# ASan-opt build
##############################
export FUZZAN_MODE=2
build_llvm 2

cd $PATHROOT
echo "build init mode"
cp ./base_template/fts-asan ./3-fts-init-logging -r
build_fts "3-fts-init-logging" "3" "0"

###########################
# RBTree-opt mode build
###########################
export FUZZAN_MODE=3
build_llvm 3

cd $PATHROOT
echo "build rbtree-full"
cp ./base_template/fts-asan ./4-fts-rbtree-full -r
build_fts "4-fts-rbtree-full" "3" "0"

#################################
# Min-shadow-opt build (1G)
#################################
export FUZZAN_MODE=4
build_llvm 4

export MINMODE_ON=1

cd $PATHROOT
echo "build min"
cp ./base_template/fts-min ./5-fts-min-1G-full -r
build_fts "5-fts-min-1G-full" "3" "1"

#################################
# Min-shadow-opt build (4G)
#################################
export FUZZAN_MODE=5
build_llvm 5

export MINMODE_ON=4

cd $PATHROOT
echo "build min"
cp ./base_template/fts-min ./6-fts-min-4G-full -r
build_fts "6-fts-min-4G-full" "3" "4"

#################################
# Min-shadow-opt build (8G)
#################################
export FUZZAN_MODE=6
build_llvm 6

export MINMODE_ON=8

cd $PATHROOT
echo "build min"
cp ./base_template/fts-min ./7-fts-min-8G-full -r
build_fts "7-fts-min-8G-full" "3" "8"

#################################
# Min-shadow-opt build (16G)
#################################
export FUZZAN_MODE=7
build_llvm 7

export MINMODE_ON=16

cd $PATHROOT
echo "build min"
cp ./base_template/fts-min ./8-fts-min-16G-full -r
build_fts "8-fts-min-16G-full" "3" "16"

###########################
# Sampling-mode build
###########################
export FUZZAN_MODE=8
build_llvm 8

export CHECK_NUM="2"
export SHM_STR="."
export SHM_INT=$1

export MINMODE_ON=1

cd $PATHROOT
echo "build sampling mode"
cp ./base_template/fts-fuzzan ./9-fts-ds-full -r
build_fts "9-fts-ds-full" "4" "1"

