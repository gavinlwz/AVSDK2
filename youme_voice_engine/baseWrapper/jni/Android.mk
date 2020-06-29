LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
ifeq ($(NEON),1)
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += -DHAVE_NEON
LOCAL_CPPFLAGS += -DHAVE_NEON
endif
endif

LOCAL_MODULE := baseWrapper

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

LOCAL_C_INCLUDES :=  \
$(LOCAL_PATH)/../..\
$(LOCAL_PATH)/../include \
$(LOCAL_PATH)/../include/api\
$(LOCAL_PATH)/../include/authentication\
$(LOCAL_PATH)/../include/dialogs\
$(LOCAL_PATH)/../include/headers\
$(LOCAL_PATH)/../include/parsers\
$(LOCAL_PATH)/../include/sigcomp\
$(LOCAL_PATH)/../include/transactions\
$(LOCAL_PATH)/../include/transports\
$(LOCAL_PATH)/../../tinyMEDIA/include\
$(LOCAL_PATH)/../../tinySDP/include\
$(LOCAL_PATH)/../../tinyHTTP/include\
$(LOCAL_PATH)/../../tinySAK/src\
$(LOCAL_PATH)/../../tinyNET/src\
$(LOCAL_PATH)/../../tinyIPSec/src\
$(LOCAL_PATH)/../../tinySIGCOMP/src\
$(LOCAL_PATH)/../../thirdparties/common/src\
$(LOCAL_PATH)/../../bindings/cocos2d-x/classes/service/impl\
$(LOCAL_PATH)/../../baseWrapper/src\
$(LOCAL_PATH)/../../youme_common/include \
$(LOCAL_PATH)/../../bindings/cocos2d-x/classes/monitor \
$(LOCAL_PATH)/../../protobuf \
$(LOCAL_PATH)/../../youme_common/include/YouMeCommon

LOCAL_SRC_FILES := \
$(LOCAL_PATH)/../src/SyncTCPTalk.cpp\
$(LOCAL_PATH)/../src/XTimer.cpp\
$(LOCAL_PATH)/../src/XTimerCWrapper.cpp\
$(LOCAL_PATH)/../src/mem_pool.cpp\
$(LOCAL_PATH)/../src/XConfigCWrapper.cpp\
$(LOCAL_PATH)/../src/TLSMemory.cpp

include $(BUILD_STATIC_LIBRARY)


