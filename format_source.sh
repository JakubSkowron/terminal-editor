#!/bin/bash

if [[ $# == 0 ]]; then
  echo "Pass source file names in parameters"
  exit 1
fi

clang-format --style=file -i "$@"
