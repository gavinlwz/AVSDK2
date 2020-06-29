#! /usr/bin/env bash
#
# Copyright (C) 2013-2014 Zhang Rui <bbcallen@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# This script is based on projects below
# https://github.com/kolyvan/kxmovie
# https://github.com/yixia/FFmpeg-Android
# http://git.videolan.org/?p=vlc-ports/android.git;a=summary
# https://github.com/kewlbear/FFmpeg-iOS-build-script/

#--------------------
echo "===================="
echo "[*] check host"
echo "===================="
set -e

#--------------------
# include


#--------------------
# common defines
FF_ARCH=$1

FF_BUILD_OPT=$2
echo "FF_ARCH=$FF_ARCH"
echo "FF_BUILD_OPT=$FF_BUILD_OPT"
if [ -z "$FF_ARCH" ]; then
    echo "You must specific an architecture 'armv7, armv7s, arm64, i386, x86_64,mac_x64, ...'.\n"
    exit 1
fi


FF_BUILD_ROOT=`pwd`
FF_TAGET_OS="darwin"


# ffmpeg build params
export COMMON_FF_CFG_FLAGS=
source $FF_BUILD_ROOT//module.sh

FF_INSTALL_LIBS=
FF_INSTALL_INCLUDE=

FFMPEG_CFG_FLAGS=
FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS $COMMON_FF_CFG_FLAGS"

# Optimization options (experts only):
# FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --disable-armv5te"
# FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --disable-armv6"
# FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --disable-armv6t2"

# Advanced options (experts only):
FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --enable-cross-compile"
# --disable-symver may indicate a bug
# FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --disable-symver"

# Developer options (useful when working on FFmpeg itself):
FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --enable-stripping"

##
FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --target-os=$FF_TAGET_OS"
FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --enable-static"
FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --disable-shared"
FFMPEG_EXTRA_CFLAGS=

# i386, x86_64
FFMPEG_CFG_FLAGS_SIMULATOR=
FFMPEG_CFG_FLAGS_SIMULATOR="$FFMPEG_CFG_FLAGS_SIMULATOR --disable-asm"
FFMPEG_CFG_FLAGS_SIMULATOR="$FFMPEG_CFG_FLAGS_SIMULATOR --disable-mmx"
FFMPEG_CFG_FLAGS_SIMULATOR="$FFMPEG_CFG_FLAGS_SIMULATOR --assert-level=2"

# armv7, armv7s, arm64
FFMPEG_CFG_FLAGS_ARM=
FFMPEG_CFG_FLAGS_ARM="$FFMPEG_CFG_FLAGS_ARM --enable-pic"
FFMPEG_CFG_FLAGS_ARM="$FFMPEG_CFG_FLAGS_ARM --enable-neon"
case "$FF_BUILD_OPT" in
    debug)
        FFMPEG_CFG_FLAGS_ARM="$FFMPEG_CFG_FLAGS_ARM --disable-optimizations"
        FFMPEG_CFG_FLAGS_ARM="$FFMPEG_CFG_FLAGS_ARM --enable-debug"
        FFMPEG_CFG_FLAGS_ARM="$FFMPEG_CFG_FLAGS_ARM --disable-small"
    ;;
    *)
        FFMPEG_CFG_FLAGS_ARM="$FFMPEG_CFG_FLAGS_ARM --enable-optimizations"
        FFMPEG_CFG_FLAGS_ARM="$FFMPEG_CFG_FLAGS_ARM --disable-debug"
        FFMPEG_CFG_FLAGS_ARM="$FFMPEG_CFG_FLAGS_ARM --enable-small"
    ;;
esac

echo "build_root: $FF_BUILD_ROOT"

#--------------------
echo "===================="
echo "[*] check gas-preprocessor"
echo "===================="
FF_TOOLS_ROOT="$FF_BUILD_ROOT"
export PATH="$FF_TOOLS_ROOT/gas-preprocessor:$PATH"

echo "gasp: $FF_TOOLS_ROOT/gas-preprocessor/gas-preprocessor.pl"

#--------------------
echo "===================="
echo "[*] config arch $FF_ARCH"
echo "===================="

FF_BUILD_NAME="unknown"
FF_XCRUN_PLATFORM="iPhoneOS"
FF_XCRUN_OSVERSION=
FF_GASPP_EXPORT=
FF_DEP_OPENSSL_INC=
FF_DEP_OPENSSL_LIB=
FF_XCODE_BITCODE=

X264_INCLUDE=
X264_LIBS=

FDK_AAC_INCLUDE=
FDK_AAC_LIBS=

if [ "$FF_ARCH" = "i386" ]; then
    FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --arch=$FF_ARCH"
    FF_BUILD_NAME="ffmpeg-i386"
    FF_BUILD_NAME_OPENSSL=openssl-i386
    FF_XCRUN_PLATFORM="iPhoneSimulator"
    FF_XCRUN_OSVERSION="-mios-simulator-version-min=8.0"
    FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS $FFMPEG_CFG_FLAGS_SIMULATOR"
    FF_INSTALL_LIBS=$FF_BUILD_ROOT/../../libs/apple/ios_i386/lib
    FF_INSTALL_INCLUDE=$FF_BUILD_ROOT/../../libs/apple/ios_i386/include
   
    X264_INCLUDE=$FF_BUILD_ROOT/../../ffmpeg-external-libs/libs-apple-ios-i386/libx264/include
    X264_LIBS=$FF_BUILD_ROOT/../../ffmpeg-external-libs/libs-apple-ios-i386/libx264/lib

elif [ "$FF_ARCH" = "x86_64" ]; then
    FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --arch=$FF_ARCH"
    FF_BUILD_NAME="ffmpeg-x86_64"
    FF_BUILD_NAME_OPENSSL=openssl-x86_64
    FF_XCRUN_PLATFORM="iPhoneSimulator"
    FF_XCRUN_OSVERSION="-mios-simulator-version-min=8.0"
    FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS $FFMPEG_CFG_FLAGS_SIMULATOR"
    FF_INSTALL_LIBS=$FF_BUILD_ROOT/../../libs/apple/ios_x86_64/lib
    FF_INSTALL_INCLUDE=$FF_BUILD_ROOT/../../libs/apple/ios_x86_64/include

    X264_INCLUDE=$FF_BUILD_ROOT/../../ffmpeg-external-libs/libs-apple-ios-x86-64/libx264/include
    X264_LIBS=$FF_BUILD_ROOT/../../ffmpeg-external-libs/libs-apple-ios-x86-64/libx264/lib

elif [ "$FF_ARCH" = "armv7" ]; then
    FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --arch=$FF_ARCH"
    FF_BUILD_NAME="ffmpeg-armv7"
    FF_BUILD_NAME_OPENSSL=openssl-armv7
    FF_XCRUN_OSVERSION="-miphoneos-version-min=8.0"
    FF_XCODE_BITCODE="-fembed-bitcode"
    FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS $FFMPEG_CFG_FLAGS_ARM --enable-pic --disable-asm"
    FF_INSTALL_LIBS=$FF_BUILD_ROOT/../../libs/apple/ios_armv7/lib
    FF_INSTALL_INCLUDE=$FF_BUILD_ROOT/../../libs/apple/ios_armv7/include

    X264_INCLUDE=$FF_BUILD_ROOT/../../ffmpeg-external-libs/libs-apple-ios-arm32/libx264/include
    X264_LIBS=$FF_BUILD_ROOT/../../ffmpeg-external-libs/libs-apple-ios-arm32/libx264/lib

#    FFMPEG_CFG_CPU="--cpu=cortex-a8"
elif [ "$FF_ARCH" = "armv7s" ]; then
    FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --arch=$FF_ARCH"
    FF_BUILD_NAME="ffmpeg-armv7s"
    FF_BUILD_NAME_OPENSSL=openssl-armv7s
    FFMPEG_CFG_CPU="--cpu=swift"
    FF_XCRUN_OSVERSION="-miphoneos-version-min=8.0"
    FF_XCODE_BITCODE="-fembed-bitcode"
    FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS $FFMPEG_CFG_FLAGS_ARM --enable-pic --disable-asm"
    FF_INSTALL_LIBS=$FF_BUILD_ROOT/../../libs/apple/ios_armv7s/lib
    FF_INSTALL_INCLUDE=$FF_BUILD_ROOT/../../libs/apple/ios_armv7s/include

    X264_INCLUDE=$FF_BUILD_ROOT/../../ffmpeg-external-libs/libs-apple-ios-arm32/libx264/include
    X264_LIBS=$FF_BUILD_ROOT/../../ffmpeg-external-libs/libs-apple-ios-arm32/libx264/lib


elif [ "$FF_ARCH" = "arm64" ]; then
    FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --arch=$FF_ARCH"
    FF_BUILD_NAME="ffmpeg-arm64"
    FF_BUILD_NAME_OPENSSL=openssl-arm64
    FF_XCRUN_OSVERSION="-miphoneos-version-min=8.0"
    FF_XCODE_BITCODE="-fembed-bitcode"
    FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS $FFMPEG_CFG_FLAGS_ARM"
    FF_GASPP_EXPORT="GASPP_FIX_XCODE5=1"
    FF_INSTALL_LIBS=$FF_BUILD_ROOT/../../libs/apple/ios_arm64/lib
    FF_INSTALL_INCLUDE=$FF_BUILD_ROOT/../../libs/apple/ios_arm64/include

    X264_INCLUDE=$FF_BUILD_ROOT/../../ffmpeg-external-libs/libs-apple-ios-arm64/libx264/include
    X264_LIBS=$FF_BUILD_ROOT/../../ffmpeg-external-libs/libs-apple-ios-arm64/libx264/lib
    
elif [ "$FF_ARCH" = "mac_x64" ]; then
    FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --arch=x86_64"
    FF_BUILD_NAME="ffmpeg-mac_x86_64"
    FF_XCODE_BITCODE="-fembed-bitcode"
    FF_XCRUN_PLATFORM="MacOSX"
    FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS $FFMPEG_CFG_FLAGS_ARM"
    FF_GASPP_EXPORT="GASPP_FIX_XCODE5=1"
    FF_INSTALL_LIBS=$FF_BUILD_ROOT/../../libs/apple/mac_x64/lib
    FF_INSTALL_INCLUDE=$FF_BUILD_ROOT/../../libs/apple/mac_x64/include

    X264_INCLUDE=$FF_BUILD_ROOT/../../ffmpeg-external-libs/libs-apple-osx/libx264/include
    X264_LIBS=$FF_BUILD_ROOT/../../ffmpeg-external-libs/libs-apple-osx/libx264/lib

    FDK_AAC_INCLUDE=$FF_BUILD_ROOT/../../ffmpeg-external-libs/libfdk-aac/include
    FDK_AAC_LIBS=$FF_BUILD_ROOT/../../ffmpeg-external-libs/libfdk-aac/lib/apple/osx
else
    echo "unknown architecture $FF_ARCH";
    exit 1
fi

echo "build_name: $FF_BUILD_NAME"
echo "platform:   $FF_XCRUN_PLATFORM"
echo "osversion:  $FF_XCRUN_OSVERSION"

#--------------------
echo "===================="
echo "[*] make ios toolchain $FF_BUILD_NAME"
echo "===================="

FF_BUILD_SOURCE="$FF_BUILD_ROOT/../../FFmpeg-release-3.3"
FF_BUILD_PREFIX="$FF_BUILD_ROOT/build/$FF_BUILD_NAME/output"

FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --prefix=$FF_BUILD_PREFIX"

mkdir -p $FF_BUILD_PREFIX

echo "build_source: $FF_BUILD_SOURCE"
echo "build_prefix: $FF_BUILD_PREFIX"

#--------------------
echo "\n--------------------"
echo "[*] configurate ffmpeg"
echo "--------------------"
FF_XCRUN_SDK=`echo $FF_XCRUN_PLATFORM | tr '[:upper:]' '[:lower:]'`
FF_XCRUN_CC="xcrun -sdk $FF_XCRUN_SDK clang"

FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS $FFMPEG_CFG_CPU"


ARCH=${FF_ARCH}
if [ "$FF_ARCH" = "mac_x64" ]; then
        ARCH="x86_64"
fi

echo ARCH:"$ARCH"

FFMPEG_CFLAGS=
FFMPEG_CFLAGS="$FFMPEG_CFLAGS -arch $ARCH"
FFMPEG_CFLAGS="$FFMPEG_CFLAGS $FF_XCRUN_OSVERSION"
FFMPEG_CFLAGS="$FFMPEG_CFLAGS $FFMPEG_EXTRA_CFLAGS"
FFMPEG_CFLAGS="$FFMPEG_CFLAGS $FF_XCODE_BITCODE"
FFMPEG_LDFLAGS="$FFMPEG_CFLAGS"
FFMPEG_DEP_LIBS=

FFMPEG_CFLAGS="$FFMPEG_CFLAGS -I$X264_INCLUDE "
FFMPEG_LDFLAGS="$FFMPEG_LDFLAGS -L$X264_LIBS"

echo  "$FF_ARCH"
if [ "$FF_ARCH" = "mac_x64" ]; then
    FFMPEG_CFLAGS="$FFMPEG_CFLAGS -I $FDK_AAC_INCLUDE "
    FFMPEG_LDFLAGS="$FFMPEG_LDFLAGS -L $FDK_AAC_LIBS"
    
    FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --enable-libfdk-aac"
fi

echo FFMPEG_CFLAGS:${FFMPEG_CFLAGS}
echo FFMPEG_LDFLAGS:${FFMPEG_LDFLAGS}
echo FFMPEG_CFG_FLAGS:${FFMPEG_CFG_FLAGS}

# #--------------------
# echo "\n--------------------"
# echo "[*] check OpenSSL"
# echo "----------------------"
# FFMPEG_DEP_OPENSSL_INC=$FF_BUILD_ROOT/build/$FF_BUILD_NAME_OPENSSL/output/include
# FFMPEG_DEP_OPENSSL_LIB=$FF_BUILD_ROOT/build/$FF_BUILD_NAME_OPENSSL/output/lib
# #--------------------
# # with openssl
# if [ -f "${FFMPEG_DEP_OPENSSL_LIB}/libssl.a" ]; then
#     FFMPEG_CFG_FLAGS="$FFMPEG_CFG_FLAGS --enable-openssl"

#     FFMPEG_CFLAGS="$FFMPEG_CFLAGS -I${FFMPEG_DEP_OPENSSL_INC}"
#     FFMPEG_DEP_LIBS="$FFMPEG_CFLAGS -L${FFMPEG_DEP_OPENSSL_LIB} -lssl -lcrypto"
# fi

#--------------------
echo "\n--------------------"
echo "[*] configure"
echo "----------------------"

if [ ! -d $FF_BUILD_SOURCE ]; then
    echo ""
    echo "!! ERROR"
    echo "!! Can not find FFmpeg directory for $FF_BUILD_NAME"
    echo "!! Run 'sh init-ios.sh' first"
    echo ""
    exit 1
fi

# xcode configuration
export DEBUG_INFORMATION_FORMAT=dwarf-with-dsym

cd $FF_BUILD_SOURCE
#if [ -f "./config.h" ]; then
#    echo 'reuse configure'
#else
    echo "config: $FFMPEG_CFG_FLAGS $FF_XCRUN_CC"
    ./configure \
        $FFMPEG_CFG_FLAGS \
        --cc="$FF_XCRUN_CC" \
        $FFMPEG_CFG_CPU \
        --extra-cflags="$FFMPEG_CFLAGS" \
        --extra-cxxflags="$FFMPEG_CFLAGS" \
        --extra-ldflags="$FFMPEG_LDFLAGS $FFMPEG_DEP_LIBS" \
        --libdir="$FF_INSTALL_LIBS" \
        --incdir="$FF_INSTALL_INCLUDE"
    make clean
#fi

#--------------------
echo "\n--------------------"
echo "[*] compile ffmpeg"
echo "--------------------"
cp config.* $FF_BUILD_PREFIX
make -j8 $FF_GASPP_EXPORT
make install
mkdir -p $FF_BUILD_PREFIX/include/libffmpeg
cp -f config.h $FF_BUILD_PREFIX/include/libffmpeg/config.h