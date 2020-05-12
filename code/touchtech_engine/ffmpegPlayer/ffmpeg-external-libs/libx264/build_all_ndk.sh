#!/bin/bash

################################################################################
# @param $1 - result of execution
# @param $2 - operation type that caused the error
################################################################################
check_result() {
  if [[ $1 != 0 ]]; then
    echo
    echo -e "\033[31m Error: Failed to ${2}!!! \033[0m"
    echo
    exit 1
  fi
}

################################################################################
#cd x264-20150817-2245-stable
cd x264-snapshot-20170705-2245

./build_ndk.sh armneon
check_result "$?" "build libx264 for ARM NEON"
./build_ndk.sh arm
check_result "$?" "build libx264 for ARM"
./build_ndk.sh x86
check_result "$?" "build libx264 for x86"
./build_ndk.sh x86_64
check_result "$?" "build libx264 for x86_64"
./build_ndk.sh arm64-v8a
check_result "$?" "build libx264 for arm64-v8a"

cd -
