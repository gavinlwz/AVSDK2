LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
ifeq ($(NEON),1)
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += -DHAVE_NEON
LOCAL_CPPFLAGS += -DHAVE_NEON
endif
endif

LOCAL_MODULE := tinySAK

LOCAL_CPP_EXTENSION := .cc  .cpp

LOCAL_CFLAGS += \
-DHAVE_CONFIG_H -DANDROID\
-Os \
-fvisibility=hidden\

LOCAL_CPPFLAGS +=\
-DHAVE_CONFIG_H -DANDROID\
-Os \
-std=c++11\
-fvisibility=hidden\

LOCAL_C_INCLUDES := \
$(LOCAL_PATH)/../src\
$(LOCAL_PATH)/../.. \
$(LOCAL_PATH)/../../baseWrapper/src\

LOCAL_SRC_FILES := \
$(LOCAL_PATH)/../src/tsk.c \
$(LOCAL_PATH)/../src/tsk_base64.c \
$(LOCAL_PATH)/../src/tsk_binaryutils.c \
$(LOCAL_PATH)/../src/tsk_buffer.c \
$(LOCAL_PATH)/../src/tsk_debug.c \
$(LOCAL_PATH)/../src/tsk_fsm.c \
$(LOCAL_PATH)/../src/tsk_hmac.c \
$(LOCAL_PATH)/../src/tsk_list.c \
$(LOCAL_PATH)/../src/tsk_log.c \
$(LOCAL_PATH)/../src/tsk_md5.c \
$(LOCAL_PATH)/../src/tsk_memory.c \
$(LOCAL_PATH)/../src/tsk_mutex.c \
$(LOCAL_PATH)/../src/tsk_object.c \
$(LOCAL_PATH)/../src/tsk_options.c \
$(LOCAL_PATH)/../src/tsk_params.c \
$(LOCAL_PATH)/../src/tsk_plugin.c \
$(LOCAL_PATH)/../src/tsk_ppfcs16.c \
$(LOCAL_PATH)/../src/tsk_ppfcs32.c \
$(LOCAL_PATH)/../src/tsk_ragel_state.c \
$(LOCAL_PATH)/../src/tsk_runnable.c \
$(LOCAL_PATH)/../src/tsk_safeobj.c \
$(LOCAL_PATH)/../src/tsk_semaphore.c \
$(LOCAL_PATH)/../src/tsk_sha1.c \
$(LOCAL_PATH)/../src/tsk_string.c \
$(LOCAL_PATH)/../src/tsk_thread.c \
$(LOCAL_PATH)/../src/tsk_time.c \
$(LOCAL_PATH)/../src/tsk_url.c \
$(LOCAL_PATH)/../src/tsk_uuid.c \
$(LOCAL_PATH)/../src/tsk_xml.c \
$(LOCAL_PATH)/../src/tsk_cond.c \


include $(BUILD_STATIC_LIBRARY)


