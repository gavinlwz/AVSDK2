#!/bin/bash

################################################################################
# Print a line of information in color red
################################################################################
print_info_in_red(){
    echo -e "\033[31m ${1} \033[0m"
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
# Check the execution result of the previous command, and exit with 1 if there's
# any error
################################################################################
check_error(){
	if [[ ${1} != 0 ]]; then
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
    echo "Usage: ${0} <[ios] [android] [android_symbol] [win32] [win64] [win64_unreal] [cocos_cpp] [cocos_js] [cocos_lua]> <version info> [upload]"
    exit 1
}

################################################################################
# Check the sanity and dependency of the parameters
################################################################################
check_params(){
    if [[ $BuildPlatform = "no" ]]; then
        print_help_and_exit "必须指定一个平台"
    fi
    
    if [[ $BuildVersion = "no" ]]; then
        print_help_and_exit "必须指定版本信息"
    fi
}

################################################################################
# Start the execution flow
################################################################################

# If no parameter, print help info and exit
if [[ $# = 0 ]]; then
    print_help_and_exit
fi

BuildPlatform="no"
BuildVersion="no"
BuildUpload="no"

# Parse parameters
for var in $*
do
    if [ $var = "ios" ]; then
        BuildPlatform=${var}
    elif [ $var = "macos" ];then
    	BuildPlatform=${var}
    elif [ $var = "android" ]; then
        BuildPlatform=$var
    elif [ $var = "android_symbol" ]; then
        BuildPlatform=$var
    elif [ $var = "win32" ]; then
        BuildPlatform=$var
    elif [ $var = "win64" ]; then
        BuildPlatform=$var
    elif [ $var = "win64_unreal" ]; then
        BuildPlatform=$var
    elif [ $var = "cocos_cpp" ]; then
        BuildPlatform=$var
    elif [ $var = "cocos_js" ]; then
        BuildPlatform=$var
    elif [ $var = "cocos_lua" ]; then
        BuildPlatform=$var
    elif [ $var = "upload" ]; then
        BuildUpload=$var
    elif [[ $var != "no" && $BuildVersion = "no" ]]; then
    	BuildVersion=$var
    elif [[ $var != "no" ]]; then
    	print_help_and_exit "Invalid parameter:${var}"
    fi
done
check_params

################################################################################
# Start packaging
################################################################################

SdkNameSuffix="${BuildVersion}_${BuildPlatform}"
SdkPkgOutputDir="youme_voice_engine_package"
SdkOutDir="youme_voice_engine"
SdkOutDirWin="youme_voice_engine_win"
OutputDir="../../Output"
SrcDir="`pwd`/../"

pushd ${OutputDir}
if [ ! -d "${SdkPkgOutputDir}" ]; then
	mkdir -p "${SdkPkgOutputDir}"
	check_error $? "创建SDK打包输出目录"
fi

TempSdkDir="YoumeVideoSDK_${SdkNameSuffix}"
SdkPkgName="${TempSdkDir}.tar.gz"
if [ -d "${TempSdkDir}" ]; then
	rm -rf "${TempSdkDir}/*"
else
	mkdir -p "${TempSdkDir}"
fi

BaseSdkDir="${TempSdkDir}"
CocosSdkDir="${TempSdkDir}/${BuildPlatform}_SDK"
if [[ $BuildPlatform = "cocos_cpp"
		|| $BuildPlatform = "cocos_js"
		|| $BuildPlatform = "cocos_lua"
	]]; then
	BaseSdkDir="${TempSdkDir}/VideoEngine_SDK"
	if [ -d "${BaseSdkDir}" ]; then
		rm -rf "${BaseSdkDir}/*"
	else
		mkdir "${BaseSdkDir}"
	fi
	if [ -d "${CocosSdkDir}" ]; then
		rm -rf "${CocosSdkDir}/*"
	else
		mkdir "${CocosSdkDir}"
	fi	
fi
	
popd #${OutputDir}

################################################################################
# Copy include files
if [[ $BuildPlatform = "ios"
		|| $BuildPlatform = "macos"
		|| $BuildPlatform = "win32"
		|| $BuildPlatform = "win64"
		|| $BuildPlatform = "win64_unreal"
		|| $BuildPlatform = "cocos_cpp"
		|| $BuildPlatform = "cocos_js"
		|| $BuildPlatform = "cocos_lua"
	]]; then
	mkdir ${OutputDir}/${BaseSdkDir}/include
	cp -R ${OutputDir}/${SdkOutDir}/include/*  ${OutputDir}/${BaseSdkDir}/include
	check_error $? "Copy include"
	if [[ $BuildPlatform = "ios" ]]; then
		# cp ../SDKTest/iOSExternalInput/VoiceTest/YMVoiceService.*       ${OutputDir}/${TempSdkDir}/include
		# cp ../SDKTest/iOSExternalInput/VoiceTest/VoiceEngineCallback.h  ${OutputDir}/${TempSdkDir}/include
		check_error $? "Copy ios wrapper"
	fi

	if [[ $BuildPlatform = "macos" ]]; then
		check_error $? "Copy mac wrapper"
	fi

	if [[ $BuildPlatform = "cocos_cpp" ]]; then
		cp -R ../CocosWrappers/cocoscpp/*  ${OutputDir}/${CocosSdkDir}
		check_error $? "Copy cocos_cpp wrapper"
	fi
	
	if [[ $BuildPlatform = "cocos_js" ]]; then
		cp -R ../CocosWrappers/cocosjs\(video\)/*  ${OutputDir}/${CocosSdkDir}
		check_error $? "Copy js wrapper"
	fi
	if [[ $BuildPlatform = "cocos_lua" ]]; then
		cp -R ../CocosWrappers/cocoslua\(video\)/*  ${OutputDir}/${CocosSdkDir}
		check_error $? "Copy lua wrapper"
	fi
	if [[ $BuildPlatform = "cocos_cpp"
		|| $BuildPlatform = "cocos_js"
		|| $BuildPlatform = "cocos_lua"
	]]; then
	rm -f ${OutputDir}/${BaseSdkDir}/include/YMVoiceService.h
	rm -f ${OutputDir}/${BaseSdkDir}/include/OpenGLView20.h
    rm -f ${OutputDir}/${BaseSdkDir}/include/OpenGLESView.h
	fi
fi

################################################################################
pushd ${OutputDir}

# Move the files we want to put into the package to the temp dir
if [[ $BuildPlatform = "ios"
		|| $BuildPlatform = "cocos_cpp"
		|| $BuildPlatform = "cocos_js"
		|| $BuildPlatform = "cocos_lua"
	]]; then
	LibDir="${BaseSdkDir}/lib/ios"
	mkdir -p "${LibDir}"
	check_error $? "Create ${LibDir}"
	cp -rf ${SdkOutDir}/lib/ios/Release-universal/*  ${LibDir}
	check_error $? "copy ${SdkOutDir}/lib/ios/Release-universal/*"
	pwd
	cp "${SrcDir}ChangeLog.md" ${LibDir}/../../
	check_error $? "cp ChangeLog"
fi

if [[ $BuildPlatform = "macos" 
		|| $BuildPlatform = "cocos_cpp"
		|| $BuildPlatform = "cocos_js"
		|| $BuildPlatform = "cocos_lua"
	]]; then
	LibDir="${BaseSdkDir}/lib/macos"
	mkdir -p "${LibDir}"
	check_error $? "Create ${LibDir}"
	cp -rf ${SdkOutDir}/lib/macos/Release-macos/*  ${LibDir}
	check_error $? "copy ${SdkOutDir}/lib/macos/Release-macos/*"
	pwd
	cp "${SrcDir}ChangeLog.md" ${LibDir}/../../
	check_error $? "cp ChangeLog"
fi

if [[ $BuildPlatform = "android"
		|| $BuildPlatform = "cocos_cpp"
		|| $BuildPlatform = "cocos_js"
		|| $BuildPlatform = "cocos_lua"
	]]; then
	LibDir="${BaseSdkDir}/lib"
	if [ ! -d "${LibDir}" ]; then
		mkdir -p "${LibDir}"
		check_error $? "Create ${LibDir}"
	fi
	cp -rf ${SdkOutDir}/lib/android  ${LibDir}
	check_error $? "copy ${SdkOutDir}/lib/android"
	cp "${SrcDir}ChangeLog.md" ${LibDir}/../
	check_error $? "cp ChangeLog"
fi

if [[ $BuildPlatform = "android_symbol" ]]; then
	LibDir="${BaseSdkDir}/symbol"
	if [ ! -d "${LibDir}" ]; then
		mkdir -p "${LibDir}"
		check_error $? "Create ${LibDir}"
	fi
	mv -f ${SdkOutDir}/symbol/android  ${LibDir}
	check_error $? "move ${SdkOutDir}/lib/android"
	cp "${SrcDir}ChangeLog.md" ${LibDir}/../../
	check_error $? "cp ChangeLog"
fi


# if [[ $BuildPlatform = "win32"
# 		|| $BuildPlatform = "cocos_cpp"
# 		|| $BuildPlatform = "cocos_js"
# 		|| $BuildPlatform = "cocos_lua"
# 	]]; then
# 	LibDir="${TempSdkDir}/lib/x86"
# 	if [ ! -d "${LibDir}" ]; then
# 		mkdir -p "${LibDir}"
# 		check_error $? "Create ${LibDir}"
# 	fi
# 	mv -f ${SdkOutDirWin}/win32/*  ${LibDir}
# 	check_error $? "move ${SdkOutDirWin}/win32/*"
# 	cp "${SrcDir}ChangeLog.md" ${LibDir}/../../
# 	check_error $? "cp ChangeLog"
# fi

# if [[ $BuildPlatform = "win64"
# 		|| $BuildPlatform = "cocos_cpp"
# 		|| $BuildPlatform = "cocos_js"
# 		|| $BuildPlatform = "cocos_lua"
# 	]]; then
# 	LibDir="${TempSdkDir}/lib/x86_64"
# 	if [ ! -d "${LibDir}" ]; then
# 		mkdir -p "${LibDir}"
# 		check_error $? "Create ${LibDir}"
# 	fi
# 	mv -f ${SdkOutDirWin}/win64/*  ${LibDir}
# 	check_error $? "move ${SdkOutDirWin}/win64/*"
# 	cp "${SrcDir}ChangeLog.md" ${LibDir}/../../
# 	check_error $? "cp ChangeLog"
# fi

# if [[ $BuildPlatform = "win64_unreal" ]]; then
# 	LibDir="${TempSdkDir}/lib/x86_64"
# 	if [ ! -d "${LibDir}" ]; then
# 		mkdir -p "${LibDir}"
# 		check_error $? "Create ${LibDir}"
# 	fi
# 	mv -f ${SdkOutDirWin}/win64_unreal/*  ${LibDir}
# 	check_error $? "move ${SdkOutDirWin}/win64_unreal/*"
# 	cp "${SrcDir}ChangeLog.md" ${LibDir}/../../
# 	check_error $? "cp ChangeLog"
# fi


#################################################################################
# Move the files back to the output dir from the temp dir
		
print_info_in_red "打包SDK(${SdkPkgName})..."
if [[ $BuildPlatform = "ios"
		|| $BuildPlatform = "android"
		|| $BuildPlatform = "macos"
	]]; then
	# 打SDK包带上sample
	if [[ $BuildPlatform = "ios" ]]; then
		cp -r ${SrcDir}SDKTest/iOSExternalInput ./sample_${BuildPlatform}
		check_error $? "cp ios sample"
	fi
	if [[ $BuildPlatform = "android" ]]; then
		cp -r ${SrcDir}SDKTest/androidExternalInput ./sample_${BuildPlatform}
		check_error $? "cp android sample"
	fi
	if [[ $BuildPlatform = "macos" ]]; then
		cp -r ${SrcDir}youme_voice_engine/xcode/YmVideoMacRef  ./sample_${BuildPlatform}
		check_error $? "cp mac sample"
	fi
	rm -rf "sample_${BuildPlatform}/.DS_Store"
	rm -rf "sample_${BuildPlatform}/DebugWS.xcworkspace"
	tar czf "${SdkPkgOutputDir}/${SdkPkgName}" "${TempSdkDir}" "sample_${BuildPlatform}"
else
	tar czf "${SdkPkgOutputDir}/${SdkPkgName}" "${TempSdkDir}"
fi
show_result $? "打包SDK(${SdkPkgName})"

#################################################################################
# Move the files back to the output dir from the temp dir
if [[ $BuildPlatform = "ios"
		|| $BuildPlatform = "cocos_cpp"
		|| $BuildPlatform = "cocos_js"
		|| $BuildPlatform = "cocos_lua"
	]]; then
	mv -f ${BaseSdkDir}/lib/ios/*  ${SdkOutDir}/lib/ios/Release-universal
	check_error $? "move ${BaseSdkDir}/libs/ios/*"
fi

# if [[ $BuildPlatform = "android"
# 		|| $BuildPlatform = "cocos_cpp"
# 		|| $BuildPlatform = "cocos_js"
# 		|| $BuildPlatform = "cocos_lua"
# 	]]; then
# 	mv -f ${TempSdkDir}/lib/android  ${SdkOutDir}/lib
# 	check_error $? "move ${TempSdkDir}/libs/android"
# fi

if [[ $BuildPlatform = "android_symbol" ]]; then
	mv -f ${BaseSdkDir}/symbol/android  ${SdkOutDir}/symbol
	check_error $? "move ${BaseSdkDir}/symbol/android"
fi

# if [[ $BuildPlatform = "win32"
# 		|| $BuildPlatform = "cocos_cpp"
# 		|| $BuildPlatform = "cocos_js"
# 		|| $BuildPlatform = "cocos_lua"
# 	]]; then
# 	mv -f ${TempSdkDir}/lib/x86/*  ${SdkOutDirWin}/win32
# 	check_error $? "move ${TempSdkDir}/libs/x86/*"
# fi

# if [[ $BuildPlatform = "win64"
# 		|| $BuildPlatform = "cocos_cpp"
# 		|| $BuildPlatform = "cocos_js"
# 		|| $BuildPlatform = "cocos_lua"
# 	]]; then
# 	LibDir="${TempSdkDir}/libs/x86_64"
# 	if [ ! -d "${LibDir}" ]; then
# 		mkdir -p "${LibDir}"
# 		check_error $? "Create ${LibDir}"
# 	fi
# 	mv -f ${TempSdkDir}/lib/x86_64/*  ${SdkOutDirWin}/win64
# 	check_error $? "move ${TempSdkDir}/libs/x86_64/*"
# fi

# if [[ $BuildPlatform = "win64_unreal" ]]; then
# 	mv -f ${TempSdkDir}/lib/x86_64/*  ${SdkOutDirWin}/win64_unreal
# 	check_error $? "move ${TempSdkDir}/libs/x86_64/*"
# fi


rm -rf ${TempSdkDir}

# Upload to the shared server
if [[ $BuildUpload != "no" ]]; then
	cp ${SdkPkgOutputDir}/${SdkPkgName} /Volumes/Release/youme_voice_engine_sdk/${SdkPkgName}
	show_result $? "上传${PLATFORM} SDK包到共享服务器"
fi

#################################################################    
popd #${OutputDir}

