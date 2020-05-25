LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := soundtouch
LOCAL_SRC_FILES := \
../../SoundTouch/AAFilter.cpp \
../../SoundTouch/FIFOSampleBuffer.cpp \
../../SoundTouch/FIRFilter.cpp \
../../SoundTouch/RateTransposer.cpp \
../../SoundTouch/SoundTouch.cpp \
../../SoundTouch/TDStretch.cpp \
../../SoundTouch/BPMDetect.cpp \
../../SoundTouch/PeakFinder.cpp \
../../SoundTouch/InterpolateLinear.cpp

ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_SRC_FILES += ../../SoundTouch/cpu_detect_x86.cpp \
../../SoundTouch/mmx_optimized.cpp 
endif

LOCAL_CFLAGS += -Wall -fvisibility=hidden -I$(LOCAL_PATH)../../../include -DST_NO_EXCEPTION_HANDLING -fdata-sections -Os -ffunction-sections -std=c++11 -DST_NO_EXCEPTION_HANDLING -DSOUNDTOUCH_INTEGER_SAMPLES=1

include $(BUILD_STATIC_LIBRARY)
