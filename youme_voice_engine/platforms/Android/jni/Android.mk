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
$(LOCAL_PATH)/../../.. \
$(LOCAL_PATH)/../../../bindings/_common \
$(LOCAL_PATH)/../../../bindings/cocos2d-x/android \
$(LOCAL_PATH)/../../../bindings/cocos2d-x/classes \
$(LOCAL_PATH)/../../../bindings/cocos2d-x/classes/service \
$(LOCAL_PATH)/../../../bindings/cocos2d-x/classes/utils \
$(LOCAL_PATH)/../../../bindings/cocos2d-x/classes/monitor \
$(LOCAL_PATH)/../../../bindings/cocos2d-x/interface \
$(LOCAL_PATH)/../../../basewrapper/src \
$(LOCAL_PATH)/../../../tinySAK/src \
$(LOCAL_PATH)/../../../tinyNET/src \
$(LOCAL_PATH)/../../../tinySDP/include \
$(LOCAL_PATH)/../../../tinyMEDIA/include \
$(LOCAL_PATH)/../../../tinyMEDIA/include/tinymedia \
$(LOCAL_PATH)/../../../tinyDAV/include \
$(LOCAL_PATH)/../../../tinyDAV/include/tinydav/audio \
$(LOCAL_PATH)/../../../tinySIP/include \
$(LOCAL_PATH)/../../../tinyRTP/include \
$(LOCAL_PATH)/../../../thirdparties/common/src \
$(LOCAL_PATH)/../../../protobuf \
$(LOCAL_PATH)/../../../ffmpegPlayer/player \
$(LOCAL_PATH)/../../../ffmpegPlayer/libs/include \
$(LOCAL_PATH)/../../../youme_common/include \
$(LOCAL_PATH)/../../../youme_common/include/YouMeCommon \
$(LOCAL_PATH)/../java/jni

BINDINGS_COMMON := $(wildcard $(LOCAL_PATH)/../../../bindings/_common/*.cxx) \
				   $(wildcard $(LOCAL_PATH)/../../../bindings/_common/*.cpp)

BINDINGS_COCOS2D := $(wildcard $(LOCAL_PATH)/../../../bindings/cocos2d-x/android/*.cpp) \
					$(wildcard $(LOCAL_PATH)/../../../bindings/cocos2d-x/classes/*.cpp) \
					$(wildcard $(LOCAL_PATH)/../../../bindings/cocos2d-x/classes/service/*.cpp) \
					$(wildcard $(LOCAL_PATH)/../../../bindings/cocos2d-x/classes/utils/*.cpp) \
					$(wildcard $(LOCAL_PATH)/../../../bindings/cocos2d-x/classes/monitor/*.cpp) \
					$(wildcard $(LOCAL_PATH)/../../../bindings/cocos2d-x/interface/*.cpp)

PROTOBUF := $(wildcard $(LOCAL_PATH)/../../../protobuf/*.cc) \
			$(wildcard $(LOCAL_PATH)/../../../protobuf/*.cpp)

JAVA_JNI := $(wildcard $(LOCAL_PATH)/../java/jni/*.cpp)

LOCAL_SRC_FILES := $(BINDINGS_COMMON) $(PROTOBUF) $(JAVA_JNI) $(BINDINGS_COCOS2D)


ifeq ($(FFMPEG),1)
LOCAL_WHOLE_STATIC_LIBRARIES := tinySAK tinyNET tinySDP tinyMEDIA tinyRTP tinyDAV webrtc opus1.1 baseWrapper soundtouch ssl youmeCommon rscode avutil avformat avcodec libx264 ffplayer
else
LOCAL_WHOLE_STATIC_LIBRARIES := tinySAK tinyNET tinySDP tinyMEDIA tinyRTP tinyDAV webrtc opus1.1 baseWrapper soundtouch ssl youmeCommon rscode
endif

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_WHOLE_STATIC_LIBRARIES+=ne10
endif

LOCAL_LDLIBS := -llog -lOpenSLES -lz -lGLESv2


include $(BUILD_SHARED_LIBRARY)
$(call import-module,../../../tinySAK/jni)
$(call import-module,../../../tinyNET/jni)
$(call import-module,../../../tinySDP/jni)
$(call import-module,../../../tinyMEDIA/jni)
$(call import-module,../../../tinyRTP/jni)
$(call import-module,../../../tinyDAV/jni)
$(call import-module,../../../baseWrapper/jni)
$(call import-module,../../../thirdparties/common/src/webrtc/jni)
$(call import-module,../../../thirdparties/common/src/opus-1.1/jni)
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
$(call import-module,../../../thirdparties/common/src/Ne10/android/jni)
endif
$(call import-module,../../../thirdparties/common/src/soundtouch/source/Android-lib/jni/)
$(call import-module,../../../ffmpegPlayer/jni)


include $(CLEAR_VARS)
LOCAL_PATH := .
LOCAL_MODULE := youmeCommon
ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../youme_common/lib/android/armeabi/libYouMeCommon.a
else ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../youme_common/lib/android/armeabi-v7a/libYouMeCommon.a
else ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../youme_common/lib/android/x86/libYouMeCommon.a
else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../youme_common/lib/android/arm64-v8a/libYouMeCommon.a
endif

include $(PREBUILT_STATIC_LIBRARY)

