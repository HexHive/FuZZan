#!/bin/bash -e
ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
LLVM_DIR=$ROOT/../../../LLVM/build/bin
AFL_PATH=$ROOT/FTS/AFL
export PATH=$LLVM_DIR:$PATH

export MINMODE_ON="0"
export MIN_SCRIPT_PATH=""

sudo sh -c 'echo core | sudo tee /proc/sys/kernel/core_pattern'
sudo sh -c 'cd /sys/devices/system/cpu; echo performance | sudo tee cpu*/cpufreq/scaling_governor; cd -'
sudo sh -c 'echo 1 | sudo tee /proc/sys/kernel/sched_child_runs_first'
sudo sysctl -w kernel.randomize_va_space=0

BINARY=${1##*/}

pushd $(dirname $1)

[[ ! -d seeds ]] && mkdir seeds
[[ ! $(find seeds -type f) ]] && echo > ./seeds/nil_seed

rm -rf corpus
mkdir -p corpus

export ASAN_OPTIONS="abort_on_error=1:detect_leaks=0:symbolize=0"

set -x

if [[ $BINARY == *-afl* ]]; then
  CMD=$AFL_PATH/afl-fuzz
  SEED=seeds
  if [[ $BINARY == *-radio ]]; then
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
