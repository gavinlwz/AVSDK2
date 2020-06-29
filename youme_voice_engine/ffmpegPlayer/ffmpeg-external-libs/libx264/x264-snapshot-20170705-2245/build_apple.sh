#!/bin/bash

if [[ "$1" != "ios_arm32"
        && "$1" != "ios_arm64"
        && "$1" != "ios_i386"
        && "$1" != "ios_x86_64"
        && "$1" != "osx_x64"
        && "$1" != "osx" ]]; then
  echo "usage: $0 <ios_arm32|ios_arm64|ios_i386|ios_x86_64|osx|osx_x64>"
  exit 1
fi

source ../../env_setup_apple.sh $1

# Overload the "AS" to "gcc", so that it can handle C preprocessor and comments.

# CPU architecture related options.
if [ "$1" = "ios_arm32" ]; then
    export AS="gas-preprocessor.pl ${CC}"
    export CFLAGS="-mcpu=cortex-a8 -mfpu=neon -arch armv7 -miphoneos-version-min=8.0 -fembed-bitcode-marker -fpic"
    export ASFLAGS="-mcpu=cortex-a8 -mfpu=neon -arch armv7 -miphoneos-version-min=8.0 -fembed-bitcode-marker -fpic"
    export LDFLAGS="-mcpu=cortex-a8 -mfpu=neon -arch armv7 -miphoneos-version-min=8.0"
    export CXXFLAGS="-mcpu=cortex-a8 -mfpu=neon -arch armv7 -miphoneos-version-min=8.0 -fembed-bitcode-marker -fpic"
elif [ "$1" = "ios_arm64" ]; then
    export AS="gas-preprocessor.pl ${CC}"
    export CFLAGS="-mfpu=neon -arch arm64 -miphoneos-version-min=8.0 -fembed-bitcode-marker -fpic"
    export ASFLAGS="-mfpu=neon -arch arm64 -miphoneos-version-min=8.0 -fembed-bitcode-marker -fpic"
    export LDFLAGS="-mfpu=neon -arch arm64 -miphoneos-version-min=8.0"
    export CXXFLAGS="-mfpu=neon -arch arm64 -miphoneos-version-min=8.0 -fembed-bitcode-marker -fpic"
elif [ "$1" = "ios_i386" ]; then
    export AS="${CC}"
    export CFLAGS="-arch i386 -mios-simulator-version-min=8.0 -fpic"
    export ASFLAGS="-arch i386 -mios-simulator-version-min=8.0 -fpic"
    export LDFLAGS="-arch i386 -mios-simulator-version-min=8.0 -fpic"
    EXTRA_CONFIG="--disable-asm"
elif [ "$1" = "ios_x86_64" ]; then
    export AS="${CC}"
    export CFLAGS="-arch x86_64 -mios-simulator-version-min=8.0 -fpic"
    export ASFLAGS="-arch x86_64 -mios-simulator-version-min=8.0 -fpic"
    export LDFLAGS="-arch x86_64 -mios-simulator-version-min=8.0 -fpic"
    EXTRA_CONFIG="--disable-asm"

elif [ "$1" = "osx_x64" ]; then
    export AS="${CC}"
    export CFLAGS="-arch x86_64"
    export ASFLAGS="-arch x86_64"
    export LDFLAGS="-arch x86_64 "
    EXTRA_CONFIG="--disable-asm"
fi

./configure \
  --chroma-format=420 \
  --enable-pic \
  --sysroot="${PLATFORM_SDK}" \
  --host=${HOST} \
  --enable-static \
  --enable-shared \
  --disable-opencl \
  --cross-prefix="${CROSS_PREFIX}" \
  --prefix="${INSTALL_PREFIX}/libx264" \
  --extra-cflags="-I${PLATFORM_SDK}/usr/include" \
  --extra-ldflags="-L${PLATFORM_SDK}/usr/lib" \
  --disable-swscale \
  --enable-strip \
  ${EXTRA_CONFIG}

check_result "$?" "configure"

make clean

make
check_result "$?" "make"

make install
check_result "$?" "install"

cp -rf ${INSTALL_PREFIX} ${OUTPUT_PATH}
check_result "$?" "copying libs to PlayerCore"