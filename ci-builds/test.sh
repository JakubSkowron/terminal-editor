#!/usr/bin/env bash

# quit on errors
set -e

pushd `pwd`

echo "Testing..."
cd ./tests
../build-zz/tests/editor-tests
echo "Testing done."

popd
