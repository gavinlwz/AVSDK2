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
if [ -z "${NDK_ROOT}" ]; then
	echo    "=========================================================================================="
	echo -e "\033[31m Error: ANDROID_NDK_ROOT is NOT set!!! \033[0m"
	echo -e "\033[31m Please set it in ~/.bash_profile to point to your NDK root path, and restart the terminal!!! \033[0m"
	echo    "=========================================================================================="
	exit 1
fi

FFMPEG_ANDROID_NDK_VERSION=10

echo
echo    "=========================================================================================="
echo -e "\033[31m ANDROID_NDK_ROOT:    ${NDK_ROOT} \033[0m"
echo -e "\033[31m FFMPEG_ANDROID_NDK_VERSION: ${FFMPEG_ANDROID_NDK_VERSION} \033[0m"
echo    "=========================================================================================="
echo

if [ "$FFMPEG_ANDROID_NDK_VERSION" = "8" ]; then
    GCCVER=4.4.3
    PREBUILT_ARM=${NDK_ROOT}/toolchains/arm-linux-androideabi-${GCCVER}/prebuilt/darwin-x86
    PLATFORM_ARM=${NDK_ROOT}/platforms/android-9/arch-arm
    PREBUILT_X86=${NDK_ROOT}/toolchains/x86-${GCCVER}/prebuilt/darwin-x86
    PLATFORM_X86=${NDK_ROOT}/platforms/android-9/arch-x86
fi

if [ "$FFMPEG_ANDROID_NDK_VERSION" = "9" ]; then
    GCCVER=4.8
    PREBUILT_ARM=${NDK_ROOT}/toolchains/arm-linux-androideabi-${GCCVER}/prebuilt/darwin-x86_64
    PLATFORM_ARM=${NDK_ROOT}/platforms/android-9/arch-arm
    PREBUILT_X86=${NDK_ROOT}/toolchains/x86-${GCCVER}/prebuilt/darwin-x86_64
    PLATFORM_X86=${NDK_ROOT}/platforms/android-9/arch-x86
fi

if [ "$FFMPEG_ANDROID_NDK_VERSION" = "10" ]; then
    GCCVER=4.9
    PREBUILT_ARM=${NDK_ROOT}/toolchains/arm-linux-androideabi-${GCCVER}/prebuilt/darwin-x86_64
    PLATFORM_ARM=${NDK_ROOT}/platforms/android-9/arch-arm
    PREBUILT_X86=${NDK_ROOT}/toolchains/x86-${GCCVER}/prebuilt/darwin-x86_64
    PLATFORM_X86=${NDK_ROOT}/platforms/android-9/arch-x86
	
	GCCVER_ARCH64=4.9
	PREBUILT_ARM64=${NDK_ROOT}/toolchains/aarch64-linux-android-${GCCVER_ARCH64}/prebuilt/darwin-x86_64
	PLATFORM_ARM64=${NDK_ROOT}/platforms/android-21/arch-arm64
	PREBUILT_X86_64=${NDK_ROOT}/toolchains/x86_64-${GCCVER_ARCH64}/prebuilt/darwin-x86_64
	PLATFORM_X86_64=${NDK_ROOT}/platforms/android-21/arch-x86_64
fi


if [ "$1" = "x86" ]; then
	PREBUILT=${PREBUILT_X86}
	PLATFORM=${PLATFORM_X86}
	CROSS_PREFIX=${PREBUILT}/bin/i686-linux-android-
	INSTALL_PREFIX=$(pwd)/../../../libs/android/x86
	export HOST=i686-linux-android
elif [ "$1" = "armneon" ]; then
	PREBUILT=${PREBUILT_ARM}
	PLATFORM=${PLATFORM_ARM}
	CROSS_PREFIX=${PREBUILT}/bin/arm-linux-androideabi-
	INSTALL_PREFIX=$(pwd)/../../../libs/android/armeabi-v7a
	export HOST=arm-linux-androideabi
elif [ "$1" = "arm" ]; then
	PREBUILT=${PREBUILT_ARM}
	PLATFORM=${PLATFORM_ARM}
	CROSS_PREFIX=${PREBUILT}/bin/arm-linux-androideabi-
	INSTALL_PREFIX=$(pwd)/../../../libs/android/armeabi
	export HOST=arm-linux-androideabi
elif [ "$1" = "arm64-v8a" ]; then
	PREBUILT=${PREBUILT_ARM64}
	PLATFORM=${PLATFORM_ARM64}
	CROSS_PREFIX=${PREBUILT}/bin/aarch64-linux-android-
	INSTALL_PREFIX=$(pwd)/../../../libs/android/arm64-v8a
	export HOST=aarch64-linux-android
elif [ "$1" = "x86_64" ]; then
	PREBUILT=${PREBUILT_X86_64}
	PLATFORM=${PLATFORM_X86_64}
	CROSS_PREFIX=${PREBUILT}/bin/x86_64-linux-android-
	INSTALL_PREFIX=$(pwd)/../../../libs/android/x86_64
	export HOST=x86_64-linux-android
fi

export CC=${CROSS_PREFIX}gcc
export CXX=${CROSS_PREFIX}g++
export CPP=${CROSS_PREFIX}cpp
export CXXCPP=${CROSS_PREFIX}cpp
export AR=${CROSS_PREFIX}ar
export LD=${CROSS_PREFIX}ld
export AS=${CROSS_PREFIX}as
export NM=${CROSS_PREFIX}nm
export STRIP=${CROSS_PREFIX}strip
export RANLIB=${CROSS_PREFIX}ranlib
export OBJDUMP=${CROSS_PREFIX}objdump
