#!bin/bash
LLVM_VER=7.0.0
SCRIPT_DIR=$(dirname $0)
LIB_FUZZING_ENGINE=libFuzzingEngine.a
LIBFUZZER_SRC=$(dirname $(dirname $SCRIPT_DIR))/../../LLVM/compiler-rt/lib/fuzzer
AFL_SRC=${SCRIPT_DIR}/../../afl-2.52b

get_git_revision() {
  GIT_REPO="$1"
  GIT_REVISION="$2"
  TO_DIR="$3"
  [ ! -e $TO_DIR ] && git clone $GIT_REPO $TO_DIR && (cd $TO_DIR && git reset --hard $GIT_REVISION)
}

build_afl() {
  set -x
  $CC $CFLAGS -c ${LIBFUZZER_SRC}/standalone/StandaloneFuzzTargetMain.c
  ar r $LIB_FUZZING_ENGINE StandaloneFuzzTargetMain.o
  rm *.o
}

build_fuzzer() {
  echo "Building with afl"
  build_afl
}

build_zlib() {
  get_git_revision https://github.com/madler/zlib.git cacf7f1d4e3d44d871b605da3b647f07d718623f zlib
  DEPS="$1"
  pushd zlib
  ./configure --static --prefix=$DEPS
  make clean
  make -j
  make install
  popd
}
