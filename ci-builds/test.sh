#!/usr/bin/env bash

# quit on errors
set -e

pushd `pwd`

echo "Testing..."
./build-zz/tests/editor-tests
echo "Testing done."

popd
