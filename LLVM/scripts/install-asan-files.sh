#!/bin/bash

#This script softlinks our modified files into the LLVM source tree

#Path to llvm source tree
llvm=`pwd`/llvm
src=`pwd`/src
runtime=`pwd`/compiler-rt

#install LLVM files
rm $llvm/lib/Transforms/Instrumentation/AddressSanitizer.cpp
ln -s $src/llvm-files/AddressSanitizer.cpp $llvm/lib/Transforms/Instrumentation/AddressSanitizer.cpp

rm $runtime/lib/asan/asan_rtl.cc
rm $runtime/lib/asan/asan_allocator.cc
rm $runtime/lib/asan/asan_allocator.h
rm $runtime/lib/asan/CMakeLists.txt
rm $runtime/lib/sanitizer_common/CMakeLists.txt
rm $runtime/lib/sanitizer_common/sanitizer_allocator_secondary.h
rm $runtime/lib/asan/asan_interceptors_memintrinsics.h
rm $runtime/lib/asan/asan_interface_internal.h
rm $runtime/lib/asan/asan_mapping.h
rm $runtime/lib/asan/asan_poisoning.cc
rm $runtime/lib/asan/asan_poisoning.h
rm $runtime/lib/asan/asan_rtems.cc
rm $runtime/lib/asan/asan_report.cc
rm $runtime/lib/sanitizer_common/sanitizer_allocator_primary64.h
rm $runtime/lib/sanitizer_common/sanitizer_platform_limits_posix.cc
rm $runtime/lib/sanitizer_common/sanitizer_platform_limits_posix.h
rm $runtime/lib/sanitizer_common/sanitizer_linux_libcdep.cc
rm $runtime/lib/asan/asan_flags.inc
rm $runtime/lib/asan/asan_shadow_setup.cc
rm $runtime/lib/asan/asan_globals.cc
rm $runtime/lib/asan/asan_fake_stack.cc
rm $runtime/lib/asan/asan_interface.inc
rm $runtime/lib/asan/asan_internal.h

ln -s $src/compiler-rt-files/asan_shadow_setup.cc $runtime/lib/asan/asan_shadow_setup.cc
ln -s $src/compiler-rt-files/asan_internal.h $runtime/lib/asan/asan_internal.h
ln -s $src/compiler-rt-files/asan_rtl.cc $runtime/lib/asan/asan_rtl.cc
ln -s $src/compiler-rt-files/asan_allocator.cc $runtime/lib/asan/asan_allocator.cc
ln -s $src/compiler-rt-files/asan_allocator.h $runtime/lib/asan/asan_allocator.h
ln -s $src/compiler-rt-files/lib_asan_cmakelists.txt $runtime/lib/asan/CMakeLists.txt
ln -s $src/compiler-rt-files/sanitizer_CMakeLists.txt $runtime/lib/sanitizer_common/CMakeLists.txt
ln -s $src/compiler-rt-files/sanitizer_allocator_secondary.h $runtime/lib/sanitizer_common/sanitizer_allocator_secondary.h
ln -s $src/compiler-rt-files/sanitizer_platform_limits_posix.h $runtime/lib/sanitizer_common/sanitizer_platform_limits_posix.h
ln -s $src/compiler-rt-files/sanitizer_platform_limits_posix.cc $runtime/lib/sanitizer_common/sanitizer_platform_limits_posix.cc
ln -s $src/compiler-rt-files/asan_interceptors_memintrinsics.h $runtime/lib/asan/asan_interceptors_memintrinsics.h
ln -s $src/compiler-rt-files/asan_interface_internal.h $runtime/lib/asan/asan_interface_internal.h
ln -s $src/compiler-rt-files/asan_mapping.h $runtime/lib/asan/asan_mapping.h
ln -s $src/compiler-rt-files/asan_poisoning.cc $runtime/lib/asan/asan_poisoning.cc
ln -s $src/compiler-rt-files/asan_poisoning.h $runtime/lib/asan/asan_poisoning.h
if [ ! -f $runtime/lib/asan/asan_rbtree.h ]; then
ln -s $src/compiler-rt-files/asan_rbtree.h $runtime/lib/asan/asan_rbtree.h
fi
if [ ! -f $runtime/lib/asan/asan_rbtree.cc ]; then
ln -s $src/compiler-rt-files/asan_rbtree.cc $runtime/lib/asan/asan_rbtree.cc
fi
if [ ! -f $runtime/lib/sanitizer_common/asan_options.h ]; then
ln -s $src/compiler-rt-files/asan_options.h $runtime/lib/sanitizer_common/asan_options.h
fi
if [ ! -f $runtime/lib/sanitizer_common/asan_options_no_function_define.h ]; then
ln -s $src/compiler-rt-files/asan_options_no_function_define.h $runtime/lib/sanitizer_common/asan_options_no_function_define.h
fi
ln -s $src/compiler-rt-files/asan_rtems.cc $runtime/lib/asan/asan_rtems.cc
ln -s $src/compiler-rt-files/asan_report.cc $runtime/lib/asan/asan_report.cc
ln -s $src/compiler-rt-files/sanitizer_allocator_primary64.h $runtime/lib/sanitizer_common/sanitizer_allocator_primary64.h
ln -s $src/compiler-rt-files/sanitizer_linux_libcdep.cc $runtime/lib/sanitizer_common/sanitizer_linux_libcdep.cc
ln -s $src/compiler-rt-files/asan_flags.inc $runtime/lib/asan/asan_flags.inc
ln -s $src/compiler-rt-files/asan_globals.cc $runtime/lib/asan/asan_globals.cc
ln -s $src/compiler-rt-files/asan_fake_stack.cc $runtime/lib/asan/asan_fake_stack.cc
ln -s $src/compiler-rt-files/asan_interface.inc $runtime/lib/asan/asan_interface.inc
