#!/bin/bash
set -euo pipefail

base_path="$(cd "$(dirname "$0")" && pwd)"
cd "$base_path"

mkdir -p bin

need_rebuild=0
if [ ! -x ./bin/build ]; then
    need_rebuild=1
elif [ build.c -nt ./bin/build ] || [ code/build_framework.h -nt ./bin/build ]; then
    need_rebuild=1
fi

if [ "$need_rebuild" -eq 1 ]; then
    echo "[build] compiling bin/build"
    cc -std=c11 -O2 -o bin/build build.c
fi

exec ./bin/build "$@"
