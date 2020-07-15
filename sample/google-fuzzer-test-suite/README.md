FuZZan: google fuzzer test suite
=====================
1. Build all 26 applications
```console
./build.sh
```

2. Run
```
2.1. Start fuzzing 
 - ./run.sh target_program_index(0-25) mode(0:asan and 1:fuzzan) unique_shadow_int_value (e.g., ./run.sh 0 0 5)
 - For unique_shadow_int_value, please do not use duplicated number.
 - Program index
 -- 0: boringssl
 -- 1: c-ares
 -- 2: freetype2
 -- 3: harfbuzz
 -- 4: json
 -- 5: lcms
 -- 6: libarchive
 -- 7: llvm-libcxxabi
 -- 8: libpng
 -- 9: libjpeg
 --10: libxml2
 --11: libssh
 --12: openssl-1.0.2d
 --13: openssl-bignum
 --14: openssl-x509
 --15: openthread-ip6
 --16: openthread-radio
 --17: pcre2
 --18: proj4
 --19: re2
 --20: sqlite
 --21: vorbis
 --22: woff2
 --23: wpantund
 --24: openssl-1.0.1f
 --25: guetzli
```
