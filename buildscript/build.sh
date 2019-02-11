#!/bin/bash
# Usage: build.sh [root_directory] [compiler_flags]
# c++ command should point to gcc or clang

root="${1:-..}/"
shift

function is_android {
  [[ $(uname -o) = *"Android"* ]]
  return $?
}

COL_YELLOW=$(echo -en "\e[93m")
COL_BLUE=$(echo -en "\e[34m")
COL_DEFAULT=$(echo -en "\e[0m")

function print_yellow {
  echo "${COL_YELLOW}$@${COL_DEFAULT}"
}

function print_blue {
  echo "${COL_BLUE}$@${COL_DEFAULT}"
}

function run_verbose {
  (set -x; "$@")
}


clang_flags="-std=c++17 -stdlib=libc++ -fexceptions $@"

print_blue "Using ${clang_flags}"

if is_android
then
  android_linker_flags="-latomic"
  print_blue "Using ${android_linker_flags}"
fi

print_yellow "Compiling editorlib"
run_verbose c++ ${clang_flags} -I"${root}third_party/gsl-2.0.0" -I"${root}third_party/tl" -I"${root}third_party/nlohmann-json-3.4.0" -I"${root}third_party/mk_wcswidth" -I"${root}utilities" -c "${root}editorlib/"*.cpp

print_yellow "Compiling utilities"
run_verbose c++ ${clang_flags} -c "${root}utilities/"*.cpp

print_yellow "Compiling mk_wcswidth"
run_verbose c++ ${clang_flags} -c "${root}third_party/mk_wcswidth/mk_wcswidth/"*.cpp

print_yellow "Compiling text_ui"
run_verbose c++ ${clang_flags} -I"${root}third_party/gsl-2.0.0" -I"${root}third_party/tl" -I"${root}utilities" -I"${root}editorlib" -c "${root}text_ui/"*.cpp

print_yellow "Compiling editor"
run_verbose c++ ${clang_flags} -I"${root}third_party/gsl-2.0.0" -I"${root}third_party/tl" -I"${root}utilities" -I"${root}text_ui" -I"${root}editorlib" -c "${root}editor/"*.cpp

print_yellow "Linking"
run_verbose c++ -o terminal_editor *.o ${android_linker_flags}
