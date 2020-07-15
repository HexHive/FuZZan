#!/bin/bash

if [ "$#" -ne 1 ]; then
  echo "Please run ./build.sh with LLVM mode (example ./build.sh 3)"
  echo "-- Mode 1: naitve mode"
  echo "-- Mode 3: fuzzan full-rb-tree mode"
  echo "-- Mode 4: fuzzan min (1G)"
  echo "-- Mode 5: fuzzan min (4G)"
  echo "-- Mode 6: fuzzan min (8G)"
  echo "-- Mode 7: fuzzan min (16G)"
  echo "-- Mode 8: fuzzan sampling mode"
  echo "-- Mode 9: fuzzan native msan + disable storage lock/unlock"
  echo "-- Mode 10: fuzzan full-min msan (16G) + disable log"
  exit 1
fi

export FUZZAN_MODE=$1

#get LLVM
if [ ! -d llvm ]; then
wget --retry-connrefused --tries=100 releases.llvm.org/7.0.0/llvm-7.0.0.src.tar.xz
tar -xf llvm-7.0.0.src.tar.xz
mv llvm-7.0.0.src llvm
rm llvm-7.0.0.src.tar.xz

pushd llvm/tools
ln -s ../../clang .
popd
fi

#get Clang
if [ ! -d clang ]; then
wget --retry-connrefused --tries=100 releases.llvm.org/7.0.0/cfe-7.0.0.src.tar.xz
tar -xf cfe-7.0.0.src.tar.xz
mv cfe-7.0.0.src clang
rm cfe-7.0.0.src.tar.xz
fi

#get compiler-rt
if [ ! -d compiler-rt ]; then
wget --retry-connrefused --tries=100 releases.llvm.org/7.0.0/compiler-rt-7.0.0.src.tar.xz
tar -xf compiler-rt-7.0.0.src.tar.xz
mv compiler-rt-7.0.0.src compiler-rt
rm compiler-rt-7.0.0.src.tar.xz

pushd llvm/projects
ln -s ../../compiler-rt .
popd
fi

# install ASAN source codes
./scripts/install-asan-files.sh
./scripts/install-msan-files.sh

make -j
