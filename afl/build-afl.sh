#!/bin/bash

if [ ! -e ../LLVM/build/bin/ ]; then
    echo "Please build clang/llvm with hextype first by running `cd ../LLVM; ./build.sh`"
    exit
fi

ROOT=`pwd`
export PATH=$ROOT/../LLVM/build/bin/:$PATH

export FUZZAN_OPTION="-DFMODE1"
cp afl-2.52b afl-2.52b-native -r
cd afl-2.52b-native
make clean && make
cd llvm_mode
make clean && make
cd ../../

export FUZZAN_OPTION="-DFMODE2"
cd afl-2.52b
make clean && make
cd llvm_mode
make clean && make
