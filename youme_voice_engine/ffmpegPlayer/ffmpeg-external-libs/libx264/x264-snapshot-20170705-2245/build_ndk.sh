#!/bin/bash

if [[ "$1" != "armneon" && "$1" != "arm" && "$1" != "x86" && "$1" != "arm64-v8a" && "$1" != "x86_64" ]]; then
  echo "usage: $0 <armneon|arm|x86|arm64-v8a|x86_64>"
  exit 1
fi

source ../../env_setup_ndk.sh $1

# Overload the "AS" to "gcc", so that it can handle C preprocessor and comments.
export AS=${CROSS_PREFIX}gcc

# CPU architecture related options.
if [ "$1" = "armneon" ]; then
  export CFLAGS="-mcpu=cortex-a8 -march=armv7-a -mfloat-abi=softfp -mfpu=neon -fpic"
  export ASFLAGS="-mcpu=cortex-a8 -march=armv7-a -mfloat-abi=softfp -mfpu=neon -fpic"
elif [ "$1" = "arm" ]; then
  export CFLAGS="-mcpu=cortex-a8 -mfloat-abi=soft -fpic"
  export ASFLAGS="-mcpu=cortex-a8 -mfloat-abi=soft -fpic"
elif [ "$1" = "x86" ]; then
  export CFLAGS="-fpic"
  export ASFLAGS="-fpic"
  EXTRA_CONFIG="--disable-asm"
elif [ "$1" = "arm64-v8a" ]; then
  export CFLAGS="-march=armv8-a"
  export ASFLAGS="-march=armv8-a"
elif [ "$1" = "x86_64" ]; then
  export CFLAGS="-fpic"
  export ASFLAGS="-fpic"
  EXTRA_CONFIG="--disable-asm"
fi

./configure \
  --chroma-format=420 \
  --enable-pic \
  --disable-cli \
  --sysroot="${PLATFORM}" \
  --host=${HOST} \
  --enable-static \
  --disable-opencl \
  --prefix="${INSTALL_PREFIX}/lib" \
  --cross-prefix="${CROSS_PREFIX}" \
  --extra-cflags="-I${PLATFORM}/usr/include -O3" \
  --extra-ldflags="-L${PLATFORM}/usr/lib" \
  ${EXTRA_CONFIG}


check_result "$?" "configure"

make clean

make -j8
check_result "$?" "make"

make install

rm -rf "${INSTALL_PREFIX}/lib/libx264"
mv "${INSTALL_PREFIX}/lib/lib" "${INSTALL_PREFIX}/lib/libx264"

check_result "$?" "install"
