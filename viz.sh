#!/bin/bash

# Usage: viz.sh hw2correctN or viz.sh hw2correctN [TYPE]
# TYPE should be one of: cfg, cfg-only, dom, dom-only, postdom, postdom-only.
# Default type is cfg.
set -Eeuo pipefail

TESTCASE=$1

# Default to cfg
VIZ_TYPE=${2:-cfg}

CURR=$(pwd)
TMP_DIR=$(realpath ./tmp) # will put .dot files here

mkdir -p $TMP_DIR
cd $TMP_DIR

# If profile data available, use it
PROF_FLAGS=""
PROF_DATA=$CURR/$TESTCASE.profdata
if [ $VIZ_TYPE = "cfg" ]; then
  if [ -f $PROF_DATA ]; then
    echo "Using prof data in visualization"
    PROF_FLAGS="-cfg-weights"
  else
    echo "No prof data, not including it in visualization"
  fi
fi

BITCODE=$CURR/$TESTCASE.bc

# Generate .dot files in tmp dir
opt $PROF_FLAGS -enable-new-pm=0 -dot-$VIZ_TYPE $BITCODE > /dev/null

# Combine .dot files into PDF
DOT_FILES=$(ls -A $TMP_DIR)
cat $DOT_FILES | dot -Tpdf > $CURR/$TESTCASE.$VIZ_TYPE.pdf
echo "Created $TESTCASE.$VIZ_TYPE.pdf"

rm -r $TMP_DIR