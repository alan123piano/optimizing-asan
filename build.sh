#!/bin/bash

set -Eeuo pipefail

cmake -B build
cd build
cmake ..
cmake --build .