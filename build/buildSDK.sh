#!/bin/bash

################################################################################
# Print a line of information in color red
################################################################################
print_info_in_red(){
    echo -e "\033[31m ${1} \033[0m"
}

################################################################################
# Print a warning
################################################################################
print_and_exit(){
    print_info_in_red "${1}!!!"
    exit 1
}

################################################################################
# Check the execution result of the previous command, and exit with 1 if there's
# any error
################################################################################
show_result(){
	if [[ ${1} = 0 ]]; then
        print_info_in_red "== ${2}: 成功 ^-^"
    else
        print_info_in_red "======================================================"
        print_info_in_red "== ${2}: 失败!!!"
        print_info_in_red "======================================================"
        exit 1
    fi
}

################################################################################
# Print help information
################################################################################
print_help_and_exit(){
    print_info_in_red "${1}"
    echo "Usage: ${0} <[ios] [android] [macos]> [ffmpeg] [clean] [sdk-pkg]"
    echo "ios, android, macos - 编译iOS/Android/MacOS平台版本, 至少要指定一个平台，也可以三个同时指定"
    echo "ffmpeg       - 支持 FFMPEG "
    echo "clean        - 编译前先清空"
    echo "sdk-pkg      - 编译后打包SDK，指定这个选项时，必须同时指定ios、macos和android平台"
    exit 1
}

################################################################################
# Print options
################################################################################
print_options_and_confirm(){
    echo "======================================================"
    echo "== 编译选项列表 =="
    echo -n "* 编译前先Clean"
    if [[ $BuildClean != "no" ]]; then
        print_info_in_red " -- yes"
    else
        print_info_in_red " -- no"
    fi

    echo -n "* 编译iOS版本"
    if [[ $BuildIos != "no" ]]; then
        print_info_in_red " -- yes"
    else
        print_info_in_red " -- no"
    fi

    echo -n "* 编译Android版本"
    if [[ $BuildAndroid != "no" ]]; then
        print_info_in_red " -- yes"
    else
        print_info_in_red " -- no"
    fi

    echo -n "* 编译MacOS版本"
    if [[ $BuildMacOS != "no" ]]; then
        print_info_in_red " -- yes"
    else
        print_info_in_red " -- no"
    fi

    echo -n "* 支持FFMPEG"
    if [[ $BuildFfmpeg != "no" ]]; then
        print_info_in_red " -- yes"
    else
        print_info_in_red " -- no"
    fi

    echo -n "* SDK打包"
    if [[ $BuildSdkPkg != "no" ]]; then
        print_info_in_red " -- yes"
    else
        print_info_in_red " -- no"
    fi
    
    echo "Start building..."    
}

################################################################################
# Check the sanity and dependency of the parameters
################################################################################
check_params(){
    if [[ $BuildSdkPkg != "no" ]]; then
        if [[ $BuildIos = "no" || $BuildAndroid = "no" || $BuildMacOS = "no" ]]; then
            print_and_exit "打包SDK，必须指定所有平台（ios 和 android）"
        fi
    fi

    if [[ $BuildIos = "no" && $BuildAndroid = "no" && $BuildMacOS = "no" ]]; then
        print_and_exit "至少指定一个目标平台（ios/android/macos）"
    fi
    
    if [[ $BuildAndroid != "no" && $BuildClean = "no" ]]; then
        if [ $BuildFfmpeg = "no" ]; then
            print_and_exit "编译Android平台需要指定 ffmpeg 参数"
        fi
    fi
}

################################################################################
# Start the execution flow
################################################################################

# If no parameter, print help info and exit
if [[ $# = 0 ]]; then
    print_help_and_exit
fi

# build parameter
BuildClean="no"
BuildFfmpeg="no"
BuildSdkPkg="no"
BuildIos="no"
BuildAndroid="no"
BuildMacOS="no"
UpdateSvn="no"

ENGINE_NAME="youme_voice_engine"

# Parse parameters
for var in $*
do
    if [ $var = "clean" ]; then
        BuildClean=$var
    elif [ $var = "ios" ]; then
        BuildIos=$var
    elif [ $var = "android" ]; then
        BuildAndroid=$var
    elif [ $var = "macos" ]; then
        BuildMacOS=$var
    elif [ $var = "ffmpeg" ]; then
        BuildFfmpeg=$var
    elif [ $var = "sdk-pkg" ]; then
        BuildSdkPkg=$var
    elif [ $var != "no" ]; then
    	print_help_and_exit "Invalid parameter:${var}"
    fi
done
check_params
print_options_and_confirm

################################################################################
# Start building
################################################################################

#修改build version info
VERSION_FILE="../$ENGINE_NAME/version.h"
mainVer=$(grep -E '#define MAIN_VER[[:blank:]]+[[:digit:]]+' ${VERSION_FILE} | awk '{print $3}')
minorVer=$(grep -E '#define MINOR_VER[[:blank:]]+[[:digit:]]+' ${VERSION_FILE} | awk '{print $3}')
releaseNo=$(grep -E '#define RELEASE_NUMBER[[:blank:]]+[[:digit:]]+' ${VERSION_FILE} | awk '{print $3}')
buildNo=$(grep -E '#define BUILD_NUMBER[[:blank:]]+[[:digit:]]+' ${VERSION_FILE} | awk '{print $3}')
if [[ x$mainVer = x || x$minorVer = x || x$releaseNo = x || x$buildNo = x ]]; then
	print_and_exit "从 version.h 获取版本号失败"
fi
if [[ $UpdateSvn != "no" ]]; then
    nextBuildNo=$(($buildNo+1))
    sed -E -i '' "s/#define BUILD_NUMBER[[:blank:]]+${buildNo}/#define BUILD_NUMBER ${nextBuildNo}/" ${VERSION_FILE}
else
    nextBuildNo=$buildNo
fi
nextBuildVersion="${mainVer}.${minorVer}.${releaseNo}.${nextBuildNo}"
echo "老版本号:${mainVer}.${minorVer}.${releaseNo}.${buildNo}"
echo "新版本号:${nextBuildVersion}"

#修改 version.h 中 FFMPEG_SUPPORT 的配置值
if [[ $BuildFfmpeg != "no" ]]; then
    ffmpegSupport="1"
else
    ffmpegSupport="0"
fi
curFFMpegSupport=$(grep -E '#define FFMPEG_SUPPORT[[:blank:]]+[[:digit:]]' ${VERSION_FILE} | awk '{print $3}')
if [ "${curFFMpegSupport}"x != "${ffmpegSupport}"x ]; then
    sed -E -i '' "s/#define FFMPEG_SUPPORT[[:blank:]]+${curFFMpegSupport}/#define FFMPEG_SUPPORT ${ffmpegSupport}/" ${VERSION_FILE}
fi
# verify if the FFMPEG_SUPPORT was correctly set
curFFMpegSupport=$(grep -E '#define FFMPEG_SUPPORT[[:blank:]]+[[:digit:]]' ${VERSION_FILE} | awk '{print $3}')
if [ "${curFFMpegSupport}"x != "${ffmpegSupport}"x ]; then
    print_and_exit "修改version.h中的 FFMPEG_SUPPORT 为${ffmpegSupport} 失败"
fi

################################################################################
# 开始编译 SDK
################################################################################
if [[ $BuildIos != "no" ]]; then
	print_info_in_red "编译iOS平台静态库..."
    pushd ../$ENGINE_NAME/xcode/
    ./build.sh ios $BuildClean $BuildFfmpeg $BuildTMediaDump $BuildRSCode $BuildWithBeauty
    show_result $?  "编译iOS平台静态库"
    popd

    #拷贝生成库到测试demo工程中
    rm -rf ../SDKTest/iosSample/SDK/include/*
    rm -rf ../SDKTest/iosSample/SDK/libs/*
    mkdir -p ../SDKTest/iosSample/SDK/include/
    cp ../../Output/$ENGINE_NAME/include/*  ../SDKTest/iosSample/SDK/include/
    show_result $? "复制SDK头文件到SDKTest iOS工程"
    mkdir -p ../SDKTest/iosSample/SDK/libs/
    cp ../../Output/$ENGINE_NAME/lib/ios/Release-universal/*.a  ../SDKTest/iosSample/SDK/libs/
    show_result $? "复制SDK库文件到SDKTest iOS工程"
fi #BuildIos

if [[ $BuildAndroid != "no" ]]; then
	print_info_in_red "编译Android平台动态库..."
    pushd ../$ENGINE_NAME/platforms/Android/jni/
    echo "./build.sh $BuildClean $BuildFfmpeg"
    ./build.sh $BuildClean $BuildFfmpeg
    show_result $?  "编译Android平台动态库"
    popd
fi #BuildAndroid

if [[ $BuildMacOS != "no" ]]; then
	print_info_in_red "编译MacOS平台静态库、动态库..."
    pushd ../$ENGINE_NAME/xcode/
    ./build.sh macos macos_dyn $BuildClean $BuildFfmpeg $BuildTMediaDump $BuildRSCode
    show_result $?  "编译MacOS平台静态库、动态库"
    popd

    TestProjectDir="../$ENGINE_NAME/xcode/YmVideoMacRef/"
    TestProjectIncludeDir=${TestProjectDir}"SDK/include/"
    TestProjectLibDir=${TestProjectDir}"SDK/libs/"
    rm -rf ${TestProjectIncludeDir}"*"
    rm -rf ${TestProjectLibDir}"*"
    echo  "include :${TestProjectIncludeDir}"
    echo "lib: ${TestProjectLibDir}"
    mkdir -p ${TestProjectIncludeDir}
    cp ../../Output/$ENGINE_NAME/include/*  ${TestProjectIncludeDir}
    show_result $? "复制SDK头文件到YmVoiceMacRef 工程"
    mkdir -p ${TestProjectLibDir}
    cp ../../Output/$ENGINE_NAME/lib/macos/Release-macos/*.a  ${TestProjectLibDir}
    show_result $? "复制SDK库文件到YmVoiceMacRef工程"
fi #BuildMacOS


#清空Output目录
if [[ $BuildClean != "no" ]]; then
    if [ -d ../../Output ]; then
        rm -rf ../../Output/$ENGINE_NAME
    else
        if [[ $BuildAndroid != "no" ]]; then
            #拷贝生成库到测试demo工程中
            rm -rf ../SDKTest/androidSampleNew/app/libs/youme_voice_engine.jar
            rm -rf ../SDKTest/androidSampleNew/app/libs/armeabi
            rm -rf ../SDKTest/androidSampleNew/app/libs/armeabi-v7a
            rm -rf ../SDKTest/androidSampleNew/app/libs/x86
            rm -rf ../SDKTest/androidSampleNew/app/libs/arm64-v8a
            mkdir -p ../SDKTest/androidSampleNew/app/libs
            cp -r ../../Output/$ENGINE_NAME/lib/android/*  ../SDKTest/androidSampleNew/app/libs/
            show_result $? "复制SDK库文件到SDKTest Android Sample new工程"
            rm -rf ../SDKTest/androidSampleNew/app/libs/android-support-v4.jar
        fi
    fi
fi

#复制SDK头文件
#SDK_INCLUDE_PATH="../../Output/${SDK_OUTPUT_DIR}/include"
#if [ ! -d ${SDK_INCLUDE_PATH} ]; then
#    mkdir -p ${SDK_INCLUDE_PATH}
#    check_error $? "创建SDK头文件Output目录"
#fi

#EXPORT_HEADER_PATH="../youme_voice_engine/bindings/cocos2d-x/interface"
#cp ${EXPORT_HEADER_PATH}/YouMeConstDefine.h  ${SDK_INCLUDE_PATH}
#check_error $? "复制SDK头文件YouMeConstDefine.h到Output目录"
#cp ${EXPORT_HEADER_PATH}/IYouMeVoiceEngine.h  ${SDK_INCLUDE_PATH}
#check_error $? "复制SDK头文件IYouMeVoiceEngine.h到Output目录"
#cp ${EXPORT_HEADER_PATH}/IYouMeEventCallback.h  ${SDK_INCLUDE_PATH}
#check_error $? "复制SDK头文件IYouMeEventCallback.h到Output目录"
#cp ${EXPORT_HEADER_PATH}/IYouMeVideoCallback.h  ${SDK_INCLUDE_PATH}
#check_error $? "复制SDK头文件IYouMeVideoCallback.h到Output目录"

#cp ${EXPORT_HEADER_PATH}/../objc/YMVoiceService.h  ${SDK_INCLUDE_PATH}
#check_error $? "复制SDK头文件YMVoiceService.h到Output目录"
#cp ${EXPORT_HEADER_PATH}/../objc/VoiceEngineCallback.h  ${SDK_INCLUDE_PATH}
#check_error $? "复制SDK头文件VoiceEngineCallback.h到Output目录"
#cp ${EXPORT_HEADER_PATH}/../objc/OpenGLView20.h  ${SDK_INCLUDE_PATH}
#check_error $? "复制SDK头文件OpenGLView20.h到Output目录"
#cp ${EXPORT_HEADER_PATH}/../objc/OpenGLESView.h  ${SDK_INCLUDE_PATH}
#check_error $? "复制SDK头文件OpenGLESView.h到Output目录"

#打包SDK
if [[ $BuildSdkPkg != "no" ]]; then
	if [[ $BuildMini != "no" ]]; then
		SdkNameSuffix="${nextBuildVersion}_min"
	elif [[ $BuildFfmpeg != "no" ]]; then
		SdkNameSuffix="${nextBuildVersion}"
	else 
		SdkNameSuffix="${nextBuildVersion}_no_bgm"
	fi

    if [[ $BuildAndroid != "no" ]]; then
        # android sdk 封包
        if [[ $BuildSdkPkg != "no" ]]; then
        ./buildSDKPackages.sh android        ${SdkNameSuffix}
        ./buildSDKPackages.sh android_symbol ${SdkNameSuffix}
        fi
    fi
  
    if [[ $BuildIos != "no" ]]; then
        # iOS sdk封包
        if [[ $BuildSdkPkg != "no" ]]; then
        ./buildSDKPackages.sh ios ${SdkNameSuffix}  
        fi
    fi

    if [[ $BuildMacOS != "no" ]]; then
        # macOS sdk封包
        if [[ $BuildSdkPkg != "no" ]]; then
        ./buildSDKPackages.sh macos            ${SdkNameSuffix} ${BuildUpload}    
        fi
    fi
fi


