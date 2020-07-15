#!/bin/bash
prelink_dir="prelink-$1"
min_mode=$2
echo $prelink_dir + $min_mode

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

[  $# -lt 3 ] && { echo "Usage: $0 out_dir min_mode binary [args]"; exit 1; }
prog=$3
shift
if [ ! -f $prog ]; then
    prog=`which "$prog"`
    [ ! -f $prog ] && { echo "Could not find $prog here nor in PATH"; exit 1; }
fi

"$DIR/prelink_binary.py" --set-rpath "$prog" --preload-lib "$DIR/libshrink-preload.so" --out-dir "$prelink_dir" --min-shadow-mode "$min_mode"

#export ASAN_OPTIONS=detect_leaks=0
#newprog="$prelink_dir/`basename "$prog"`"
#export LD_LIBRARY_PATH="`pwd`/$prelink_dir:$LD_LIRARY_PATH"
#export LD_PRELOAD="`pwd`/$prelink_dir/libshrink-preload.so"
#exec "$newprog" "$@"
