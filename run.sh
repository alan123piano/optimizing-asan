#!/bin/bash

set -Eeuo pipefail

# b = view LLVM bytecode
# o = run OptimizeASan pass
VIEW_BYTECODE=0
RUN_OPT_ASAN=0
while getopts "bao" opt; do
    case $opt in
        b)
            VIEW_BYTECODE=1
            ;;
        o)
            RUN_OPT_ASAN=1
            ;;
    esac
done

shift $((OPTIND - 1))

case "$1" in
    "build") # build LLVM passes
        cmake -B build
        cd build
        cmake ..
        cmake --build .
        ;;
    "test")
	#clang -emit-llvm $2.cpp -c -o $2.llvmbc
	clang -Xclang -disable-O0-optnone -emit-llvm  $2.cpp -c -o $2.llvmbc
	opt -mem2reg $2.llvmbc -o $2.out.llvmbc
	mv $2.out.llvmbc $2.llvmbc
	opt -loop-rotate $2.llvmbc -o $2.out.llvmbc
	mv $2.out.llvmbc $2.llvmbc
        if [ "$RUN_OPT_ASAN" -eq 1 ]; then
            opt -enable-new-pm=0 -load build/optimize_asan/LLVMPJT_OPTIMIZE_ASAN.so -optimize_asan < $2.llvmbc > $2.out.llvmbc
            mv $2.out.llvmbc $2.llvmbc
        fi
        opt -enable-new-pm=0 -load build/asan/LLVMPJT_ASAN.so -asan < $2.llvmbc > $2.out.llvmbc
        mv $2.out.llvmbc $2.llvmbc
        if [ "$VIEW_BYTECODE" -eq 1 ]; then
            llvm-dis $2.llvmbc -o -
        else
            clang -O0 -lasan -x ir $2.llvmbc -o $2.exe
            time ./$2.exe > /dev/null
        fi
        ;;
    *)
        echo "Invalid usage"
        exit 1
        ;;
esac
