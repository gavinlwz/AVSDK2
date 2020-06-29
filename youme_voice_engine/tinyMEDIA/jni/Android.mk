LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

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

LOCAL_MODULE := tinyMEDIA

LOCAL_CPP_EXTENSION := .cc  .cpp


LOCAL_CFLAGS += \
-DHAVE_CONFIG_H -DANDROID\
-Os \
-fvisibility=hidden\
-ffunction-sections -fdata-sections

LOCAL_CPPFLAGS +=\
-Os \
-std=c++11\
-fvisibility=hidden -fvisibility-inlines-hidden\
-ffunction-sections -fdata-sections

LOCAL_C_INCLUDES := \
$(LOCAL_PATH)/../..\
$(LOCAL_PATH)/../include\
$(LOCAL_PATH)/../include/tinymedia\
$(LOCAL_PATH)/../../tinySAK/src\
$(LOCAL_PATH)/../../tinyNET/src\
$(LOCAL_PATH)/../../tinySDP/include\
$(LOCAL_PATH)/../../tinyRTP/include\
$(LOCAL_PATH)/../../tinyRTP/include/tinyrtp/rtp\
$(LOCAL_PATH)/../../tinyDAV/include\
$(LOCAL_PATH)/../../baseWrapper/src\

LOCAL_SRC_FILES := \
$(LOCAL_PATH)/../src/tmedia.c \
$(LOCAL_PATH)/../src/tmedia_codec.c \
$(LOCAL_PATH)/../src/tmedia_common.c \
$(LOCAL_PATH)/../src/tmedia_consumer.c \
$(LOCAL_PATH)/../src/tmedia_defaults.c \
$(LOCAL_PATH)/../src/tmedia_denoise.c \
$(LOCAL_PATH)/../src/tmedia_jitterbuffer.c \
$(LOCAL_PATH)/../src/tmedia_params.c \
$(LOCAL_PATH)/../src/tmedia_producer.c \
$(LOCAL_PATH)/../src/tmedia_session.c \
$(LOCAL_PATH)/../src/tmedia_session_dummy.c \
$(LOCAL_PATH)/../src/content/tmedia_content.c \
$(LOCAL_PATH)/../src/content/tmedia_content_cpim.c \
$(LOCAL_PATH)/../src/tmedia_imageattr.c \
$(LOCAL_PATH)/../src/tmedia_converter_video.c \
$(LOCAL_PATH)/../src/tmedia_rscode.c \
$(LOCAL_PATH)/../src/tmedia_utils.c \

include $(BUILD_STATIC_LIBRARY)


