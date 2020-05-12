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
# @param $1 - message to print.
################################################################################
print_message() {
	echo    "=========================================================================================="
	echo -e "\033[31m ${1} \033[0m"
	echo    "=========================================================================================="
}

################################################################################
#cd x264-20150817-2245-stable
cd x264-snapshot-20170705-2245

print_message "Start building libx264 for iOS arm32"
./build_apple.sh ios_arm32
check_result "$?" "build libx264 for iOS arm32"
print_message "Finished building libx264 for iOS arm32"

print_message "Start building libx264 for iOS arm64"
./build_apple.sh ios_arm64
check_result "$?" "build libx264 for iOS arm64"
print_message "Finished building libx264 for iOS arm64"

print_message "Start building libx264 for iOS i386"
./build_apple.sh ios_i386
check_result "$?" "build libx264 for iOS i386"
print_message "Finished building libx264 for iOS i386"

print_message "Start building libx264 for iOS x86_64"
./build_apple.sh ios_x86_64
check_result "$?" "build libx264 for iOS x86_64"
print_message "Finished building libx264 for iOS x86_64"

print_message "Start lipo libx264 for ios"  
mkdir -p "./universal/lib"
xcrun -sdk iphoneos lipo -output ./universal/lib/libx264.a  -create -arch armv7 ../../libs-apple-ios-arm32/libx264/lib/libx264.a -arch arm64 ../../libs-apple-ios-arm64/libx264/lib/libx264.a -arch i386 ../../libs-apple-ios-i386/libx264/lib/libx264.a -arch x86_64 ../../libs-apple-ios-x86-64/libx264/lib/libx264.a

LIB_OUTPUT_PATH="libs-apple-ios-universal"
IOS_PROJ_OUTPUT_PATH="../../../libs/apple/universal"
mkdir -p "${IOS_PROJ_OUTPUT_PATH}"

cp -r ../../libs-apple-ios-arm32/libx264/include ${IOS_PROJ_OUTPUT_PATH}/../../include
cp -r ./universal/lib/* ${IOS_PROJ_OUTPUT_PATH}/

print_message "Finished lipo libx264 for ios"

print_message "Start building libx264 for MacOSX osx_x64"
./build_apple.sh osx_x64
check_result "$?" "build libx264 for MacOSX osx_x64"
print_message "Finished building libx264 for MacOSX osx_x64"

cp  "../../libs-apple-osx/libx264/lib/libx264.a"     "../../../libs/apple/osx/"



cd -
