LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
ifeq ($(NEON),1)
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += -DHAVE_NEON
LOCAL_CPPFLAGS += -DHAVE_NEON
endif
endif

LOCAL_MODULE := tinyNET

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
$(LOCAL_PATH)/../../baseWrapper/src\
$(LOCAL_PATH)/../src\
$(LOCAL_PATH)/../..\
$(LOCAL_PATH)/../../tinySAK/src\

LOCAL_SRC_FILES := \
$(LOCAL_PATH)/../src/tnet.c \
$(LOCAL_PATH)/../src/tnet_endianness.c \
$(LOCAL_PATH)/../src/tnet_poll.c \
$(LOCAL_PATH)/../src/tnet_socket.c \
$(LOCAL_PATH)/../src/tnet_transport.c \
$(LOCAL_PATH)/../src/tnet_transport_poll.c \
$(LOCAL_PATH)/../src/tnet_utils.c \



include $(BUILD_STATIC_LIBRARY)


