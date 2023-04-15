#!/bin/bash

#set -Eeuo pipefail

case "$1" in
    "pass") # build passes
        cmake -B build
        cd build
        cmake ..
        cmake --build .
        ;;
    "opt") # run LLVM passes using opt
        clang -emit-llvm $2.cpp -c -o - |
        opt -enable-new-pm=0 -load build/mypass/LLVMPJT_OPTIMIZE_ASAN.so -optimize_asan |
        opt -enable-new-pm=0 -load build/asan/LLVMPJT_ASAN.so -asan |
        # llvm-dis -o -
        clang -lasan -x ir - -o $2
        ;;
    *)
        echo "Invalid usage"
        exit 1
        ;;
esac
