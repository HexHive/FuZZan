LLVM_DIR = ${CURDIR}/llvm
BUILD_DIR = ${CURDIR}/build

fuzzan:
	mkdir -p ${BUILD_DIR}
	(cd ${BUILD_DIR} && \
	  CC=clang CXX=clang++ \
	  cmake -G "Ninja"\
	  -DFUZZAN_VERSION=${FUZZAN_MODE}\
	  -DCMAKE_BUILD_TYPE=Release \
	  -DCMAKE_C_COMPILER=clang \
	  -DCMAKE_CXX_COMPILER=clang++ \
	  -DLLVM_ENABLE_ASSERTIONS=OFF \
	  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	  -DLLVM_BUILD_TESTS=OFF \
	  -DLLVM_BUILD_EXAMPLES=OFF \
	  -DLLVM_INCLUDE_TESTS=ON\
	  -DLLVM_INCLUDE_EXAMPLES=ON \
	  -DLLVM_TARGETS_TO_BUILD="X86" \
	  -DBUILD_SHARED_LIBS=OFF \
	${LLVM_DIR})
	(ninja -C ${BUILD_DIR})

test:
	(cd ${BUILD_DIR} && ninja check-asan)

clean:
	rm -rf ${BUILD_DIR}

.PHONY: fuzzan test clean
