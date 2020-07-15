#!/bin/bash
ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd FTS
ln -s $ROOT/../../../LLVM/compiler-rt/lib/fuzzer Fuzzer
ln -s $ROOT/../../../afl/afl-2.52b-native AFL
popd
