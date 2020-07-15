#!/bin/bash -e
ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
LLVM_DIR=$ROOT/../../../LLVM/build/bin
AFL_PATH=$ROOT/FTS/AFL
export PATH=$LLVM_DIR:$PATH
BINARY=${1##*/}

sudo sh -c 'echo core | sudo tee /proc/sys/kernel/core_pattern'
sudo sh -c 'cd /sys/devices/system/cpu; echo performance | sudo tee cpu*/cpufreq/scaling_governor; cd -'
sudo sh -c 'echo 1 | sudo tee /proc/sys/kernel/sched_child_runs_first'
sudo sysctl -w kernel.randomize_va_space=0

export CHECK_NUM="1"
export SHM_STR="."
export SHM_INT=$2
export MINMODE_ON="1"
export MIN_SCRIPT_PATH="/home/jeon41/FuZZan/etc/libshrink/"

export SAMPLE_PATH="$(pwd)/$1"

pushd $(dirname $1)

[[ ! -d seeds ]] && mkdir seeds

if [[ -d ../seeds ]]; then
  cp ../seeds/* ./seeds -r
fi

[[ ! $(find seeds -type f) ]] && echo > ./seeds/nil_seed

rm -rf corpus
mkdir -p corpus

export ASAN_OPTIONS="abort_on_error=1:detect_leaks=0:symbolize=0"

set -x

if [[ $BINARY == *-afl* ]]; then
  CMD=$AFL_PATH/afl-fuzz
  SEED=seeds
  if [[ $BINARY == *woff2-2016-05-06-afl ]]; then
    cp ../*.o .
  fi
  if [[ $BINARY == *sqlite-2016-11-14-afl ]]; then
    cp ../*.o .
  fi
  if [[ $BINARY == *-afl-x509 ]]; then
    cp ../x509.o .
  fi
  if [[ $BINARY == *-afl-bignum ]]; then
    cp ../bignum.o .
  fi
  if [[ $BINARY == *-1.0.1f-afl ]]; then
    [[ ! -d runtime ]] && mkdir runtime
    cp ../runtime/* ./runtime -r
  fi
  if [[ $BINARY == *libxml2-v2.9.2-afl ]]; then
    cp ../xml.dict .
  fi
  if [[ $BINARY == *-radio ]]; then
    [[ ! -d seeds-radio ]] && mkdir seeds-radio
    cp ../seeds-radio/* ./seeds-radio -r
    SEED=seeds-radio
  fi
  exec_cmd="$CMD -L 0 -t 25000 -i ${SEED} -o corpus -m none"
  if ls ./*.dict 2>/dev/null; then
    dict_path="$(find . -maxdepth 1 -name "*.dict" | head -n 1)"
    exec_cmd="${exec_cmd} -x ${dict_path}@9"
  fi
  #AFL_PERSISTENT=1 $exec_cmd -- ./${BINARY}
  $exec_cmd -- ./${BINARY} @@
elif [[ $BINARY == *-libfuzzer ]]; then
  rm -rf corpus
  cp -R seeds corpus
  exec_cmd="./$BINARY corpus"
  $exec_cmd
fi
popd
