#!/bin/bash
. $(dirname $0)/../common.sh

build_app() {
  pushd libpcap
  autoreconf -f -i
  ./configure --disable-shared
  cp ../../../Makefile_libpcap ./Makefile
  make clean
  make -j
  popd

  rm -rf build
  mkdir build
  pushd SRC
  autoreconf -f -i
  ./configure --prefix=`pwd`/../build
  cp ../../../Makefile_SRC ./Makefile
  make clean
  make -j
  make install
  popd
}

get_git_revision https://github.com/the-tcpdump-group/tcpdump.git 0b3880c91e169db7cfbdce1b18ef4f1e3fd277de SRC
get_git_revision https://github.com/the-tcpdump-group/libpcap.git 1a83bb6703bdcb3f07433521d95b788fa2ab4825 libpcap

build_app
cp build/sbin/tcpdump .

if [[ $1 != 0 ]]; then
 $(pwd)/../../../../etc/libshrink/wrap.sh tcpdump $1 $(pwd)/tcpdump
fi
