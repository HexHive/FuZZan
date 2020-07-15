#!/bin/bash
. $(dirname $0)/../common.sh

[ ! -e binutils-2.31.90.tar.xz ] && wget ftp://sourceware.org/pub/binutils/snapshots/binutils-2.31.90.tar.xz
[ ! -e binutils-2.31.90 ] && tar -xf binutils-2.31.90.tar.xz

build_app() {
  rm -rf build
  mkdir build
  pushd binutils-2.31.90
  ./configure --prefix=`pwd`/../build
  make clean
  make -j -i
  make install
  popd
}

build_app
cp build/bin/* .

if [[ $1 != 0 ]]; then
 $(pwd)/../../../../etc/libshrink/wrap.sh c++filt $1 $(pwd)/c++filt
 $(pwd)/../../../../etc/libshrink/wrap.sh objdump $1 $(pwd)/objdump
 $(pwd)/../../../../etc/libshrink/wrap.sh nm $1 $(pwd)/nm
 $(pwd)/../../../../etc/libshrink/wrap.sh size $1 $(pwd)/size
fi


