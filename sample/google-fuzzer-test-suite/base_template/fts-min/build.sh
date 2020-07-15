#!/bin/bash -e
ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
LLVM_DIR=$ROOT/../../../LLVM/build/bin
export AFL_PATH=$ROOT/FTS/AFL

# Ensure that fuzzing engine, if defined, is valid
export FUZZING_ENGINE=${1:-"afl"}
POSSIBLE_FUZZING_ENGINE="libfuzzer afl fsanitize_fuzzer hooks"
!(echo "$POSSIBLE_FUZZING_ENGINE" | grep -w "$FUZZING_ENGINE" > /dev/null) && \
  echo "USAGE: Error: If defined, FUZZING_ENGINE should be one of the following:
  $POSSIBLE_FUZZING_ENGINE. However, it was defined as $FUZZING_ENGINE" && exit 1

if [[ $FUZZING_ENGINE == "afl" ]]; then
  if [[ ! -f $AFL_PATH/afl-clang-fast ]]; then
    echo "Could not find afl-clang-fast, please build afl first"
    exit
  fi
  export CC=afl-clang-fast
  export CXX=afl-clang-fast++
  export CFLAGS="-O2 -fno-omit-frame-pointer -gline-tables-only -fsanitize=address -mcmodel=small"
  export CXXFLAGS="-O2 -fno-omit-frame-pointer -gline-tables-only -lpthread -ldl -fsanitize=address -mcmodel=small"
  export LIBS="-lstdc++ -lpthread -ldl"
elif [[ $FUZZING_ENGINE == "libfuzzer" ]]; then
  export CC=clang
  export CXX=clang++
  export CFLAGS=$FUZZ_CXXFLAGS
  export CXXFLAGS=$FUZZ_CXXFLAGS
  export LDFLAGS=$FUZZ_CXXFLAGS
fi

export PATH=$AFL_PATH:$LLVM_DIR:$PATH

PKG=$2
PKG_NAME=${PKG##*/}

mkdir -p build/$FUZZING_ENGINE/$PKG_NAME
if [ -d $PKG/seeds ]; then
  cp -R $PKG/seeds build/$FUZZING_ENGINE/$PKG_NAME/
fi
pushd build/$FUZZING_ENGINE/$PKG_NAME

if [ -f ../../../$PKG/build.sh ]; then
  ../../../$PKG/build.sh
  echo "$PKG_NAME build done"
else
  echo "$PKG is not a package path"
fi

echo "apply shrinking script\n"
echo $(pwd)

if [[ $PKG_NAME == "openthread-2018-02-27" ]]; then
  $(pwd)/../../../../../../etc/libshrink/wrap.sh $PKG_NAME-radio $3 $(pwd)/$PKG_NAME-afl-radio
  $(pwd)/../../../../../../etc/libshrink/wrap.sh $PKG_NAME-ip6 $3 $(pwd)/$PKG_NAME-afl-ip6
elif [[ $PKG_NAME == "openssl-1.1.0c" ]]; then
  $(pwd)/../../../../../../etc/libshrink/wrap.sh $PKG_NAME-x509 $3 $(pwd)/$PKG_NAME-afl-x509
  $(pwd)/../../../../../../etc/libshrink/wrap.sh $PKG_NAME-bignum $3 $(pwd)/$PKG_NAME-afl-bignum
else
  $(pwd)/../../../../../../etc/libshrink/wrap.sh  $PKG_NAME $3 $(pwd)/$PKG_NAME-afl
fi
