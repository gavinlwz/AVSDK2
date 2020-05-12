LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)



LOCAL_MODULE := webrtc

LOCAL_CPP_EXTENSION := .cc .cpp

LOCAL_CFLAGS := -DHAVE_CONFIG_H -DANDROID -frtti -D__UCLIBC__ -DWEBRTC_POSIX -DWEBRTC_LINUX -DWEBRTC_ANDROID -D__STDC_FORMAT_MACROS -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -Os -fvisibility=hidden \


LOCAL_CPPFLAGS +=\
-DHAVE_CONFIG_H -DANDROID\
-Os \
-std=c++11\
-fvisibility=hidden\
-fpermissive\

LOCAL_CFLAGS += -DWEBRTC_THREAD_RR -DWEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE -DWEBRTC_USE_H264 -DWEBRTC_INITIALIZE_FFMPEG -DNO_STL


LOCAL_C_INCLUDES := $(LOCAL_PATH)/../..\
$(LOCAL_PATH)/../../../../..\
$(LOCAL_PATH)/../../../../../tinySAK/src\
$(LOCAL_PATH)/../..\
$(LOCAL_PATH)/../../libyuv\
$(LOCAL_PATH)/../../opus-1.1/include\
$(LOCAL_PATH)/../common_audio/signal_processing/include\
$(LOCAL_PATH)/../common_audio/resampler/include\
$(LOCAL_PATH)/../modules/audio_coding/codecs/cng/include\
$(LOCAL_PATH)/../modules/audio_processing/ns\
$(LOCAL_PATH)/../modules/audio_processing/utility\
$(LOCAL_PATH)/../common_video/include\
$(LOCAL_PATH)/../modules/video_coding/include\
$(LOCAL_PATH)/../modules/video_processing/include\
$(LOCAL_PATH)../modules/video_coding/utility/\
$(LOCAL_PATH)/../../../../../youme_common/include\
$(LOCAL_PATH)/../../../../../youme_common/include/YouMeCommon\

LOCAL_SRC_FILES := \
../common_types.cpp \
../base/exp_filter.cpp \
../base/location.cpp \
../base/asyncinvoker.cpp \
../base/asyncfile.cpp \
../base/asyncresolverinterface.cpp \
../base/asyncsocket.cpp \
../base/asyncpacketsocket.cpp \
../base/asynctcpsocket.cpp \
../base/asyncudpsocket.cpp \
../base/base64.cpp \
../base/buffer.cpp \
../base/bitbuffer.cpp \
../base/bytebuffer.cpp \
../base/checks.cpp \
../base/criticalsection.cpp \
../base/event.cpp \
../base/event_tracer.cpp \
../base/ifaddrs-android.cpp \
../base/ipaddress.cpp \
../base/logging.cpp \
../base/messagehandler.cpp \
../base/messagequeue.cpp \
../base/nethelpers.cpp \
../base/physicalsocketserver.cpp \
../base/platform_thread.cpp \
../base/sharedexclusivelock.cpp \
../base/signalthread.cpp \
../base/sigslot.cpp \
../base/socketaddress.cpp \
../base/stringencode.cpp \
../base/thread.cpp\
../base/timeutils.cpp \
../system_wrappers/source/logging.cpp \
../system_wrappers/source/critical_section.cpp \
../system_wrappers/source/critical_section_posix.cpp \
../system_wrappers/source/metrics_default.cpp \
../system_wrappers/source/aligned_malloc.cpp \
../system_wrappers/source/tick_util.cpp \
../system_wrappers/source/timestamp_extrapolator.cpp \
../system_wrappers/source/rw_lock.cpp \
../system_wrappers/source/rw_lock_posix.cpp \
../system_wrappers/source/event.cpp \
../system_wrappers/source/cpu_features.cpp \
../modules/audio_processing/utility/webrtc_new/delay_estimator_wrapper_new.cpp \
../modules/audio_processing/utility/webrtc_new/delay_estimator_new.cpp \
../modules/audio_processing/utility/delay_estimator_wrapper.cpp \
../modules/audio_processing/utility/delay_estimator.cpp \
../modules/audio_processing/utility/block_mean_calculator.cpp \
../modules/audio_processing/utility/ooura_fft.cpp \
../modules/audio_processing/aecm/echo_control_mobile.cpp \
../modules/audio_processing/aecm/aecm_core.cpp \
../modules/audio_processing/aecm/aecm_core_c.cpp \
../modules/audio_processing/aec_new/aec_core_new.cpp \
../modules/audio_processing/aec_new/aec_resampler_new.cpp \
../modules/audio_processing/aec_new/echo_cancellation_new.cpp \
../modules/audio_processing/aec_new/aec_rdft_new.cpp \
../modules/audio_processing/aec/aec_core.cpp \
../modules/audio_processing/aec/aec_resampler.cpp \
../modules/audio_processing/aec/echo_cancellation.cpp \
../modules/audio_processing/agc/legacy/analog_agc.cpp \
../modules/audio_processing/agc/legacy/digital_agc.cpp \
../modules/audio_processing/ns/noise_suppression_x.cpp \
../modules/audio_processing/ns/nsx_core.cpp \
../modules/audio_processing/ns/nsx_core_c.cpp \
../modules/audio_coding/codecs/audio_decoder.cpp \
../modules/audio_coding/codecs/audio_encoder.cpp \
../modules/audio_coding/codecs/opus/audio_decoder_opus.cpp \
../modules/audio_coding/codecs/opus/audio_encoder_opus.cpp \
../modules/audio_coding/codecs/opus/opus_interface.cpp \
../modules/audio_coding/codecs/cng/webrtc_cng.cpp \
../modules/audio_coding/codecs/cng/cng_helpfuns.cpp \
../modules/audio_coding/neteq/accelerate.cpp \
../modules/audio_coding/neteq/audio_classifier.cpp \
../modules/audio_coding/neteq/audio_decoder_impl.cpp \
../modules/audio_coding/neteq/audio_multi_vector.cpp \
../modules/audio_coding/neteq/audio_vector.cpp \
../modules/audio_coding/neteq/background_noise.cpp \
../modules/audio_coding/neteq/buffer_level_filter.cpp \
../modules/audio_coding/neteq/comfort_noise.cpp \
../modules/audio_coding/neteq/decision_logic.cpp \
../modules/audio_coding/neteq/decision_logic_fax.cpp \
../modules/audio_coding/neteq/decision_logic_normal.cpp \
../modules/audio_coding/neteq/decision_logic_game_voice.cpp \
../modules/audio_coding/neteq/decoder_database.cpp \
../modules/audio_coding/neteq/delay_manager.cpp \
../modules/audio_coding/neteq/delay_peak_detector.cpp \
../modules/audio_coding/neteq/dsp_helper.cpp \
../modules/audio_coding/neteq/dtmf_buffer.cpp \
../modules/audio_coding/neteq/dtmf_tone_generator.cpp \
../modules/audio_coding/neteq/expand.cpp \
../modules/audio_coding/neteq/merge.cpp \
../modules/audio_coding/neteq/nack.cpp \
../modules/audio_coding/neteq/neteq.cpp \
../modules/audio_coding/neteq/neteq_impl.cpp \
../modules/audio_coding/neteq/normal.cpp \
../modules/audio_coding/neteq/packet_buffer.cpp \
../modules/audio_coding/neteq/payload_splitter.cpp \
../modules/audio_coding/neteq/post_decode_vad.cpp \
../modules/audio_coding/neteq/preemptive_expand.cpp \
../modules/audio_coding/neteq/random_vector.cpp \
../modules/audio_coding/neteq/rtcp.cpp \
../modules/audio_coding/neteq/statistics_calculator.cpp \
../modules/audio_coding/neteq/sync_buffer.cpp \
../modules/audio_coding/neteq/time_stretch.cpp \
../modules/audio_coding/neteq/timestamp_scaler.cpp \
../common_audio/ring_buffer.cpp \
../common_audio/audio_util.cpp \
../common_audio/signal_processing/auto_corr_to_refl_coef.c \
../common_audio/signal_processing/auto_correlation.c \
../common_audio/signal_processing/complex_fft.c \
../common_audio/signal_processing/copy_set_operations.c \
../common_audio/signal_processing/division_operations.c \
../common_audio/signal_processing/dot_product_with_scale.c \
../common_audio/signal_processing/energy.c \
../common_audio/signal_processing/filter_ar.c \
../common_audio/signal_processing/filter_ma_fast_q12.c \
../common_audio/signal_processing/get_hanning_window.c \
../common_audio/signal_processing/get_scaling_square.c \
../common_audio/signal_processing/ilbc_specific_functions.c \
../common_audio/signal_processing/levinson_durbin.c \
../common_audio/signal_processing/lpc_to_refl_coef.c \
../common_audio/signal_processing/randomization_functions.c \
../common_audio/signal_processing/real_fft.c \
../common_audio/signal_processing/refl_coef_to_lpc.c \
../common_audio/signal_processing/resample.c \
../common_audio/signal_processing/resample_48khz.c \
../common_audio/signal_processing/resample_by_2.c \
../common_audio/signal_processing/resample_by_2_internal.c \
../common_audio/signal_processing/resample_fractional.c \
../common_audio/signal_processing/spl_init.c \
../common_audio/signal_processing/spl_sqrt.c \
../common_audio/signal_processing/spl_version.c \
../common_audio/signal_processing/splitting_filter.c \
../common_audio/signal_processing/sqrt_of_one_minus_x_squared.c \
../common_audio/vad/vad_core.cpp \
../common_audio/vad/vad_filterbank.cpp \
../common_audio/vad/vad_gmm.cpp \
../common_audio/vad/vad_sp.cpp \
../common_audio/vad/webrtc_vad.cpp \
../common_video/i420_buffer_pool.cc \
../common_video/video_frame.cc \
../common_video/video_frame_buffer.cc \
../common_video/video_render_frames.cc \
../common_video/h264/h264_common.cc \
../common_video/h264/pps_parser.cc \
../common_video/h264/sps_parser.cc \
../modules/video_coding/utility/qp_parser.cc \
../modules/video_coding/utility/frame_dropper.cc \
../modules/video_coding/utility/h264_bitstream_parser.cc \
../api/java/jni/classreferenceholder.cc \
../api/java/jni/jni_helpers.cc \
../api/java/jni/eglbase_jni.cc \
../api/java/jni/native_handle_impl.cc \
../modules/video_coding/codecs/h264/androidmediaencoder_jni.cc \
../modules/video_coding/codecs/h264/h264.cc \
../modules/video_coding/codecs/h264/androidmediadecoder_jni.cc \
../modules/video_coding/codecs/h264/surfacetexturehelper_jni.cc \
../modules/utility/source/audio_frame_operations.cc \


ifeq ($(TARGET_ARCH_ABI),x86)

LOCAL_SRC_FILES += \
../modules/audio_processing/utility/ooura_fft_sse2.cpp \
../modules/audio_processing/aec_new/aec_core_sse2_new.cpp \
../modules/audio_processing/aec_new/aec_rdft_sse2_new.cpp \
../modules/audio_processing/aec/aec_core_sse2.cpp \
../common_audio/resampler/sinc_resampler_sse.cpp \

endif

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += -DHAVE_NEON -DWEBRTC_ARCH_ARM_V7 -DWEBRTC_HAS_NEON -DWEBRTC_ARCH_ARM_NEON -DWEBRTC_ANDROID -flax-vector-conversions
LOCAL_CPPFLAGS += -DHAVE_NEON -DWEBRTC_ARCH_ARM_V7 -DWEBRTC_HAS_NEON -DWEBRTC_ARCH_ARM_NEON -DWEBRTC_ANDROID -flax-vector-conversions
# vector_scaling_operations.c is necessary even if vector_scaling_operations_neon.S is included.
# because vector_scaling_operations_neon.S only optimize part of the functions in vector_scaling_operations.c
# and export with different names.
LOCAL_SRC_FILES += \
../common_audio/signal_processing/complex_bit_reverse_arm.S \
../common_audio/signal_processing/cross_correlation_neon.S \
../common_audio/signal_processing/downsample_fast_neon.S \
../common_audio/signal_processing/filter_ar_fast_q12_armv7.S \
../common_audio/signal_processing/min_max_operations_neon.S \
../common_audio/signal_processing/spl_sqrt_floor_arm.S \
../common_audio/signal_processing/vector_scaling_operations_neon.S \
../common_audio/signal_processing/vector_scaling_operations.c \
../common_audio/signal_processing/min_max_operations.c \
../common_audio/resampler/sinc_resampler_neon.cpp \
../common_audio/resampler/sinc_resampler.cpp \
../common_audio/resampler/sinusoidal_linear_chirp_source.cpp \
../common_audio/resampler/push_sinc_resampler.cpp \
../common_audio/resampler/push_resampler.cpp \
../common_audio/resampler/resampler.cpp \
../modules/audio_processing/aecm/aecm_core_neon.cpp \
../modules/audio_processing/ns/nsx_core_neon.cpp \
../modules/audio_processing/utility/ooura_fft_neon.cpp \
../modules/audio_processing/aec_new/aec_core_neon_new.cpp \
../modules/audio_processing/aec_new/aec_rdft_neon_new.cpp \
../modules/audio_processing/aec_new/aec_rdft_neon1_new.cpp \
../modules/audio_processing/aec/aec_core_neon.cpp
else
LOCAL_SRC_FILES += \
../common_audio/signal_processing/complex_bit_reverse.c \
../common_audio/signal_processing/cross_correlation.c \
../common_audio/signal_processing/downsample_fast.c \
../common_audio/signal_processing/filter_ar_fast_q12.c \
../common_audio/signal_processing/min_max_operations.c \
../common_audio/signal_processing/spl_sqrt_floor.c \
../common_audio/signal_processing/vector_scaling_operations.c \
../common_audio/resampler/sinc_resampler.cpp \
../common_audio/resampler/sinusoidal_linear_chirp_source.cpp \
../common_audio/resampler/push_sinc_resampler.cpp \
../common_audio/resampler/push_resampler.cpp \
../common_audio/resampler/resampler.cpp
endif



include $(BUILD_STATIC_LIBRARY)


