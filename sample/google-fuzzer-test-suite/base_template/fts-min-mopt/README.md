Fuzzing google fuzzer-test-suite with ToggleFuzz
==================================================
# Setup
```
$ ./setup.sh
```

# Build package
- To build a package with fuzzing engine
```
$ ./build.sh [fuzzing_engine] [path_to_package]
$ ./build.sh afl FTS/fuzzer-test-suite/libpng-1.2.56
```
- The built package is under
```
$ build/[fuzzing_engine]/[package]
```

# Testing package
- To test a package with fuzzing engine
```
$ ./run.sh [path_to_package_binary]
$ ./run.sh build/afl/libpng-1.2.56/libpng-1.2.56-afl
```
