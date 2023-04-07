#!/bin/bash

set -Eeuo pipefail

case "$1" in
    "pass") # build passes
        cmake -B build
        cd build
        cmake ..
        cmake --build .
        ;;
    "opt") # run LLVM passes using opt
        clang -emit-llvm hello.cpp -c -o - |
        # opt -enable-new-pm=0 -load build/mypass/LLVMPJT_MYPASS.so -hello |
        opt -enable-new-pm=0 -load build/asan/LLVMPJT_ASAN.so -asan |
        llvm-dis -o -
        # clang -x ir - -o hello -g
        ;;
    *)
        echo "Invalid usage"
        exit 1
        ;;
esac
