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
$(LOCAL_PATH)/../../../interface \
$(LOCAL_PATH)/../../../interface/classes \
$(LOCAL_PATH)/../../../interface/classes/utils \
$(LOCAL_PATH)/../../../interface/classes/monitor \
$(LOCAL_PATH)/../../../interface/classes/service \
$(LOCAL_PATH)/../../../interface/common \
$(LOCAL_PATH)/../../../interface/android \
$(LOCAL_PATH)/../../../platforms/Android/jni \
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
$(LOCAL_PATH)/../../../protobuf\
$(LOCAL_PATH)/../../../ffmpegPlayer/player\
$(LOCAL_PATH)/../../../ffmpegPlayer/libs/include\
$(LOCAL_PATH)/../../../youme_common/include\
$(LOCAL_PATH)/../../../youme_common/include/YouMeCommon \
$(LOCAL_PATH)/../../../thirdparties/common/src \


Interface_path := $(LOCAL_PATH)/../../../interface
Interface_files := $(shell find $(Interface_path))
Interface_src_files := $(filter %.cpp %.c %.cxx,$(Interface_files))

Protobuf_path := $(LOCAL_PATH)/../../../protobuf
Protobuf_files := $(shell find $(Protobuf_path))
Protobuf_src_files := $(filter %.cc %.cpp,$(Protobuf_files))

Jni_path := $(LOCAL_PATH)/../../../platforms/Android/jni
Jni_files := $(shell find $(Jni_path))
Jni_src_files := $(filter %.cc %.cpp,$(Jni_files))

LOCAL_SRC_FILES += $(Jni_src_files) $(Interface_src_files) $(Protobuf_src_files)

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
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../youme_common/lib/android/armeabi/libYouMeCommon.a
else ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../youme_common/lib/android/armeabi-v7a/libYouMeCommon.a
else ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../youme_common/lib/android/x86/libYouMeCommon.a
else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../youme_common/lib/android/arm64-v8a/libYouMeCommon.a
endif

include $(PREBUILT_STATIC_LIBRARY)

