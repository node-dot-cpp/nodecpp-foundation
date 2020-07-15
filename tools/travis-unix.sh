#!/bin/sh
set -ev

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE=ON ..
cmake --build .
ctest --output-on-failure
