#!/bin/bash

################################################################################
# Check the execution result of the previous command, and exit with 1 if there's
# any error
################################################################################
check_result(){
if [[ ${1} = 0 ]]; then
echo -e "\033[31m == ${2}: 成功 ^-^ \033[0m"
else
echo -e "\033[31m ====================================================== \033[0m"
echo -e "\033[31m == ${2}: 失败!!! \033[0m"
echo -e "\033[31m ====================================================== \033[0m"
exit 1
fi
}

#处理参数
CleanType="no"
FfmpegSupport="no"
TMediadump="0"
RSCode="0"

for var in $*
do
if [[ $var = "clean" ]]; then
CleanType="yes"
elif [[ $var = "ffmpeg" ]]; then
FfmpegSupport="yes"
elif [[ $var = "tmediadump" ]]; then
TMediadump="1"
elif [[ $var = "rscode" ]]; then
RSCode="1"
elif [[ $var = "jar" ]];then
OnlyJar="jar"
fi
done

#准备目录
OUTPUT_PATH="../../../../Output"
SDK_PATH="$OUTPUT_PATH/youme_voice_engine"
LIB_PATH="$SDK_PATH/lib"
SYMBOL_PATH="$SDK_PATH/symbol"
if [ ! -d $SDK_PATH ]; then
mkdir -p $SDK_PATH
fi


ARMV5_PATH="$LIB_PATH/android/armeabi"
ARMV7_PATH="$LIB_PATH/android/armeabi-v7a"
ARM64_PATH="$LIB_PATH/android/arm64-v8a"
X86_PATH="$LIB_PATH/android/x86"
X86_64_PATH="$LIB_PATH/android/x86_64"
if [ ! -d $ARMV5_PATH ]; then
mkdir -p $ARMV5_PATH
fi

if [ ! -d $ARMV7_PATH ]; then
mkdir -p $ARMV7_PATH
fi

if [ ! -d $arm64_PATH ]; then
mkdir -p $arm64_PATH
fi

if [ ! -d $X86_PATH ]; then
mkdir -p $X86_PATH
fi

if [ ! -d $X86_64_PATH ]; then
mkdir -p $X86_64_PATH
fi

ARMV5_SYMBOL_PATH="$SYMBOL_PATH/android/armeabi"
ARMV7_SYMBOL_PATH="$SYMBOL_PATH/android/armeabi-v7a"
ARM64_SYMBOL_PATH="$SYMBOL_PATH/android/arm64-v8a"
X86_SYMBOL_PATH="$SYMBOL_PATH/android/x86"
X86_64_SYMBOL_PATH="$SYMBOL_PATH/android/x86_64"


if [ ! -d $ARMV5_SYMBOL_PATH ]; then
mkdir -p $ARMV5_SYMBOL_PATH
fi

if [ ! -d $ARMV7_SYMBOL_PATH ]; then
mkdir -p $ARMV7_SYMBOL_PATH
fi

if [ ! -d $ARM64_SYMBOL_PATH ]; then
mkdir -p $ARM64_SYMBOL_PATH
fi

if [ ! -d $X86_SYMBOL_PATH ]; then
mkdir -p $X86_SYMBOL_PATH
fi

if [ ! -d $X86_64_SYMBOL_PATH ]; then
mkdir -p $X86_64_SYMBOL_PATH
fi

if [[ "$CleanType" = "yes" ]]; then
ndk-build clean
check_result $? "Clean Android 动态库"
fi

echo $FfmpegSupport
if [[ "$FfmpegSupport" = "yes" ]]; then
FFMPEG_SUPPORT="1"
    # Rebuild FFMPEG libs only when cleaning is required.
    # The FFMPEG lib is rarely changed, so usually we don't need to build it.
    # But for a "clean" build, to make sure it's really clean, we also rebuild the FFMPEG libs.
    if [[ "$CleanType" = "yes" ]]; then
        echo "=== 开始编译x264 ==="
        pushd ../ffmpegPlayer/ffmpeg-external-libs/libx264
        sh build_all_ndk.sh
        check_result $? "Building x264"
        popd

        echo "=== 开始编译FFmpeg ==="
        pushd ../ffmpegPlayer/build/ffmpeg_android
        sh compile-ffmpeg.sh all
        check_result $? "Building FFMPEG"
        popd
    fi
else
FFMPEG_SUPPORT="0"
fi

function buildSO(){
    ndk-build -j8 TARGET_PLATFORM=android-14 APP_ABI="armeabi armeabi-v7a x86 " NEON=1 FFMPEG=${FFMPEG_SUPPORT} TMediaDump=${TMediadump} RSCode=${RSCode}
    check_result $? "编译 Android 非64位动态库"

    cp ../libs/armeabi/libyoume_voice_engine.so $ARMV5_PATH/
    check_result $? "复制 Android armeabi so"
    cp ../libs/armeabi-v7a/libyoume_voice_engine.so $ARMV7_PATH/
    check_result $? "复制 Android armeabi-v7a so"
    cp ../libs/x86/libyoume_voice_engine.so $X86_PATH/
    check_result $? "复制 Android x86 so"

    cp ../obj/local/armeabi/libyoume_voice_engine.so $ARMV5_SYMBOL_PATH/libyoume_voice_engine.so
    check_result $? "复制 Android armeabi symbol so"
    cp ../obj/local/armeabi-v7a/libyoume_voice_engine.so $ARMV7_SYMBOL_PATH/libyoume_voice_engine.so
    check_result $? "复制 Android armeabi-v7a symbol so"
    cp ../obj/local/x86/libyoume_voice_engine.so $X86_SYMBOL_PATH/libyoume_voice_engine.so
    check_result $? "复制 Android x86 symbol so"
}

#arm64 编译
function buildSO64(){
    #check and fix ndk r10e libc.so bug
    ndk_version=`cat "$NDK_ROOT/RELEASE.TXT"`
    if [ "$ndk_version" = "r10e (64-bit)" ]; then
        cp ./libc.so "$NDK_ROOT/platforms/android-21/arch-arm64/usr/lib/libc.so"
        check_result $? "fix android-21 arm64 libc.so"
    fi

    #build arm64-va8
    ndk-build -j8 TARGET_PLATFORM=android-21 APP_ABI="arm64-v8a" NEON=1 FFMPEG=${FFMPEG_SUPPORT} TMediaDump=${TMediadump} RSCode=${RSCode}
    check_result $? "编译 Android 64位动态库"

    cp ../libs/arm64-v8a/libyoume_voice_engine.so $ARM64_PATH/
    check_result $? "复制 Android arm64-v8a so"

    cp ../obj/local/arm64-v8a/libyoume_voice_engine.so $ARM64_SYMBOL_PATH/
    check_result $? "复制 Android arm64-v8a symbol so"   
}

function buildJar(){
    pushd ../java

    # 编译
    #ant debug
    sh build.sh
    check_result $? "编译 Android jar包"
    popd

    if [ -f ../java/build/libs/youme_voice_engine.jar ]; then
        cp ../java/build/libs/youme_voice_engine.jar $LIB_PATH/android/youme_voice_engine.jar
        cp ../java/libs/android-support-v4.jar $LIB_PATH/android/android-support-v4.jar
        check_result $? "复制 Android jar包"
    fi
}

buildSO
buildSO64
buildJar



