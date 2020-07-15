#!/usr/bin/env bash
rpath="$(readelf -d "$1" | grep RUNPATH | sed 's/^.*\[\(.*\)\]$/\1/')"
export LD_LIBRARY_PATH="$rpath:$LD_LIBRARY_PATH"
ulimit -s 8192
exec $@
