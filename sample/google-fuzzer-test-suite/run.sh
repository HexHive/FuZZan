#!/bin/bash

if [ "$#" -ne 3 ]; then
  echo "./run.sh targe_program(0-25) mode(0:asan and 1:fuzzan) unique_shadow_int_value(please avoid duplicated number) (e.g., ./run.sh 0~25 0~1 5)"
  exit 1
fi

surfix_path="/build/afl/"
prefix_path="$(pwd)/"

export CHECK_NUM="1"
export SHM_STR="."
export SHM_INT=$3
export SWITCHRECORD_PATH="$log_path/switch_num_$3.txt"
export EXECTIMERECORD_PATH="$log_path/exec_time_$3.txt"
export FUZZAN_LOG_PATH="$log_path/fuZZan_auto_$3.txt"

# 26 applications
declare -a array=("boringssl" "c-ares" "freetype2" "harfbuzz" "json" "lcms" "libarchive" "llvm-libcxxabi" "libpng" "libjpeg" "libxml2" "libssh" "openssl-1.0.2d" "openssl-bignum" "openssl-x509" "openthread-ip6" "openthread-radio" "pcre2" "proj4" "re2"  "sqlite" "vorbis" "woff2" "wpantund" "openssl-1.0.1f" "guetzli")
declare -a run_commands=("/boringssl-2016-02-12/boringssl-2016-02-12-afl" "/c-ares-CVE-2016-5180/c-ares-CVE-2016-5180-afl" "/freetype2-2017/freetype2-2017-afl" "/harfbuzz-1.3.2/harfbuzz-1.3.2-afl" "/json-2017-02-12/json-2017-02-12-afl" "/lcms-2017-03-21/lcms-2017-03-21-afl" "/libarchive-2017-01-04/libarchive-2017-01-04-afl" "/llvm-libcxxabi-2017-01-27/llvm-libcxxabi-2017-01-27-afl" "/libpng-1.2.56/libpng-1.2.56-afl" "/libjpeg-turbo-07-2017/libjpeg-turbo-07-2017-afl" "/libxml2-v2.9.2/libxml2-v2.9.2-afl" "/libssh-2017-1272/libssh-2017-1272-afl" "/openssl-1.0.2d/openssl-1.0.2d-afl" "/openssl-1.1.0c/openssl-1.1.0c-afl-bignum" "/openssl-1.1.0c/openssl-1.1.0c-afl-x509" "/openthread-2018-02-27/openthread-2018-02-27-afl-ip6" "/openthread-2018-02-27/openthread-2018-02-27-afl-radio" "/pcre2-10.00/pcre2-10.00-afl" "/proj4-2017-08-14/proj4-2017-08-14-afl" "/re2-2014-12-09/re2-2014-12-09-afl" "/sqlite-2016-11-14/sqlite-2016-11-14-afl" "/vorbis-2017-12-11/vorbis-2017-12-11-afl" "/woff2-2016-05-06/woff2-2016-05-06-afl" "/wpantund-2018-02-27/wpantund-2018-02-27-afl" "/openssl-1.0.1f/openssl-1.0.1f-afl" "/guetzli-2017-3-30/guetzli-2017-3-30-afl")
declare -a short_commands=("/boringssl-2016-02-12/" "/c-ares-CVE-2016-5180/" "/freetype2-2017/" "/harfbuzz-1.3.2/" "/json-2017-02-12/" "/lcms-2017-03-21/" "/libarchive-2017-01-04/" "/llvm-libcxxabi-2017-01-27/" "/libpng-1.2.56/" "/libjpeg-turbo-07-2017/" "/libxml2-v2.9.2/" "/libssh-2017-1272/" "/openssl-1.0.2d/" "/openssl-1.1.0c/" "/openssl-1.1.0c/" "/openthread-2018-02-27/" "/openthread-2018-02-27/" "/pcre2-10.00/" "/proj4-2017-08-14/" "/re2-2014-12-09/" "/sqlite-2016-11-14/" "/vorbis-2017-12-11/" "/woff2-2016-05-06/" "/wpantund-2018-02-27/" "/openssl-1.0.1f/" "/guetzli-2017-3-30/")

declare -a run_commands_min=("/boringssl-2016-02-12/prelink-boringssl-2016-02-12/boringssl-2016-02-12-afl" "/c-ares-CVE-2016-5180/prelink-c-ares-CVE-2016-5180/c-ares-CVE-2016-5180-afl" "/freetype2-2017/prelink-freetype2-2017/freetype2-2017-afl" "/harfbuzz-1.3.2/prelink-harfbuzz-1.3.2/harfbuzz-1.3.2-afl" "/json-2017-02-12/prelink-json-2017-02-12/json-2017-02-12-afl" "/lcms-2017-03-21/prelink-lcms-2017-03-21/lcms-2017-03-21-afl" "/libarchive-2017-01-04/prelink-libarchive-2017-01-04/libarchive-2017-01-04-afl" "/llvm-libcxxabi-2017-01-27/prelink-llvm-libcxxabi-2017-01-27/llvm-libcxxabi-2017-01-27-afl" "/libpng-1.2.56/prelink-libpng-1.2.56/libpng-1.2.56-afl" "/libjpeg-turbo-07-2017/prelink-libjpeg-turbo-07-2017/libjpeg-turbo-07-2017-afl" "/libxml2-v2.9.2/prelink-libxml2-v2.9.2/libxml2-v2.9.2-afl" "/libssh-2017-1272/prelink-libssh-2017-1272/libssh-2017-1272-afl" "/openssl-1.0.2d/prelink-openssl-1.0.2d/openssl-1.0.2d-afl" "/openssl-1.1.0c/prelink-openssl-1.1.0c-bignum/openssl-1.1.0c-afl-bignum" "/openssl-1.1.0c/prelink-openssl-1.1.0c-x509/openssl-1.1.0c-afl-x509" "/openthread-2018-02-27/prelink-openthread-2018-02-27-ip6/openthread-2018-02-27-afl-ip6" "/openthread-2018-02-27/prelink-openthread-2018-02-27-radio/openthread-2018-02-27-afl-radio" "/pcre2-10.00/prelink-pcre2-10.00/pcre2-10.00-afl" "/proj4-2017-08-14/prelink-proj4-2017-08-14/proj4-2017-08-14-afl" "/re2-2014-12-09/prelink-re2-2014-12-09/re2-2014-12-09-afl" "/sqlite-2016-11-14/prelink-sqlite-2016-11-14/sqlite-2016-11-14-afl" "/vorbis-2017-12-11/prelink-vorbis-2017-12-11/vorbis-2017-12-11-afl" "/woff2-2016-05-06/prelink-woff2-2016-05-06/woff2-2016-05-06-afl" "/wpantund-2018-02-27/prelink-wpantund-2018-02-27/wpantund-2018-02-27-afl" "/openssl-1.0.1f/prelink-openssl-1.0.1f/openssl-1.0.1f-afl" "/guetzli-2017-3-30/prelink-guetzli-2017-3-30/guetzli-2017-3-30-afl")
declare -a short_commands_min=("/boringssl-2016-02-12/prelink-boringssl-2016-02-12/" "/c-ares-CVE-2016-5180/prelink-c-ares-CVE-2016-5180/" "/freetype2-2017/prelink-freetype2-2017/" "/harfbuzz-1.3.2/prelink-harfbuzz-1.3.2/" "/json-2017-02-12/prelink-json-2017-02-12/" "/lcms-2017-03-21/prelink-lcms-2017-03-21/" "/libarchive-2017-01-04/prelink-libarchive-2017-01-04/" "/llvm-libcxxabi-2017-01-27/prelink-llvm-libcxxabi-2017-01-27/" "/libpng-1.2.56/prelink-libpng-1.2.56/" "/libjpeg-turbo-07-2017/prelink-libjpeg-turbo-07-2017/" "/libxml2-v2.9.2/prelink-libxml2-v2.9.2/" "/libssh-2017-1272/prelink-libssh-2017-1272/" "/openssl-1.0.2d/prelink-openssl-1.0.2d/" "/openssl-1.1.0c/prelink-openssl-1.1.0c-bignum/" "/openssl-1.1.0c/prelink-openssl-1.1.0c-x509/" "/openthread-2018-02-27/prelink-openthread-2018-02-27-ip6/" "/openthread-2018-02-27/prelink-openthread-2018-02-27-radio/" "/pcre2-10.00/prelink-pcre2-10.00/" "/proj4-2017-08-14/prelink-proj4-2017-08-14/" "/re2-2014-12-09/prelink-re2-2014-12-09/" "/sqlite-2016-11-14/prelink-sqlite-2016-11-14/" "/vorbis-2017-12-11/prelink-vorbis-2017-12-11/" "/woff2-2016-05-06/prelink-woff2-2016-05-06/" "/wpantund-2018-02-27/prelink-wpantund-2018-02-27/" "/openssl-1.0.1f/prelink-openssl-1.0.1f/" "/guetzli-2017-3-30/prelink-guetzli-2017-3-30/")

declare -a modes=("2-fts-asan" "9-fts-ds-full")

sudo sh -c 'echo core | sudo tee /proc/sys/kernel/core_pattern'
sudo sh -c 'cd /sys/devices/system/cpu; echo performance | sudo tee cpu*/cpufreq/scaling_governor; cd -'
sudo sh -c 'echo 1 | sudo tee /proc/sys/kernel/sched_child_runs_first'
sudo sysctl -w kernel.randomize_va_space=0

i=$1 
j=$2 
x=$3

# set environment for each application
# move to target directory
cd "$prefix_path${modes[$j]}"
export TARGET_ENV="${array[$i]}"
export SAMPLE_PATH="$(pwd)/build/afl/${run_commands_min[$i]}"

index=$(($j + 1))
asan_switch_path="$prefix_path/3-fts-init-logging/build/afl/"
rb_switch_path="$prefix_path/4-fts-rbtree-full/build/afl/"
min_switch_path="$prefix_path/5-fts-min-1G-full/build/afl/"
min_switch_path_4g="$prefix_path/6-fts-min-4G-full/build/afl/"
min_switch_path_8g="$prefix_path/7-fts-min-8G-full/build/afl/"
min_switch_path_16g="$prefix_path/8-fts-min-16G-full/build/afl/"
export ASAN_PATH="$asan_switch_path${run_commands[$i]}"
export RBTREE_PATH="$rb_switch_path${run_commands[$i]}"
export MINSHADOW_PATH="$min_switch_path${run_commands_min[$i]}"
export MINSHADOW_PATH_4G="$min_switch_path_4g${run_commands_min[$i]}"
export MINSHADOW_PATH_8G="$min_switch_path_8g${run_commands_min[$i]}"
export MINSHADOW_PATH_16G="$min_switch_path_16g${run_commands_min[$i]}"

if [ $j -gt 3 ] && [ $j -lt 9 ]; then
  echo "run min mode !"
  ./run.sh build/afl/${run_commands_min[$i]} $1
else
  echo "run normal mode !"
  echo "build/afl/${run_commands[$i]}"
  ./run.sh build/afl/${run_commands[$i]} $1
fi
