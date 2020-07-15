#!/bin/bash
. $(dirname $0)/../common.sh

build_app() {
  rm -rf file_deps
  mkdir -p file_deps
  pushd zlib
  ./configure --static --prefix=`pwd`/../file_deps
  make clean
  make -j -i
  make install
  popd

  rm -rf build
  mkdir -p build
  pushd SRC
  autoreconf -i
  LDFLAGS="-L`pwd`/../file_deps/lib $LDFLAGS" \
  ./configure --prefix=`pwd`/../build --disable-libseccomp --disable-shared --enable-static
  make clean
  make -j -i
  make install
  popd
}

get_git_revision https://github.com/file/file.git 6367a7c9b476767a692f76e78e3b355dc9386e48 SRC
get_git_revision https://github.com/madler/zlib.git cacf7f1d4e3d44d871b605da3b647f07d718623f zlib

build_app
cp build/bin/file .

if [[ $1 != 0 ]]; then
 $(pwd)/../../../../etc/libshrink/wrap.sh file $1 $(pwd)/file
fi

mkdir -p magic
cp build/share/misc/magic.mgc magic/
