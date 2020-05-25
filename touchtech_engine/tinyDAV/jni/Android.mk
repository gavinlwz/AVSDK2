LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
ifeq ($(NEON),1)
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += -DHAVE_NEON
LOCAL_CPPFLAGS += -DHAVE_NEON
endif
endif


LOCAL_CFLAGS += -DFF_API_SWS_GETCONTEXT
LOCAL_CPPFLAGS += -DFF_API_SWS_GETCONTEXT

LOCAL_MODULE := tinyDAV

LOCAL_CPP_EXTENSION := .cc  .cpp .cxx





LOCAL_CFLAGS += \
-DHAVE_CONFIG_H -DANDROID -DOS_ANDROID \
-O3 \
-fvisibility=hidden \
-ffunction-sections -fdata-sections

LOCAL_CPPFLAGS +=\
-DHAVE_CONFIG_H -DANDROID -DOS_ANDROID \
-O3 \
-std=c++11 \
-fvisibility=hidden -fvisibility-inlines-hidden\
-ffunction-sections -fdata-sections\

LOCAL_C_INCLUDES :=  \
$(LOCAL_PATH)/../..\
$(LOCAL_PATH)/../../interface \
$(LOCAL_PATH)/../../interface/common \
$(LOCAL_PATH)/../../interface/classes \
$(LOCAL_PATH)/../../interface/classes/service \
$(LOCAL_PATH)/../../interface/classes/monitor \
$(LOCAL_PATH)/../../interface/classes/utils \
$(LOCAL_PATH)/../../baseWrapper/src \
$(LOCAL_PATH)/../../thirdparties/common/include \
$(LOCAL_PATH)/../../thirdparties/common/src/opus-1.1/include \
$(LOCAL_PATH)/../../thirdparties/common/src/webrtc/common_audio/vad/include \
$(LOCAL_PATH)/../../thirdparties/common/src/webrtc/modules/audio_processing/aecm \
$(LOCAL_PATH)/../../thirdparties/common/src/webrtc/modules/audio_processing/aec_new\
$(LOCAL_PATH)/../../thirdparties/common/src/webrtc/modules/audio_processing/aec \
$(LOCAL_PATH)/../../thirdparties/common/src/webrtc/modules/audio_processing/ns \
$(LOCAL_PATH)/../../thirdparties/common/src/webrtc/modules/audio_processing/agc/legacy \
$(LOCAL_PATH)/../../thirdparties/common/src/webrtc/modules/audio_coding/codecs/cng \
$(LOCAL_PATH)/../../thirdparties/common/src/soundtouch/include \
$(LOCAL_PATH)/../../thirdparties/common/src \
$(LOCAL_PATH)/../../thirdparties/common/src/rscode-1.3 \
$(LOCAL_PATH)/../../protobuf \
$(LOCAL_PATH)/../../tinyRTP/include \
$(LOCAL_PATH)/../../tinyRTP/include/tinyrtp \
$(LOCAL_PATH)/../../tinyRTP/include/tinyrtp/rtp \
$(LOCAL_PATH)/../../tinyMEDIA/include \
$(LOCAL_PATH)/../../tinySDP/include \
$(LOCAL_PATH)/../../tinyBFCP/include \
$(LOCAL_PATH)/../../tinyMSRP/include \
$(LOCAL_PATH)/../../tinySAK/src \
$(LOCAL_PATH)/../../tinyNET/src \
$(LOCAL_PATH)/../../tinyIPSec/src \
$(LOCAL_PATH)/../../tinyMEDIA/include \
$(LOCAL_PATH)/../../youme_common/include \
$(LOCAL_PATH)/../../youme_common/include/YouMeCommon \
$(LOCAL_PATH)/../../ffmpegPlayer/libs/include \
$(LOCAL_PATH)/../../ffmpegPlayer/libs/include/android \
$(LOCAL_PATH)/../include \
$(LOCAL_PATH)/../include/tinydav/common \
$(LOCAL_PATH)/../include/tinydav/video \
$(LOCAL_PATH)/../include/tinydav/audio \
$(LOCAL_PATH)/../src/audio/audio_opensles \
$(LOCAL_PATH)/../src/audio/android \

LOCAL_SRC_FILES := \
$(LOCAL_PATH)/../src/tdav.c \
$(LOCAL_PATH)/../src/tdav_session_av.c \
$(LOCAL_PATH)/../src/audio/tdav_consumer_audio.c \
$(LOCAL_PATH)/../src/audio/tdav_producer_audio.c \
$(LOCAL_PATH)/../src/audio/tdav_session_audio.cpp \
$(LOCAL_PATH)/../src/audio/tdav_webrtc_denoise.cpp \
$(LOCAL_PATH)/../src/audio/youme_jitter.cpp \
$(LOCAL_PATH)/../src/audio/tdav_youme_neteq_jitterbuffer.cpp \
$(LOCAL_PATH)/../src/audio/tdav_audio_unit.c \
$(LOCAL_PATH)/../src/audio/audio_opensles/audio_opensles.cxx \
$(LOCAL_PATH)/../src/audio/audio_opensles/audio_opensles_consumer.cxx \
$(LOCAL_PATH)/../src/audio/audio_opensles/audio_opensles_device.cxx \
$(LOCAL_PATH)/../src/audio/audio_opensles/audio_opensles_device_impl.cxx \
$(LOCAL_PATH)/../src/audio/audio_opensles/audio_opensles_producer.cxx \
$(LOCAL_PATH)/../src/audio/android/audio_android_producer.cxx \
$(LOCAL_PATH)/../src/audio/android/audio_android_consumer.cxx \
$(LOCAL_PATH)/../src/audio/android/audio_android.cxx \
$(LOCAL_PATH)/../src/audio/android/audio_android_device_impl.cxx \
$(LOCAL_PATH)/../src/codecs/opus/tdav_codec_opus.cpp \
$(LOCAL_PATH)/../src/codecs/h264/tdav_codec_h264_rtp.cpp \
$(LOCAL_PATH)/../src/codecs/h264/tdav_codec_h264.cpp \
$(LOCAL_PATH)/../src/codecs/mixer/tdav_codec_audio_mixer.cpp \
$(LOCAL_PATH)/../src/codecs/mixer/tdav_codec_audio_resample.cpp \
$(LOCAL_PATH)/../src/codecs/mixer/tdav_codec_audio_howling_supp.cpp \
$(LOCAL_PATH)/../src/codecs/mixer/tdav_codec_audio_biquad_filter.cpp \
$(LOCAL_PATH)/../src/codecs/mixer/tdav_codec_audio_fftwrap.cpp \
$(LOCAL_PATH)/../src/codecs/mixer/tdav_codec_audio_kiss_fft.cpp \
$(LOCAL_PATH)/../src/codecs/mixer/tdav_codec_audio_kiss_fftr.cpp \
$(LOCAL_PATH)/../src/codecs/bandwidth_ctrl/tdav_codec_bandwidth_ctrl.cpp \
$(LOCAL_PATH)/../src/codecs/rtp_extension/tdav_codec_rtp_extension.cpp \
$(LOCAL_PATH)/../src/video/tdav_consumer_video.c \
$(LOCAL_PATH)/../src/video/tdav_producer_video.c \
$(LOCAL_PATH)/../src/video/tdav_converter_video.cxx \
$(LOCAL_PATH)/../src/video/tdav_runnable_video.c \
$(LOCAL_PATH)/../src/video/tdav_session_video.cpp \
$(LOCAL_PATH)/../src/video/jb/tdav_video_frame.c \
$(LOCAL_PATH)/../src/video/jb/tdav_video_jb.cpp \
$(LOCAL_PATH)/../src/video/jb/youme_video_jitter.cpp \
$(LOCAL_PATH)/../src/video/common/video.cpp \
$(LOCAL_PATH)/../src/video/common/video_consumer.cpp \
$(LOCAL_PATH)/../src/video/common/video_producer.cpp \
$(LOCAL_PATH)/../src/video/common/video_device_impl.cpp \
$(LOCAL_PATH)/../src/common/tdav_rscode.c \
$(LOCAL_PATH)/../src/common/tdav_audio_rscode.c \

include $(BUILD_STATIC_LIBRARY)


