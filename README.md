# FuZZan
The combination of a fuzzer with ASan is currently the most effective approach to find memory safety violations.
However, several of ASan’s design choices conflict with fuzzer executions, increasing the runtime cost and reducing the benefit of combining fuzzing and sanitization. Thus, we propose to use FuZZan, our dynamic metadata structure switching sanitizer, instead of ASan for fuzzing. We design new metadata structures to replace ASan’s rigid shadow memory, reducing the memory management overhead while maintaining the same error detection abilities. Our dynamic metadata structure adaptively selects the most efficient metadata structure for the current fuzzing campaign without manual configuration.

## Environment
- Tested on Ubuntu 19.10 64bit

## Build FuZZan-enabled LLVM and AFL
- "fuzzan_autosetup.sh" script builds FuZZan-enabled LLVM and AFL

```
$ git clone git@github.com:HexHive/FuZZan.git
$ cd FuZZan
$ ./fuzzan_autosetup.sh
```

## Build Target Fuzzing Program with FuZZan
- To use FuZZan's dynamic metadata switching mode, you need to build target programs as seven different modes
```
- ASan-Opt (Mode# is 2)
- FuZZan RBTree-Opt(Mode# is 3)
- FuZZan Min-shadow-Opt 1G (Mode# is 4)
- FuZZan Min-shadow-Opt 4G (Mode# is 5)
- FuZZan Min-shadow-Opt 8G (Mode# is 6)
- FuZZan Min-shadow-Opt 16G (Mode# is 7)
- FuZZan Sampling mode (Mode# is 8)
```
- Before building each different mode, please rebuild (run `make -j`) LLVM and build the target program after resetting the "FUZZAN_MODE" environment variable with target FuZZan mode's number.
```
- e.g., export FUZZAN_MODE=3 (when you want to use RBTree(#3) mode)
```
- As an example, please refer to the real-world applications (sample/applications/build.sh) or google-fuzzer-test-suite (sample/google-fts/build.sh) build script.

## Run fuzzing
To run the dynamic metadata switching mode, you need to set the environment variables below:
```
- MINMODE_ON : please set 1 (true), as sampling mode is based on min-shadow mode
- MIN_SCRIPT_PATH : please set libshrink path (e.g., /home/foo/FuZZan/etc/libshrink/)
- SAMPLE_PATH : sampling binary path
- RBTREE_PATH : rbtree binary path
- MINSHADOW_PATH : minshadow (1G) binary path
- MINSHADOW_PATH_4G : minshadow (4G) binary path
- MINSHADOW_PATH_8G : minshadow (8G) binary path
- MINSHADOW_PATH_16G : minshadow (16G) binary path
- ASAN_PATH : asan binary path
- SHM_STR : string to create unique shard-memory key
- SHM_INT : int to create unique shard-memory key
- CHECK_NUM : the number of sampling mode iterations (default: 1)
```
- As an example, please refer to the real-world application (sample/applications/run.sh) script or google-fuzzer-test-suite (sample/google-fts/base_template/fts-asan/build.sh) script.

## Example
### Test binutils, libpng, tcpdump, and size
Please refer: /sample/applications/README.md

### Test google fuzzer test suite
Please refer: /sample/google-fuzzer-test-suite/README.md
