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

print_info_in_red(){
    echo -e "\033[31m ${1} \033[0m :"
}

XCODEPROJECTPATH='./YouMeVoiceEngine'

#编译iOS

CleanType="no"
FfmpegSupport="no"
BeautySupport="no"
GCCProcessorDefinitions="\$GCC_PREPROCESSOR_DEFINITIONS YOUME_VIDEO=1"
GCCProcessorDefinitionsiPhone="\$GCC_PREPROCESSOR_DEFINITIONS YOUME_VIDEO=1"
GCCProcessorDefinitionsMac="\$GCC_PREPROCESSOR_DEFINITIONS YOUME_VIDEO=1"
BuildIos="no"
BuildMacOS="no"
BuildMacOSDyn="no"
for var in $*
do
    if [[ $var = "clean" ]]; then
        CleanType="yes"
    elif [[ $var = "ffmpeg" ]]; then
    	 FfmpegSupport="yes"
    elif [[ $var = "tmediadump" ]]; then
         GCCProcessorDefinitions="$GCCProcessorDefinitions TMEDIA_DUMP_ENABLE=1 "
         GCCProcessorDefinitionsiPhone="$GCCProcessorDefinitionsiPhone TMEDIA_DUMP_ENABLE=1"
    elif [[ $var = "rscode" ]]; then
         GCCProcessorDefinitions="$GCCProcessorDefinitions TDAV_RS_CODE_ENABLE=1 "
         GCCProcessorDefinitionsiPhone="$GCCProcessorDefinitionsiPhone TDAV_RS_CODE_ENABLE=1"
    elif [[ $var = "beauty" ]]; then
        BeautySupport="yes"
    elif [[ $var = "ios" ]];then
        BuildIos="yes"
    elif [[ $var = "macos" ]];then
        BuildMacOS="yes"
    elif [[ $var = "macos_dyn" ]];then
        BuildMacOSDyn="yes"
    fi
done


VOICE_LIB_NAME="libyoume_voice_engine.a"
FFMPEG_LIB_NAME="libffmpeg3.3.a"

# build FFmpeg for all arch
if [[ $FfmpegSupport = "yes" ]]; then
    FFMPEG_PLAYER_PATH=$(pwd)/../ffmpegPlayer
    FFMPEG_LIB_PATH_MAC=${FFMPEG_PLAYER_PATH}/libs/apple/osx
    FFMPEG_LIB_PATH_IPHONEOS=${FFMPEG_PLAYER_PATH}/libs/apple/iphoneos
    FFMPEG_LIB_PATH_UNIVERSAL=${FFMPEG_PLAYER_PATH}/libs/apple/universal
    THIRD_LIB_PATH=$(pwd)/../youme_common/thirdlib

    # Rebuild FFMPEG libs only when cleaning is required.
    # The FFMPEG lib is rarely changed, so usually we don't need to build it.
    # But for a "clean" build, to make sure it's really clean, we also rebuild the FFMPEG libs.
    if [[ $CleanType = "yes" ]]; then
        echo "=== 开始编译x264 ==="
        pushd ${FFMPEG_PLAYER_PATH}/ffmpeg-external-libs/libx264
        sh build_all_apple.sh
        check_result $? "Building x264"
        popd

        echo "=== 开始编译FFmpeg ==="
        pushd ${FFMPEG_PLAYER_PATH}/build/ffmpeg_ios
        sh compile-ffmpeg.sh all
        check_result $? "Building FFMPEG"
        popd
    fi
fi

if [[ $BuildIos = "yes" ]]; then
    VOICE_LIB_PATH_SIMULATOR="${XCODEPROJECTPATH}/build/Release-iphonesimulator"
    VOICE_LIB_PATH_IPHONEOS="${XCODEPROJECTPATH}/build/Release-iphoneos"
    VOICE_LIB_PATH_UNIVERSAL="${XCODEPROJECTPATH}/build/Release-universal"

    if [ ! -d $VOICE_LIB_PATH_UNIVERSAL ]; then
        mkdir -p $VOICE_LIB_PATH_UNIVERSAL
    fi

    # Get the SDK version for iPhone available on this PC
    SDK_VER_IOS=$(xcodebuild -showsdks | grep -m 1 iphone | awk '{print $2}')
    echo "SDK ios  ver: ${SDK_VER_IOS}"


    if [[ $CleanType = "yes" ]]; then
        rm -rf ${VOICE_LIB_PATH_SIMULATOR}/*
        rm -rf ${VOICE_LIB_PATH_IPHONEOS}/*
        rm -rf ${VOICE_LIB_PATH_UNIVERSAL}/*
        xcodebuild clean -project $XCODEPROJECTPATH/youme_voice_engine.xcodeproj -sdk iphonesimulator${SDK_VER_IOS} -UseModernBuildSystem=NO
        check_result $? "iOS虚拟机静态库 清除"
        xcodebuild clean -project $XCODEPROJECTPATH/youme_voice_engine.xcodeproj -sdk iphoneos${SDK_VER_IOS} -UseModernBuildSystem=NO
        check_result $? "iOS真机静态库 清除"
    fi

    # 真机宏定义设置
    if [[ $BeautySupport = "yes" ]]; then
            GCCProcessorDefinitionsiPhone="$GCCProcessorDefinitionsiPhone OPEN_BEAUTIFY=1 "
    fi
    echo "开始编译iOS虚拟机静态库..."
    xcodebuild -project $XCODEPROJECTPATH/youme_voice_engine.xcodeproj -target youme_voice_engine -configuration Release -sdk iphonesimulator${SDK_VER_IOS} -arch i386 -arch x86_64 GCC_PREPROCESSOR_DEFINITIONS="$GCCProcessorDefinitions" -UseModernBuildSystem=NO
    check_result $? "iOS虚拟机静态库 编译"

    echo "开始编译iOS真机静态库..."
    xcodebuild -project $XCODEPROJECTPATH/youme_voice_engine.xcodeproj -target youme_voice_engine -configuration Release -sdk iphoneos${SDK_VER_IOS} -arch armv7 -arch arm64 GCC_PREPROCESSOR_DEFINITIONS="$GCCProcessorDefinitionsiPhone WEBRTC_HAS_NEON=1" -UseModernBuildSystem=NO
    check_result $? "iOS真机静态库 编译"
    #xcodebuild -project $XCODEPROJECTPATH/youme_voice_engine.xcodeproj -target youme_voice_engine_resource -configuration Release -sdk iphoneos9.3 -arch armv7 -arch arm64 -UseModernBuildSystem=NO
    #check_result $? "iOS真机静态库资源 编译"

    lipo -output ${VOICE_LIB_PATH_UNIVERSAL}/${VOICE_LIB_NAME}  -create ${VOICE_LIB_PATH_SIMULATOR}/${VOICE_LIB_NAME} ${VOICE_LIB_PATH_IPHONEOS}/${VOICE_LIB_NAME}
    check_result $? "Merge iphoneSimulator/iphoneOS libs into universal one"

    # Combine FFMPEG libs into the final library
    if [[ $FfmpegSupport = "yes" ]]; then
        VOICE_LIB_NAME_MERGED="libyoume_voice_engine_merged.a"

        pushd ${VOICE_LIB_PATH_IPHONEOS}
        check_result $? "cd ${VOICE_LIB_PATH_IPHONEOS}"
        libtool -static -no_warning_for_no_symbols -o ${FFMPEG_LIB_NAME} ${FFMPEG_LIB_PATH_UNIVERSAL}/libavutil.a ${FFMPEG_LIB_PATH_UNIVERSAL}/libavformat.a ${FFMPEG_LIB_PATH_UNIVERSAL}/libavcodec.a 
        check_result $? "create ffmpeg iphoneos libs"
        if [[ $BeautySupport = "yes" ]]; then
        libtool -static -no_warning_for_no_symbols -o ${VOICE_LIB_NAME_MERGED} ${FFMPEG_LIB_PATH_UNIVERSAL}/libx264.a  ${VOICE_LIB_NAME}
        else
        libtool -static -no_warning_for_no_symbols -o ${VOICE_LIB_NAME_MERGED} ${FFMPEG_LIB_PATH_UNIVERSAL}/libx264.a ${VOICE_LIB_NAME}
        fi
        check_result $? "Merge x264 iphoneos libs"
        popd

        pushd ${VOICE_LIB_PATH_UNIVERSAL}
        check_result $? "cd ${VOICE_LIB_PATH_UNIVERSAL}"
        libtool -static -no_warning_for_no_symbols -o ${FFMPEG_LIB_NAME} ${FFMPEG_LIB_PATH_UNIVERSAL}/libavutil.a ${FFMPEG_LIB_PATH_UNIVERSAL}/libavformat.a ${FFMPEG_LIB_PATH_UNIVERSAL}/libavcodec.a 
        check_result $? "create ffmpeg iphoneos libs"
        if [[ $BeautySupport = "yes" ]]; then
        libtool -static -no_warning_for_no_symbols -o ${VOICE_LIB_NAME_MERGED}  ${FFMPEG_LIB_PATH_UNIVERSAL}/libx264.a  ${VOICE_LIB_NAME}
        else
        libtool -static -no_warning_for_no_symbols -o ${VOICE_LIB_NAME_MERGED}  ${FFMPEG_LIB_PATH_UNIVERSAL}/libx264.a ${VOICE_LIB_NAME}
        fi
        check_result $? "Merge ffmpeg universal libs"
        popd

        VOICE_LIB_NAME=${VOICE_LIB_NAME_MERGED}
    fi

    echo "复制iOS静态库文件..."

    OUTPUT_LIB_PATH_IPHONEOS="../../../Output/youme_voice_engine/lib/ios/Release-iphoneos"
    OUTPUT_LIB_PATH_UNIVERSAL="../../../Output/youme_voice_engine/lib/ios/Release-universal"

    if [ ! -d $OUTPUT_LIB_PATH_IPHONEOS ]; then
        mkdir -p $OUTPUT_LIB_PATH_IPHONEOS
    fi
    if [ ! -d $OUTPUT_LIB_PATH_UNIVERSAL ]; then
        mkdir -p $OUTPUT_LIB_PATH_UNIVERSAL
    fi
    cp ${VOICE_LIB_PATH_IPHONEOS}/${VOICE_LIB_NAME}  ${OUTPUT_LIB_PATH_IPHONEOS}/libyoume_voice_engine.a
    cp ${VOICE_LIB_PATH_IPHONEOS}/${FFMPEG_LIB_NAME}  ${OUTPUT_LIB_PATH_IPHONEOS}/${FFMPEG_LIB_NAME}
    cp ../youme_common/lib/ios/libYouMeCommon.a      ${OUTPUT_LIB_PATH_IPHONEOS}/libYouMeCommon.a
    check_result $? "复制iOS真机静态库"
    cp ${VOICE_LIB_PATH_UNIVERSAL}/${VOICE_LIB_NAME}  ${OUTPUT_LIB_PATH_UNIVERSAL}/libyoume_voice_engine.a
    cp ${VOICE_LIB_PATH_UNIVERSAL}/${FFMPEG_LIB_NAME}  ${OUTPUT_LIB_PATH_UNIVERSAL}/${FFMPEG_LIB_NAME}
    cp ../youme_common/lib/ios/libYouMeCommon.a       ${OUTPUT_LIB_PATH_UNIVERSAL}/libYouMeCommon.a
    check_result $? "复制iOS全架构（真机+虚拟机）静态库"
    #cp -R $XCODEPROJECTPATH/build/Release-iphoneos/youme_voice_engine_resource.bundle $IOS_LIB_PATH/youme_voice_engine_resource.bundle
    #check_result $? "复制iOS真机静态库资源"
fi

#mac
if [[ $BuildMacOS = "yes" ]]; then
    VOICE_LIB_NAME="libyoume_voice_engine_mac.a"
    VOICE_LIB_PATH_MAC="${XCODEPROJECTPATH}/build/Release"
    if [ ! -d $VOICE_LIB_PATH_MAC ]; then
        mkdir -p $VOICE_LIB_PATH_MAC
    fi

    SDK_VER_MAC=$(xcodebuild -showsdks | grep -v DriverKit | grep -m 1 macosx | awk '{print $2}')
    echo "SDK mac ver: ${SDK_VER_MAC}"

    if [[ $CleanType = "yes" ]]; then
        rm -rf ${VOICE_LIB_PATH_MAC}/*
        #todopinky:这里怎么设置target呢？需要设置吗？
        xcodebuild clean -project $XCODEPROJECTPATH/youme_voice_engine.xcodeproj -target youme_voice_engine_mac -sdk macosx${SDK_VER_MAC} -UseModernBuildSystem=NO
        check_result $? "Mac静态库 清除"
    fi

    echo "开始编译Mac静态库..."
    xcodebuild -project $XCODEPROJECTPATH/youme_voice_engine.xcodeproj -target youme_voice_engine_mac -configuration Release -sdk macosx${SDK_VER_MAC} GCC_PREPROCESSOR_DEFINITIONS="$GCCProcessorDefinitionsMac" -UseModernBuildSystem=NO
    check_result $? "Mac静态库 编译"

    # Combine FFMPEG libs into the final library
    if [[ $FfmpegSupport = "yes" ]]; then
        VOICE_LIB_NAME_MERGED="libyoume_voice_engine_merged.a"

        pushd ${VOICE_LIB_PATH_MAC}
        check_result $? "cd ${VOICE_LIB_PATH_MAC}"
        libtool -static -no_warning_for_no_symbols -o ${FFMPEG_LIB_NAME} ${FFMPEG_LIB_PATH_MAC}/libavutil.a ${FFMPEG_LIB_PATH_MAC}/libavformat.a ${FFMPEG_LIB_PATH_MAC}/libavcodec.a  ${FFMPEG_LIB_PATH_MAC}/libswresample.a
        check_result $? "create ffmpeg mac libs"
        libtool -static -no_warning_for_no_symbols -o ${VOICE_LIB_NAME_MERGED} ${FFMPEG_LIB_PATH_MAC}/libx264.a  ${FFMPEG_LIB_PATH_MAC}/libfdk-aac.a   ${VOICE_LIB_NAME}
        check_result $? "Merge x264  fdk-aac  mac libs"
        popd

        VOICE_LIB_NAME=${VOICE_LIB_NAME_MERGED}
    fi

    echo "复制mac静态库文件..."

    OUTPUT_LIB_PATH_MAC="../../../Output/youme_voice_engine/lib/macos/Release-macos"

    if [ ! -d $OUTPUT_LIB_PATH_MAC ]; then
        mkdir -p $OUTPUT_LIB_PATH_MAC
    fi

    cp ${VOICE_LIB_PATH_MAC}/${VOICE_LIB_NAME}  ${OUTPUT_LIB_PATH_MAC}/libyoume_voice_engine.a
    cp ${VOICE_LIB_PATH_MAC}/${FFMPEG_LIB_NAME}  ${OUTPUT_LIB_PATH_MAC}/${FFMPEG_LIB_NAME}
    cp ../youme_common/lib/macos/libYouMeCommon.a      ${OUTPUT_LIB_PATH_MAC}/libYouMeCommon.a
    check_result $? "复制mac静态库"
fi

if [[ $BuildMacOSDyn = "yes" ]]; then
    VOICE_LIB_NAME="youme_voice_engine_macd.dylib"
    VOICE_LIB_PATH_MAC="${XCODEPROJECTPATH}/build/Release"
    if [ ! -d $VOICE_LIB_PATH_MAC ]; then
        mkdir -p $VOICE_LIB_PATH_MAC
    fi

    SDK_VER_MAC=$(xcodebuild -showsdks |grep -v DriverKit | grep -m 1 macosx | awk '{print $2}')
    echo "SDK mac ver: ${SDK_VER_MAC}"

    if [[ $CleanType = "yes" ]]; then
        rm -rf ${VOICE_LIB_PATH_MAC}/*
        #todopinky:这里怎么设置target呢？需要设置吗？
        xcodebuild clean -project $XCODEPROJECTPATH/youme_voice_engine.xcodeproj -target youme_voice_engine_macd -sdk macosx${SDK_VER_MAC} -UseModernBuildSystem=NO
        check_result $? "Mac动态库 清除"
    fi

    # Combine FFMPEG libs into the final library
    if [[ $FfmpegSupport = "yes" ]]; then
        #这里，要先编译ffmpeg，再编译动态库
        echo "开始编译Mac动态库..."
        xcodebuild -project $XCODEPROJECTPATH/youme_voice_engine.xcodeproj -target youme_voice_engine_macd -configuration Release -sdk macosx${SDK_VER_MAC} GCC_PREPROCESSOR_DEFINITIONS="$GCCProcessorDefinitionsMac" -UseModernBuildSystem=NO
        check_result $? "Mac动态库 编译"
    fi

    echo "复制mac动态库文件..."

    OUTPUT_LIB_PATH_MAC="../../../Output/youme_voice_engine/lib/macos/Release-macos"

    if [ ! -d $OUTPUT_LIB_PATH_MAC ]; then
        mkdir -p $OUTPUT_LIB_PATH_MAC
    fi

    cp ${VOICE_LIB_PATH_MAC}/${VOICE_LIB_NAME}  ${OUTPUT_LIB_PATH_MAC}/youme_voice_engine_mac.dylib
    check_result $? "复制mac动态库"
fi
