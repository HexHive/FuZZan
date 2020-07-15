#!/bin/bash
ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd FTS
ln -s $ROOT/../../../LLVM/compiler-rt/lib/fuzzer Fuzzer
ln -s $ROOT/../../../MOpt-AFL/afl-2.52b AFL
popd
