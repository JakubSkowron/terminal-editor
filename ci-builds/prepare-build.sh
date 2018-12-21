#!/usr/bin/env bash

#this script assumes, that:
#  - COMPILER variable is set to proper compiler

# quit on errors
set -e

pushd `pwd`

echo "Running CMake with COMPILER=$COMPILER"

export CXX="$COMPILER"
#export CC="clang-3.7"

mkdir build-zz
cd build-zz

cmake ..

popd
