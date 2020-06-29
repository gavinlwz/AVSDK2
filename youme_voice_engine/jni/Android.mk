LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
$(call import-add-path,$(LOCAL_PATH))


LOCAL_CFLAGS := \
-DHAVE_CONFIG_H -DANDROID -DOS_ANDROID -DYOUME_VIDEO\
-Os \


LOCAL_CPPFLAGS +=\
-DHAVE_CONFIG_H -DANDROID -DOS_ANDROID -DYOUME_VIDEO\
-Os \
-std=c++11\
-fexceptions \

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
ifeq ($(NEON),1)
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += -DHAVE_NEON
LOCAL_CPPFLAGS += -DHAVE_NEON
endif
endif

ifeq ($(TMediaDump),1)
LOCAL_CFLAGS += -DTMEDIA_DUMP_ENABLE=1
LOCAL_CPPFLAGS += -DTMEDIA_DUMP_ENABLE=1
endif

ifeq ($(RSCode),1)
LOCAL_CFLAGS += -DTDAV_RS_CODE_ENABLE=1
LOCAL_CPPFLAGS += -DTDAV_RS_CODE_ENABLE=1
endif

LOCAL_LDFLAGS += -Wl,--gc-sections


LOCAL_MODULE := youme_voice_engine

LOCAL_CPP_EXTENSION := .cc  .cpp .cxx

LOCAL_C_INCLUDES :=  \
$(LOCAL_PATH)/..\
$(LOCAL_PATH)/../bindings/_common \
$(LOCAL_PATH)/../bindings \
$(LOCAL_PATH)/../basewrapper/src \
$(LOCAL_PATH)/../tinySAK/src \
$(LOCAL_PATH)/../tinyNET/src \
$(LOCAL_PATH)/../tinySDP/include \
$(LOCAL_PATH)/../tinyMEDIA/include \
$(LOCAL_PATH)/../tinyMEDIA/include/tinymedia \
$(LOCAL_PATH)/../tinyDAV/include \
$(LOCAL_PATH)/../tinyDAV/include/tinydav/audio \
$(LOCAL_PATH)/../tinySIP/include \
$(LOCAL_PATH)/../tinyRTP/include \
$(LOCAL_PATH)/../bindings/cocos2d-x/android \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/events \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/media \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/model \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/service \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/service/impl \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/utils \
$(LOCAL_PATH)/../bindings/cocos2d-x/interface \
$(LOCAL_PATH)/../bindings/cocos2d-x/java/jni \
$(LOCAL_PATH)/../bindings/cocos2d-x/interface/imp\
$(LOCAL_PATH)/../thirdparties/common/src\
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/datareport\
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/monitor\
$(LOCAL_PATH)/../protobuf\
$(LOCAL_PATH)/../ffmpegPlayer/player\
$(LOCAL_PATH)/../ffmpegPlayer/libs/include\
$(LOCAL_PATH)/../youme_common/include\
$(LOCAL_PATH)/../youme_common/include/YouMeCommon

LOCAL_SRC_FILES := \
$(LOCAL_PATH)/../bindings/_common/MediaSessionMgr.cxx \
$(LOCAL_PATH)/../bindings/_common/SafeObject.cxx \
$(LOCAL_PATH)/../bindings/_common/AVSessionMgr.cpp \
$(LOCAL_PATH)/../bindings/_common/ImageUtils.cpp \
$(LOCAL_PATH)/../bindings/_common/ICameraManager.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/android/CameraManager.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/NgnEngine.cpp\
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/NgnApplication.cpp\
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/service/impl/NgnNetworkService.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/service/impl/NgnLoginService.cpp\
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/service/impl/NgnMemoryConfiguration.cpp\
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/service/impl/YMVideoRecorder.cpp\
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/service/impl/YMVideoRecorderManager.cpp\
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/service/impl/YMTranscriberManager.cpp\
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/service/impl/YMTranscriberItem.cpp\
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/utils/SDKValidateTalk.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/utils/NgnConfigurationEntry.cpp\
$(LOCAL_PATH)/../protobuf/common.pb.cc \
$(LOCAL_PATH)/../protobuf/YouMeServerValidProtocol.pb.cc \
$(LOCAL_PATH)/../protobuf/YoumeRunningState.pb.cc \
$(LOCAL_PATH)/../protobuf/uploadlog.pb.cc \
$(LOCAL_PATH)/../protobuf/serverlogin.pb.cc \
$(LOCAL_PATH)/../protobuf/ProtocolBufferHelp.cpp\
$(LOCAL_PATH)/../protobuf/BandwidthControl.pb.cc\
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/monitor/MonitoringCenter.cpp\
$(LOCAL_PATH)/../bindings/cocos2d-x/interface/imp/IYouMeVoiceEngine.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/interface/imp/YouMeVoiceEngine.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/interface/imp/IYouMeCInterface.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/java/jni/com_youme_voiceengine_NativeEngine.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/monitor/ReportService.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/service/impl/NgnTalkManager.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/service/impl/NgnVideoManager.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/service/impl/VideoRender.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/service/impl/VideoRenderManager.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/service/impl/VideoUserInfo.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/service/impl/VideoRenderInfo.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/service/impl/CustomInputManager.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/service/impl/YMAudioMixer.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/interface/YouMeEngineManagerForQiniu.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/classes/monitor/AVStatistic.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/interface/YouMeEngineVideoCodec.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/interface/YouMeEngineAudioMixerUtils.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/interface/YouMeVideoMixerAdapter.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/java/jni/VideoMixerDroid.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/java/jni/YouMeApplication_Android.cpp \
$(LOCAL_PATH)/../bindings/cocos2d-x/java/jni/GLESNative.cpp \

ifeq ($(FFMPEG),1)
LOCAL_WHOLE_STATIC_LIBRARIES := tinySAK tinyNET tinySDP tinyMEDIA tinyRTP tinyDAV webrtc opus1.1 baseWrapper soundtouch ssl youmeCommon rscode avutil avformat avcodec libx264 ffplayer
else
LOCAL_WHOLE_STATIC_LIBRARIES := tinySAK tinyNET tinySDP tinyMEDIA tinyRTP tinyDAV webrtc opus1.1 baseWrapper soundtouch ssl youmeCommon rscode
endif

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_WHOLE_STATIC_LIBRARIES+=ne10
endif

LOCAL_LDLIBS := -llog -lOpenSLES -lz -lGLESv2

#ifeq ($(TARGET_ARCH_ABI),x86)
#LOCAL_STATIC_LIBRARIES = ssl_x86 crypto_x86
#else
#LOCAL_STATIC_LIBRARIES = ssl crypto
#endif


include $(BUILD_SHARED_LIBRARY)
$(call import-module,../tinySAK/jni)
$(call import-module,../tinyNET/jni)
$(call import-module,../tinySDP/jni)
$(call import-module,../tinyMEDIA/jni)
$(call import-module,../tinyRTP/jni)
$(call import-module,../tinyDAV/jni)
$(call import-module,../baseWrapper/jni)
$(call import-module,../thirdparties/common/src/webrtc/jni)
$(call import-module,../thirdparties/common/src/opus-1.1/jni)
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
$(call import-module,../thirdparties/common/src/Ne10/android/jni)
endif
$(call import-module,../thirdparties/common/src/soundtouch/source/Android-lib/jni/)
$(call import-module,../ffmpegPlayer/jni)



#include $(CLEAR_VARS)
#LOCAL_PATH = .
#ifeq ($(TARGET_ARCH_ABI),x86)
#LOCAL_MODULE := ssl_x86
#LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparties/android/x86/lib/libssl.a
#else
#LOCAL_MODULE := ssl
#LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparties/android/armeabi-v7a/lib/libssl.a
#endif
#include $(PREBUILT_STATIC_LIBRARY)

#include $(CLEAR_VARS)
#LOCAL_PATH = .
#ifeq ($(TARGET_ARCH_ABI),x86)
#LOCAL_MODULE := crypto_x86
#LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparties/android/x86/lib/libcrypto.a
#else
#LOCAL_MODULE := crypto
#LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparties/android/armeabi-v7a/lib/libcrypto.a
#endif
#include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH := .
LOCAL_MODULE := youmeCommon
ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_SRC_FILES := $(LOCAL_PATH)/../youme_common/lib/android/armeabi/libYouMeCommon.a
else ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_SRC_FILES := $(LOCAL_PATH)/../youme_common/lib/android/armeabi-v7a/libYouMeCommon.a
else ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_SRC_FILES := $(LOCAL_PATH)/../youme_common/lib/android/x86/libYouMeCommon.a
else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_SRC_FILES := $(LOCAL_PATH)/../youme_common/lib/android/arm64-v8a/libYouMeCommon.a
endif

include $(PREBUILT_STATIC_LIBRARY)

