LOCAL_PATH := $(call my-dir)

# If FFMPEG will not be supported, do not compile this module at all
ifeq ($(FFMPEG),1)

# Copy the prebuilt FFMpeg libs
include $(CLEAR_VARS)
LOCAL_MODULE := avutil
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libs/android/$(TARGET_ARCH_ABI)/lib/libavutil.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avformat
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libs/android/$(TARGET_ARCH_ABI)/lib/libavformat.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avcodec
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libs/android/$(TARGET_ARCH_ABI)/lib/libavcodec.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libx264
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libs/android/$(TARGET_ARCH_ABI)/lib/libx264/libx264.a
include $(PREBUILT_STATIC_LIBRARY)


# Build the FFMpeg Player
include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
ifeq ($(NEON),1)
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += -DHAVE_NEON
LOCAL_CPPFLAGS += -DHAVE_NEON
endif
endif

LOCAL_MODULE := ffplayer

LOCAL_CPP_EXTENSION := .cc  .cpp  .cxx 

LOCAL_CFLAGS += \
-DHAVE_CONFIG_H -DANDROID\
-Os \
-fvisibility=hidden\

LOCAL_CPPFLAGS +=\
-DHAVE_CONFIG_H -DANDROID\
-Os \
-std=c++11\
-fvisibility=hidden -fvisibility-inlines-hidden\

LOCAL_C_INCLUDES := \
$(LOCAL_PATH)/../libs/include \
$(LOCAL_PATH)/../player \
$(LOCAL_PATH)/../.. \
$(LOCAL_PATH)/../../tinySAK/src \

LOCAL_SRC_FILES := \
$(LOCAL_PATH)/../player/IFFMpegDecoder.cpp \
$(LOCAL_PATH)/../player/FFMpegDecoder.cpp \

include $(BUILD_STATIC_LIBRARY)

endif #ifeq ($(FFMPEG),1)


