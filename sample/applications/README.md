FuZZan: realworld application test
=====================
1. Build
```console
./build.sh (binutils|file|libpng|tcpdump)
```

2. Run
```
2.1. Please copy this script into target testing folder
 - (e.g., cp ./run.sh /build_asan)
 - (e.g., cp ./run.sh /build_fuzzan)

2.2. Start fuzzing 
 - ./run.sh targe_program_name unique_shadow_int_value afl_mode(1: native-ASan, 2: FuZZan) (e.g., run.sh objdump 3 1)
 - For unique_shadow_int_value, please do not use duplicated number.
 - (e.g., /build_asan/run.sh objdump 3 1)
 - (e.g., /build_fuzzan/run.sh objdump 3 2)
```
