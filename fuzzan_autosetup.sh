#!/bin/bash
set -e

# install prerequisite package
sudo apt install prelink patchelf

# build llvm as native version
echo "build llvm"
export FUZZAN_MODE="1"
pushd LLVM
./build.sh 1
popd

# build libshrink
pushd etc/libshrink
./build.sh
popd

# build afl
echo "build afl"
pushd afl
./build-afl.sh
popd
