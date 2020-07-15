#!/bin/bash
. $(dirname $0)/../common.sh

build_app() {
  mkdir -p build
  git clone git@github.com:glennrp/libpng.git libpng
  pushd libpng
  ./configure --disable-shared --prefix=`pwd`/../build --disable-libseccomp 
  make clean all
  sudo make install
  popd
}

build_app

if [[ $1 != 0 ]]; then
 $(pwd)/../../../../etc/libshrink/wrap.sh pngfix $1 $(pwd)/build/bin/pngfix
fi
