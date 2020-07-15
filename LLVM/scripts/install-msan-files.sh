#!/bin/bash

#This script softlinks our modified files into the LLVM source tree

#Path to llvm source tree
llvm=`pwd`/llvm
src=`pwd`/src
runtime=`pwd`/compiler-rt

rm $runtime/lib/msan/msan_allocator.cc
rm $runtime/lib/msan/msan.h
rm $runtime/lib/msan/msan_linux.cc
rm $runtime/lib/msan/CMakeLists.txt
rm $runtime/lib/msan/msan_interceptors.cc
rm $runtime/lib/msan/msan_origin.h

ln -s $src/compiler-rt-files/msan_interceptors.cc $runtime/lib/msan/msan_interceptors.cc
ln -s $src/compiler-rt-files/msan_allocator.cc $runtime/lib/msan/msan_allocator.cc
ln -s $src/compiler-rt-files/msan.h $runtime/lib/msan/msan.h
ln -s $src/compiler-rt-files/msan_linux.cc $runtime/lib/msan/msan_linux.cc
ln -s $src/compiler-rt-files/msan_origin.h $runtime/lib/msan/msan_origin.h
ln -s $src/compiler-rt-files/lib_msan_cmakelists.txt $runtime/lib/msan/CMakeLists.txt
