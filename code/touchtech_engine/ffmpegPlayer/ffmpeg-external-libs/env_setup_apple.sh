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
# This script is "sourced" by the actual building script. So the base directory
# of this script is different from the working directory.
BASE_DIR=$(pwd)/../..
OUTPUT_PATH=${BASE_DIR}/../Core/jni/Prj_iOS/Lib/libFFmpeg-2.2.3-extLibs
mkdir -p ${OUTPUT_PATH}

IOS_SDK=$(xcodebuild -showsdks | grep iphoneos | sort | head -n 1 | awk '{print $NF}')
SIM_SDK=$(xcodebuild -showsdks | grep iphonesimulator | sort | head -n 1 | awk '{print $NF}')
MACOSX_SDK=$(xcodebuild -showsdks | grep -v DriverKit | grep macosx | sort | head -n 1 | awk '{print $NF}')



SYSTEM_BASE=/Applications/Xcode.app/Contents/Developer
if [ "$1" = "ios_arm32" ]; then
	PLATFORM_BASE=${SYSTEM_BASE}/Platforms/iPhoneOS.platform/Developer
	PLATFORM_SDK=${PLATFORM_BASE}/SDKs/${IOS_SDK}.sdk
	CROSS_PREFIX=${SYSTEM_BASE}/Toolchains/XcodeDefault.xctoolchain/usr/bin/
	INSTALL_PREFIX=${BASE_DIR}/libs-apple-ios-arm32
	export HOST=arm-apple-darwin
elif [ "$1" = "ios_arm64" ]; then
    PLATFORM_BASE=${SYSTEM_BASE}/Platforms/iPhoneOS.platform/Developer
    PLATFORM_SDK=${PLATFORM_BASE}/SDKs/${IOS_SDK}.sdk
    CROSS_PREFIX=${SYSTEM_BASE}/Toolchains/XcodeDefault.xctoolchain/usr/bin/
    INSTALL_PREFIX=${BASE_DIR}/libs-apple-ios-arm64
    export HOST=aarch64-apple-darwin
elif [ "$1" = "ios_i386" ]; then
	PLATFORM_BASE=${SYSTEM_BASE}/Platforms/iPhoneSimulator.platform/Developer
	PLATFORM_SDK=${PLATFORM_BASE}/SDKs/${SIM_SDK}.sdk
	CROSS_PREFIX=${SYSTEM_BASE}/Toolchains/XcodeDefault.xctoolchain/usr/bin/
	INSTALL_PREFIX=${BASE_DIR}/libs-apple-ios-i386
	export HOST=i386-apple-darwin
elif [ "$1" = "ios_x86_64" ]; then
    PLATFORM_BASE=${SYSTEM_BASE}/Platforms/iPhoneSimulator.platform/Developer
    PLATFORM_SDK=${PLATFORM_BASE}/SDKs/${SIM_SDK}.sdk
    CROSS_PREFIX=${SYSTEM_BASE}/Toolchains/XcodeDefault.xctoolchain/usr/bin/
    INSTALL_PREFIX=${BASE_DIR}/libs-apple-ios-x86-64
    export HOST=i386-apple-darwin
elif [ "$1" = "osx_x64" ]; then
    PLATFORM_BASE=${SYSTEM_BASE}/Platforms/MacOSX.platform/Developer
    PLATFORM_SDK=${PLATFORM_BASE}/SDKs/${MACOSX_SDK}.sdk  
    CROSS_PREFIX=${SYSTEM_BASE}/Toolchains/XcodeDefault.xctoolchain/usr/bin/
    INSTALL_PREFIX=${BASE_DIR}/libs-apple-osx
    #export HOST=aarch64-apple-darwin
fi

export CC=${SYSTEM_BASE}/usr/bin/gcc
export CXX=${SYSTEM_BASE}/usr/bin/g++
export CPP=${SYSTEM_BASE}/usr/bin/gcc
export CXXCPP=${SYSTEM_BASE}/usr/bin/gcc
export LD=${SYSTEM_BASE}/usr/bin/ld
export AR=${CROSS_PREFIX}ar
export AS=${CROSS_PREFIX}as
export NM=${CROSS_PREFIX}nm
export STRIP=${CROSS_PREFIX}strip
export RANLIB=${CROSS_PREFIX}ranlib
export OBJDUMP=${CROSS_PREFIX}objdump

# The path set here is for accessing some extra tools such as gas-preprocessor.pl
export PATH=${PATH}:${BASE_DIR}