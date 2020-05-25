LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
ifeq ($(NEON),1)
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += -DHAVE_NEON
LOCAL_CPPFLAGS += -DHAVE_NEON
endif
endif

LOCAL_MODULE := tinyRTP

LOCAL_CPP_EXTENSION := .cc  .cpp


LOCAL_CFLAGS += \
-DHAVE_CONFIG_H -DANDROID  -DYOUME_VIDEO\ 
-Os \
-fvisibility=hidden\

LOCAL_CPPFLAGS +=\
-DHAVE_CONFIG_H -DANDROID  -DYOUME_VIDEO\
-Os \
-std=c++11\
-fvisibility=hidden\

LOCAL_C_INCLUDES := \
$(LOCAL_PATH)/../include\
$(LOCAL_PATH)/../include/tinyrtp \
$(LOCAL_PATH)/../include/tinyrtp/rtp \
$(LOCAL_PATH)/../include/tinyrtp/rtcp \
$(LOCAL_PATH)/../..\
$(LOCAL_PATH)/../../tinySAK/src\
$(LOCAL_PATH)/../../tinyNET/src\
$(LOCAL_PATH)/../../tinySDP/include\
$(LOCAL_PATH)/../../tinyMEDIA/include\
$(LOCAL_PATH)/../../tinyDAV/include\
$(LOCAL_PATH)/../../tinyDAV/include/tinydav/common\
$(LOCAL_PATH)/../../baseWrapper/src\


LOCAL_SRC_FILES := \
$(LOCAL_PATH)/../src/trtp.c \
$(LOCAL_PATH)/../src/trtp_manager.c \
$(LOCAL_PATH)/../src/trtp_sort.c \
$(LOCAL_PATH)/../src/trtp_statistic.c \
$(LOCAL_PATH)/../src/rtp/trtp_rtp_header.c \
$(LOCAL_PATH)/../src/rtp/trtp_rtp_packet.c \
$(LOCAL_PATH)/../src/rtcp/trtp_rtcp_header.cpp \
$(LOCAL_PATH)/../src/rtcp/trtp_rtcp_packet.cpp \
$(LOCAL_PATH)/../src/rtcp/trtp_rtcp_report_bye.cpp \
$(LOCAL_PATH)/../src/rtcp/trtp_rtcp_sdes_item.cpp \
$(LOCAL_PATH)/../src/rtcp/trtp_rtcp_report_sdes.cpp \
$(LOCAL_PATH)/../src/rtcp/trtp_rtcp_report_xr.cpp \
$(LOCAL_PATH)/../src/rtcp/trtp_rtcp_rblock.cpp \
$(LOCAL_PATH)/../src/rtcp/trtp_rtcp_report.cpp \
$(LOCAL_PATH)/../src/rtcp/trtp_rtcp_report_rr.cpp \
$(LOCAL_PATH)/../src/rtcp/trtp_rtcp_sdes_chunck.cpp \
$(LOCAL_PATH)/../src/rtcp/trtp_rtcp_report_fb.cpp \
$(LOCAL_PATH)/../src/rtcp/trtp_rtcp_report_sr.cpp \
$(LOCAL_PATH)/../src/rtcp/trtp_rtcp_session.cpp 


include $(BUILD_STATIC_LIBRARY)


