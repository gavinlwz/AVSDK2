LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
ifeq ($(NEON),1)
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += -DHAVE_NEON
LOCAL_CPPFLAGS += -DHAVE_NEON
endif
endif

LOCAL_MODULE := tinySDP

LOCAL_CPP_EXTENSION := .cc  .cpp 


LOCAL_CFLAGS += \
-DANDROID\
-Os \
-fvisibility=hidden\

LOCAL_CPPFLAGS +=\
-DANDROID\
-Os \
-std=c++11\
-fvisibility=hidden\

LOCAL_C_INCLUDES := \
$(LOCAL_PATH)/../..\
$(LOCAL_PATH)/../include\
$(LOCAL_PATH)/../../tinySAK/src\
$(LOCAL_PATH)/../../bindings/cocos2d-x/service/impl\
$(LOCAL_PATH)/../../tinyMEDIA/include\
$(LOCAL_PATH)/../../tinyMEDIA/include/tinymedia\



LOCAL_SRC_FILES := \
$(LOCAL_PATH)/../src/tsdp.c \
$(LOCAL_PATH)/../src/tsdp_message.c \
$(LOCAL_PATH)/../src/headers/tsdp_header.c \
$(LOCAL_PATH)/../src/headers/tsdp_header_A.c \
$(LOCAL_PATH)/../src/headers/tsdp_header_C.c \
$(LOCAL_PATH)/../src/headers/tsdp_header_M.c \
$(LOCAL_PATH)/../src/headers/tsdp_header_O.c \
$(LOCAL_PATH)/../src/parsers/tsdp_parser_message.c




include $(BUILD_STATIC_LIBRARY)


