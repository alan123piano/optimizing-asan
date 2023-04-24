#!/bin/bash

# Example usage: ./run.sh -bo hw2perf1 > hw2perf1_opt.txt

set -Eeuo pipefail

# b = view LLVM bytecode
# o = run OptimizeASan pass
VIEW_BYTECODE=0
RUN_OPT_ASAN=0
while getopts "bo" opt; do
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

TESTCASE=$1

# Convert source code into LLVM bytecode.
clang -Xclang -disable-O0-optnone -emit-llvm  $TESTCASE.cpp -c -o $TESTCASE.bc

# Canonicalize natural loops.
opt -passes='loop-simplify' $TESTCASE.bc -o $TESTCASE.out.bc
mv $TESTCASE.out.bc $TESTCASE.bc

# Add profiling instrumentation.
opt -passes='pgo-instr-gen,instrprof' $TESTCASE.bc -o $TESTCASE.prof.bc

# Generate executable from profiling code.
clang -fprofile-instr-generate -x ir $TESTCASE.prof.bc -o $TESTCASE.prof.exe

# Run profiler-embedded executable, which generates a default.profraw file.
# We discard the output into /dev/null.
./$TESTCASE.prof.exe > /dev/null

# Convert profiling data into LLVM form.
llvm-profdata merge -o $TESTCASE.profdata default.profraw

# The "Profile Guided Optimization Use" pass attaches the profile data to the .bc file.
opt -passes="pgo-instr-use" -o $TESTCASE.bc -pgo-test-profile-file=$TESTCASE.profdata < $TESTCASE.prof.bc > /dev/null

# Now, our .bc file is augmented with the profile data.
# Below, we run our own passes.

# Run mem2reg.
opt -mem2reg $TESTCASE.bc -o $TESTCASE.out.bc
mv $TESTCASE.out.bc $TESTCASE.bc

if [ "$RUN_OPT_ASAN" -eq 1 ]; then
    # Run OptimizeASan.
    opt -enable-new-pm=0 -load build/optimize_asan/LLVMPJT_OPTIMIZE_ASAN.so -optimize_asan < $TESTCASE.bc > $TESTCASE.out.bc
    mv $TESTCASE.out.bc $TESTCASE.bc
fi

# Run ASan instrumentation.
opt -enable-new-pm=0 -load build/asan/LLVMPJT_ASAN.so -asan < $TESTCASE.bc > $TESTCASE.out.bc
mv $TESTCASE.out.bc $TESTCASE.bc

if [ "$VIEW_BYTECODE" -eq 1 ]; then
    # Show bytecode.
    llvm-dis $TESTCASE.bc -o -
else
    # Compile executable and benchmark its performance.
    clang -lasan -x ir $TESTCASE.bc -o $TESTCASE.exe
    time ./$TESTCASE.exe > /dev/null
fi
;;
