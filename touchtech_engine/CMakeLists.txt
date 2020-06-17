project (youme_voice_engine)

# Precompiled macro and Compile option
IF (WIN32)
ADD_DEFINITIONS(-DCURL_STATICLIB -DCARES_STATICLIB -DWIN32_LEAN_AND_MEAN -DYOUMEDLL_EXPORTS -DTINYSAK_IMPORTS_IGNORE -DHAVE_CONFIG_H -DTINYNET_IMPORTS_IGNORE -DTNET_UNDER_WINDOWS -DTINYMEDIA_IMPORTS_IGNORE -DTINYSDP_IMPORTS_IGNORE -DTINYRTP_IMPORTS_IGNORE -DTINYDAV_IMPORTS_IGNORE -D_XKEYCHECK_H -DHAVE_WAVE_API -DUSE_ALLOCA -DOPUS_BUILD -DWEBRTC_WIN -DNOMINMAX -DTINYWRAP_IMPORTS_IGNORE -DWIN_FFMPEG_SUPPORT=1 -DYOUME_VIDEO=1 -DLIBSCREEN_EXPORTS)

ELSEIF (__LINUX__)
ADD_DEFINITIONS(-DCURL_STATICLIB -DCARES_STATICLIB -DWEBRTC_LINUX -DYOUMEDLL_EXPORTS -DTINYSAK_IMPORTS_IGNORE -DHAVE_CONFIG_H -DTINYNET_IMPORTS_IGNORE -DTINYMEDIA_IMPORTS_IGNORE -DTINYSDP_IMPORTS_IGNORE -DTINYRTP_IMPORTS_IGNORE -DTINYDAV_IMPORTS_IGNORE -D_XKEYCHECK_H -DUSE_ALLOCA -DOPUS_BUILD -DNOMINMAX -DTINYWRAP_IMPORTS_IGNORE  -DYOUME_VIDEO=1 -D__UCLIBC__ -DWEBRTC_POSIX -DOS_LINUX -DYOUME_VIDEO -D__STDC_FORMAT_MACROS -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS)

ENDIF()

# header path
IF (WIN32)
INCLUDE_DIRECTORIES (..
                    .
                    ./baseWrapper/src
                    ./youme_common/include 
                    ./youme_common/include/YouMeCommon
                    ./youme_common/include/libNls
                    ./youme_common/include/libNls/pthread
                    ./tinySAK/src ./tinyMEDIA/include
                    ./tinySDP/include 
                    ./tinyMEDIA/include/tinymedia 
                    ./tinyNET/src 
                    ./tinyRTP/include 
                    ./bindings/cocos2d-x/classes/service/impl 
                    ./tinyDAV/include 
                    ./thirdparties/common/src 
                    ./protobuf 
                    ./thirdparties/common/src/opus-1.1/include 
                    ./bindings/cocos2d-x/classes/monitor 
                    ./thirdparties/common/src/webrtc/modules/audio_processing/aecm 
                    ./thirdparties/common/src/webrtc/modules/audio_processing/aec 
                    ./thirdparties/common/src/webrtc/modules/audio_processing/ns 
                    ./thirdparties/common/src/webrtc/common_audio/vad/include 
                    ./thirdparties/common/src/webrtc/modules/audio_coding/codecs/cng 
                    ./thirdparties/common/src/webrtc/modules/audio_processing/agc/legacy 
                    ./thirdparties/common/src/soundtouch/include 
                    ./thirdparties/common/src/opus-1.1/celt 
                    ./thirdparties/common/src/opus-1.1/silk/fixed 
                    ./thirdparties/common/src/opus-1.1/silk 
                    ./thirdparties/common/src/webrtc/common_audio/signal_processing/include 
                    ./bindings/cocos2d-x/classes/utils 
                    ./bindings/cocos2d-x/classes 
                    ./bindings/cocos2d-x/interface 
                    ./bindings/cocos2d-x/classes/service  
                    ./bindings/_common 
                    ./bindings/cocos2d-x/interface/imp 
                    ./thirdparties/common/src/zlib 
                    ./ffmpegplayer/libs/include 
                    ./tinyDAV/include/tinydav 
                    ./tinyDAV/include/tinydav/video 
                    ./tinyDAV/include/tinydav/audio 
                    ./bindings/cocos2d-x/win 
                    ./tinyDAV/include/tinydav/common 
                    ./tinyRTP/include/tinyrtp 
                    ./tinyRTP/include/tinyrtp/rtp 
                    ./tinyRTP/include/tinyrtp/rtcp 
                    ./tinyDAV/include/tinydav/audio/coreaudio 
                    ./bindings/cocos2d-x/win/DShow/include 
                    ./bindings/cocos2d-x/win/SDL2/include 
                    ./ffmpegPlayer/player)
ElSE (__LINUX__)
INCLUDE_DIRECTORIES (..
                    .
                    ./baseWrapper/src
                    ./youme_common/include 
                    ./youme_common/include/YouMeCommon
                    ./tinySAK/src ./tinyMEDIA/include
                    ./tinySDP/include 
                    ./tinyMEDIA/include/tinymedia 
                    ./tinyNET/src 
                    ./tinyRTP/include 
                    ./bindings/cocos2d-x/classes/service/impl 
                    ./tinyDAV/include 
                    ./thirdparties/common/src 
                    ./protobuf 
                    ./thirdparties/common/src/opus-1.1/include 
                    ./bindings/cocos2d-x/classes/monitor 
                    ./thirdparties/common/src/webrtc/modules/audio_processing/aecm 
                    ./thirdparties/common/src/webrtc/modules/audio_processing/aec 
                    ./thirdparties/common/src/webrtc/modules/audio_processing/ns 
                    ./thirdparties/common/src/webrtc/common_audio/vad/include 
                    ./thirdparties/common/src/webrtc/modules/audio_coding/codecs/cng 
                    ./thirdparties/common/src/webrtc/modules/audio_processing/agc/legacy 
                    ./thirdparties/common/src/soundtouch/include 
                    ./thirdparties/common/src/opus-1.1/celt 
                    ./thirdparties/common/src/opus-1.1/silk/fixed 
                    ./thirdparties/common/src/opus-1.1/silk 
                    ./thirdparties/common/src/webrtc/common_audio/signal_processing/include 
                    ./bindings/cocos2d-x/classes/utils 
                    ./bindings/cocos2d-x/classes 
                    ./bindings/cocos2d-x/interface 
                    ./bindings/cocos2d-x/classes/service  
                    ./bindings/_common 
                    ./bindings/cocos2d-x/interface/imp
                    ./bindings/cocos2d-x/linux
                    ./thirdparties/common/src/zlib 
                    ./ffmpegplayer/libs/include 
                    ./tinyDAV/include/tinydav 
                    ./tinyDAV/include/tinydav/video 
                    ./tinyDAV/include/tinydav/audio 
                    ./tinyDAV/include/tinydav/common 
                    ./tinyRTP/include/tinyrtp 
                    ./tinyRTP/include/tinyrtp/rtp 
                    ./tinyRTP/include/tinyrtp/rtcp 
                    ./tinyDAV/include/tinydav/audio/alsa 
                    ../SDKTest/linux
                    ../SDKTest/linux/include)
ENDIF()

FILE (GLOB header_files *.h)
FILE (GLOB source_files *.c *.cc *.cpp)

#pb
FILE (GLOB protobuf protobuf/*.h protobuf/*.cpp protobuf/*.cc)
SOURCE_GROUP("protobuf" FILES ${protobuf})

#baseWrapper
FILE (GLOB baseWrapper baseWrapper/src/*.h baseWrapper/src/*.cpp baseWrapper/src/*.cc)
SOURCE_GROUP("baseWrapper\\src" FILES ${baseWrapper})

#soundtouch
if (NO_SOUNDTOUCH)
	ADD_DEFINITIONS(-DNO_SOUNDTOUCH)

else(NO_SOUNDTOUCH)
	FILE (GLOB soundtouch_include thirdparties/common/src/soundtouch/include/*.h)
	SOURCE_GROUP("thirdparties\\common\\src\\soundtouch\\include" FILES ${soundtouch_include})

	FILE (GLOB soundtouch_SoundStretch thirdparties/common/src/soundtouch/source/SoundStretch/*.h thirdparties/common/src/soundtouch/source/SoundStretch/*.c thirdparties/common/src/soundtouch/source/SoundStretch/*.cpp)
	SOURCE_GROUP("thirdparties\\common\\src\\soundtouch\\source\\SoundStretch" FILES ${soundtouch_SoundStretch})

	FILE (GLOB soundtouch_SoundTouch thirdparties/common/src/soundtouch/source/SoundTouch/*.h thirdparties/common/src/soundtouch/source/SoundTouch/*.c thirdparties/common/src/soundtouch/source/SoundTouch/*.cpp)
	SOURCE_GROUP("thirdparties\\common\\src\\soundtouch\\source\\SoundTouch" FILES ${soundtouch_SoundTouch})

endif(NO_SOUNDTOUCH)

#opus
FILE (GLOB opus_celt_arm thirdparties/common/src/opus-1.1/celt/arm/*.h thirdparties/common/src/opus-1.1/celt/arm/*.c thirdparties/common/src/opus-1.1/celt/arm/*.cpp)
SOURCE_GROUP("thirdparties\\common\\src\\opus-1.1\\celt\\arm" FILES ${opus_celt_arm})

FILE (GLOB opus_celt thirdparties/common/src/opus-1.1/celt/*.h thirdparties/common/src/opus-1.1/celt/*.c thirdparties/common/src/opus-1.1/celt/*.cpp)
SOURCE_GROUP("thirdparties\\common\\src\\opus-1.1\\celt" FILES ${opus_celt})

FILE (GLOB opus_include thirdparties/common/src/opus-1.1/include/*.h thirdparties/common/src/opus-1.1/include/*.c thirdparties/common/src/opus-1.1/include/*.cpp)
SOURCE_GROUP("thirdparties\\common\\src\\opus-1.1\\include" FILES ${opus_include})

FILE (GLOB opus_silk_arm thirdparties/common/src/opus-1.1/silk/arm/*.h thirdparties/common/src/opus-1.1/silk/arm/*.c thirdparties/common/src/opus-1.1/silk/arm/*.cpp)
SOURCE_GROUP("thirdparties\\common\\src\\opus-1.1\\silk\\arm" FILES ${opus_silk_arm})

FILE (GLOB opus_silk_fixed thirdparties/common/src/opus-1.1/silk/fixed/*.h thirdparties/common/src/opus-1.1/silk/fixed/*.c thirdparties/common/src/opus-1.1/silk/fixed/*.cpp)
SOURCE_GROUP("thirdparties\\common\\src\\opus-1.1\\silk\\fixed" FILES ${opus_silk_fixed})

FILE (GLOB opus_silk thirdparties/common/src/opus-1.1/silk/*.h thirdparties/common/src/opus-1.1/silk/*.c thirdparties/common/src/opus-1.1/silk/*.cpp)
SOURCE_GROUP("thirdparties\\common\\src\\opus-1.1\\silk" FILES ${opus_silk})

FILE (GLOB opus_src thirdparties/common/src/opus-1.1/src/*.h thirdparties/common/src/opus-1.1/src/*.c thirdparties/common/src/opus-1.1/src/*.cpp)
SOURCE_GROUP("thirdparties\\common\\src\\opus-1.1\\src" FILES ${opus_src})

#webrtc modules
set (webrtc thirdparties/common/src/webrtc/common_types.cpp )
SOURCE_GROUP("thirdparties\\common\\src\\webrtc" FILES ${webrtc})

set (webrtc_base thirdparties/common/src/webrtc/base/checks.cpp 
                thirdparties/common/src/webrtc/base/logging.cpp 
				thirdparties/common/src/webrtc/base/bitbuffer.cpp
				thirdparties/common/src/webrtc/base/buffer.cpp)
SOURCE_GROUP("thirdparties\\common\\src\\webrtc\\src\\base" FILES ${webrtc_base})

set (webrtc_system_wrapers_source thirdparties/common/src/webrtc/system_wrappers/source/metrics_default.cpp 
                                thirdparties/common/src/webrtc/system_wrappers/source/critical_section_win.cpp 
                                thirdparties/common/src/webrtc/system_wrappers/source/critical_section.cpp
                                thirdparties/common/src/webrtc/system_wrappers/source/cpu_features.cpp
                                thirdparties/common/src/webrtc/system_wrappers/source/aligned_malloc.cpp
                                thirdparties/common/src/webrtc/system_wrappers/source/logging.cpp  )
SOURCE_GROUP("thirdparties\\common\\src\\webrtc\\src\\system_wrappers\\source" FILES ${webrtc_system_wrapers_source})

FILE (GLOB webrtc_system_wrapers_include thirdparties/common/src/webrtc/system_wrappers/include/*.h thirdparties/common/src/webrtc/system_wrappers/include/*.c thirdparties/common/src/webrtc/system_wrappers/include/*.cpp)
SOURCE_GROUP("thirdparties\\common\\src\\webrtc\\src\\system_wrappers\\include" FILES ${webrtc_system_wrapers_include})

set (webrtc_common_audio thirdparties/common/src/webrtc/common_audio/ring_buffer.cpp 
                        thirdparties/common/src/webrtc/common_audio/audio_util.cpp )
SOURCE_GROUP("thirdparties\\common\\src\\webrtc\\src\\common_audio" FILES ${webrtc_common_audio})

set (webrtc_common_audio_signal_processing thirdparties/common/src/webrtc/common_audio/signal_processing/spl_init.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/spl_sqrt.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/spl_sqrt_floor.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/splitting_filter.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/sqrt_of_one_minus_x_squared.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/vector_scaling_operations.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/levinson_durbin.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/lpc_to_refl_coef.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/min_max_operations.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/randomization_functions.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/real_fft.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/refl_coef_to_lpc.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/resample_48khz.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/resample_by_2_internal.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/resample_by_2.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/resample_fractional.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/resample.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/filter_ar_fast_q12.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/filter_ma_fast_q12.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/get_hanning_window.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/get_scaling_square.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/ilbc_specific_functions.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/division_operations.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/dot_product_with_scale.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/downsample_fast.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/energy.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/filter_ar.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/auto_corr_to_refl_coef.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/auto_correlation.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/complex_bit_reverse.c 
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/complex_fft.c
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/copy_set_operations.c
                                        thirdparties/common/src/webrtc/common_audio/signal_processing/cross_correlation.c )
SOURCE_GROUP("thirdparties\\common\\src\\webrtc\\src\\common_audio\\signal_processing" FILES ${webrtc_common_audio_signal_processing})

FILE (GLOB webrtc_common_audio_signal_processing_include thirdparties/common/src/webrtc/common_audio/signal_processing/include/*.h 
                                                    thirdparties/common/src/webrtc/signal_processing/include/*.c 
                                                    thirdparties/common/src/webrtc/signal_processing/include/*.cpp)
SOURCE_GROUP("thirdparties\\common\\src\\webrtc\\src\\common_audio\\signal_processing\\include" FILES ${webrtc_common_audio_signal_processing_include})

set (webrtc_common_audio_signal_resampler thirdparties/common/src/webrtc/common_audio/resampler/push_resampler.cpp
                                        thirdparties/common/src/webrtc/common_audio/resampler/push_sinc_resampler.cpp
                                        thirdparties/common/src/webrtc/common_audio/resampler/resampler_new.cpp
                                        thirdparties/common/src/webrtc/common_audio/resampler/sinc_resampler.cpp
                                        thirdparties/common/src/webrtc/common_audio/resampler/sinc_resampler_sse.cpp
                                        thirdparties/common/src/webrtc/common_audio/resampler/sinusoidal_linear_chirp_source.cpp)
SOURCE_GROUP("thirdparties\\common\\src\\webrtc\\src\\common_audio\\resampler" FILES ${webrtc_common_audio_signal_resampler})

FILE (GLOB webrtc_common_audio_signal_resampler_include thirdparties/common/src/webrtc/common_audio/resampler/include/*.h 
                                                    thirdparties/common/src/webrtc/common_audio/resampler/*.c 
                                                    thirdparties/common/src/webrtc/common_audio/resampler/*.cpp)
SOURCE_GROUP("thirdparties\\common\\src\\webrtc\\src\\common_audio\\resampler\\include" FILES ${webrtc_common_audio_signal_resampler_include})



set (webrtc_common_audio_vad thirdparties/common/src/webrtc/common_audio/vad/vad_core.cpp
                            thirdparties/common/src/webrtc/common_audio/vad/vad_filterbank.cpp
                            thirdparties/common/src/webrtc/common_audio/vad/vad_gmm.cpp
                            thirdparties/common/src/webrtc/common_audio/vad/vad_sp.cpp 
                            thirdparties/common/src/webrtc/common_audio/vad/webrtc_vad.cpp )
SOURCE_GROUP("thirdparties\\common\\src\\webrtc\\src\\common_audio\\vad" FILES ${webrtc_common_audio_vad})

FILE (GLOB webrtc_common_audio_vad_include thirdparties/common/src/webrtc/common_audio/vad/include/*.h 
                                        thirdparties/common/src/webrtc/vad/include/*.c 
                                        thirdparties/common/src/webrtc/vad/include/*.cpp)
SOURCE_GROUP("thirdparties\\common\\src\\webrtc\\src\\common_audio\\vad\\include" FILES ${webrtc_common_audio_vad_include})

set (webrtc_modules_audio_coding_codecs thirdparties/common/src/webrtc/modules/audio_coding/codecs/audio_decoder.cpp
                                        thirdparties/common/src/webrtc/modules/audio_coding/codecs/audio_encoder.cpp
                                        thirdparties/common/src/webrtc/modules/audio_coding/codecs/cng/webrtc_cng.cpp
                                        thirdparties/common/src/webrtc/modules/audio_coding/codecs/cng/cng_helpfuns.cpp
                                        thirdparties/common/src/webrtc/modules/audio_coding/codecs/opus/audio_decoder_opus.cpp
                                        thirdparties/common/src/webrtc/modules/audio_coding/codecs/opus/audio_encoder_opus.cpp
                                        thirdparties/common/src/webrtc/modules/audio_coding/codecs/opus/opus_interface.cpp)
SOURCE_GROUP("thirdparties\\common\\src\\webrtc\\src\\modules\\audio_coding\\codecs" FILES ${webrtc_modules_audio_coding_codecs})

set (webrtc_modules_audio_coding_main thirdparties/common/src/webrtc/modules/audio_coding/neteq/accelerate.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/audio_classifier.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/audio_decoder_impl.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/audio_multi_vector.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/audio_vector.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/background_noise.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/buffer_level_filter.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/comfort_noise.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/decision_logic.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/decision_logic_fax.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/decision_logic_game_voice.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/decision_logic_normal.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/decoder_database.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/delay_manager.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/delay_peak_detector.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/dsp_helper.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/dtmf_buffer.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/dtmf_tone_generator.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/expand.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/merge.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/nack.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/neteq.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/neteq_impl.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/normal.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/packet_buffer.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/payload_splitter.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/post_decode_vad.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/preemptive_expand.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/random_vector.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/rtcp.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/statistics_calculator.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/sync_buffer.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/time_stretch.cpp
                                    thirdparties/common/src/webrtc/modules/audio_coding/neteq/timestamp_scaler.cpp)
SOURCE_GROUP("thirdparties\\common\\src\\webrtc\\src\\modules\\audio_coding\\main" FILES ${webrtc_modules_audio_coding_main})

set (webrtc_modules_audio_processing thirdparties/common/src/webrtc/modules/audio_processing/aecm/aecm_core.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/aecm/aecm_core_c.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/aec/aec_core_sse2.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/aecm/echo_control_mobile.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/aec/aec_core.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/aec/aec_resampler.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/aec/echo_cancellation.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/aec_new/aec_core_sse2_new.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/aec_new/aec_core_new.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/aec_new/aec_rdft_new.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/aec_new/aec_rdft_sse2_new.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/aec_new/aec_resampler_new.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/aec_new/echo_cancellation_new.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/agc/histogram.cc
                                    thirdparties/common/src/webrtc/modules/audio_processing/agc/utility.cc
                                    thirdparties/common/src/webrtc/modules/audio_processing/agc/legacy/analog_agc.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/agc/legacy/digital_agc.cpp
                                    thirdparties/common/src/webrtc/modules/utility/source/audio_frame_operations.cc
                                    thirdparties/common/src/webrtc/modules/audio_processing/utility/delay_estimator_wrapper.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/utility/delay_estimator.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/utility/webrtc_new/delay_estimator_wrapper_new.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/utility/webrtc_new/delay_estimator_new.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/utility/block_mean_calculator.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/utility/ooura_fft.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/utility/ooura_fft_sse2.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/ns/noise_suppression.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/ns/noise_suppression_x.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/ns/ns_core.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/ns/nsx_core.cpp
                                    thirdparties/common/src/webrtc/modules/audio_processing/ns/nsx_core_c.cpp)
SOURCE_GROUP("thirdparties\\common\\src\\webrtc\\src\\modules\\audio_processing" FILES ${webrtc_modules_audio_processing})


set (webrtc_common_h264 thirdparties/common/src/webrtc/common_video/h264/h264_common.cc
                                        thirdparties/common/src/webrtc/common_video/h264/sps_parser.cc)
SOURCE_GROUP("thirdparties\\common\\src\\webrtc\\src\\common_video\\h264" FILES ${webrtc_common_h264})

#bindings common
FILE (GLOB bindings_common bindings/_common/*.h  bindings/_common/*.c bindings/_common/*.cpp bindings/_common/*.cxx)
SOURCE_GROUP("bindings\\_common" FILES ${bindings_common})

#bindings classes
FILE (GLOB_RECURSE bindings_cocos2d_classes bindings/cocos2d-x/classes/*.h  bindings/cocos2d-x/classes/*.cpp bindings/cocos2d-x/classes/*.hpp)
SOURCE_GROUP("bindings\\cocos2d-x\\classes" FILES ${bindings_cocos2d_classes})

#bindings interface
FILE (GLOB_RECURSE bindings_cocos2d_interface bindings/cocos2d-x/interface/*.h  bindings/cocos2d-x/interface/*.cpp bindings/cocos2d-x/interface/*.hpp)
SOURCE_GROUP("bindings\\cocos2d-x\\interface" FILES ${bindings_cocos2d_interface})

#bindings platform module
IF(WIN32)
set (bindings_cocos2d_win bindings/cocos2d-x/win/CameraCaptureDShow.cpp
                        bindings/cocos2d-x/win/CameraManager.cpp
                        bindings/cocos2d-x/win/SDLDisplay.cpp 
                        bindings/cocos2d-x/win/IYouMeVideoRenderEngine.cpp 
                        bindings/cocos2d-x/win/VideoRenderMgr.cpp 
                        bindings/cocos2d-x/win/VideoRenderWnd.cpp 
bindings/cocos2d-x/win/help_functions_dshow.cc
bindings/cocos2d-x/win/sink_filter_dshow.cc
bindings/cocos2d-x/win/AliTranscriberWin.cpp
bindings/cocos2d-x/win/AudioDeviceMgr_win.cpp
)
#bindings win
#FILE (GLOB_RECURSE bindings_cocos2d_win bindings/cocos2d-x/win/*.h  bindings/cocos2d-x/win/*.cpp bindings/cocos2d-x/win/*.hpp)
SOURCE_GROUP("bindings\\cocos2d-x\\win" FILES ${bindings_cocos2d_win})

ELSEIF (__LINUX__)
set (bindings_cocos2d_linux bindings/cocos2d-x/linux/CameraManager.cpp)

ENDIF()

#ffmpeg module
FILE (GLOB_RECURSE ffmpegPlayer_player ffmpegPlayer/player/*.h  ffmpegPlayer/player/*.cpp ffmpegPlayer/player/*.hpp)
SOURCE_GROUP("ffmpegPlayer\\player" FILES ${ffmpegPlayer_player})

#tinysak
FILE (GLOB tinySAK tinySAK/src/*.h  tinySAK/src/*.c tinySAK/src/*.cpp)
SOURCE_GROUP("tinySAK\\src" FILES ${tinySAK})

#tinynet
set (tinyNET tinyNET/src/tnet_utils.c 
            tinyNET/src/tnet_transport_win32.c
            tinyNET/src/tnet_transport.c
            tinyNET/src/tnet_socket.c
            tinyNET/src/tnet_poll.c
            tinyNET/src/tnet_endianness.c
            tinyNET/src/tnet.c)
SOURCE_GROUP("tinyNET\\src" FILES ${tinyNET} )

#tinyMEDIA
FILE (GLOB tinyMEDIA tinyMEDIA/src/*.h  tinyMEDIA/src/*.c tinyMEDIA/src/*.cpp)
SOURCE_GROUP("tinyMEDIA\\src" FILES ${tinyMEDIA})

FILE (GLOB tinyMEDIA_Content tinyMEDIA/src/content/*.h  tinyMEDIA/src/content/*.c tinyMEDIA/src/content/*.cpp)
SOURCE_GROUP("tinyMEDIA\\src\\content" FILES ${tinyMEDIA_Content})

#tinySDP
FILE (GLOB tinySDP tinySDP/src/*.h  tinySDP/src/*.c tinySDP/src/*.cpp)
SOURCE_GROUP("tinySDP\\src" FILES ${tinySDP})

FILE (GLOB tinySDP_parsers tinySDP/src/parsers/.h  tinySDP/src/parsers/*.c tinySDP/src/parsers/*.cpp)
SOURCE_GROUP("tinySDP\\src\\parsers" FILES ${tinySDP_parsers})

FILE (GLOB tinySDP_headers tinySDP/src/headers/.h  tinySDP/src/headers/*.c tinySDP/src/headers/*.cpp)
SOURCE_GROUP("tinySDP\\src\\headers" FILES ${tinySDP_headers})

#tinyRTP
FILE (GLOB tinyRTP tinyRTP/src/*.h  tinyRTP/src/*.c tinyRTP/src/*.cpp)
SOURCE_GROUP("tinyRTP\\src" FILES ${tinyRTP})

FILE (GLOB tinyRTP_rtp tinyRTP/src/rtp/*.h  tinyRTP/src/rtp/*.c tinyRTP/src/rtp/*.cpp)
SOURCE_GROUP("tinyRTP\\src\\rtp" FILES ${tinyRTP_rtp})

FILE (GLOB tinyRTP_rtcp tinyRTP/src/rtcp/*.h  tinyRTP/src/rtcp/*.c tinyRTP/src/rtcp/*.cpp)
SOURCE_GROUP("tinyRTP\\src\\rtcp" FILES ${tinyRTP_rtcp})

#tinyDAV
FILE (GLOB tinyDAV tinyDAV/src/*.h  tinyDAV/src/*.c tinyDAV/src/*.cpp)
SOURCE_GROUP("tinyDAV\\src" FILES ${tinyDAV})

set (tinyDAV_codecs tinyDAV/src/codecs/bandwidth_ctrl/tdav_codec_bandwidth_ctrl.cpp 
                    tinyDAV/src/codecs/mixer/tdav_codec_audio_mixer.cpp
                    tinyDAV/src/codecs/mixer/tdav_codec_audio_resample.cpp
                    tinyDAV/src/codecs/opus/tdav_codec_opus.cpp
                    tinyDAV/src/codecs/mixer/tdav_codec_audio_howling_supp.cpp
                    tinyDAV/src/codecs/mixer/tdav_codec_audio_biquad_filter.cpp
                    tinyDAV/src/codecs/mixer/tdav_codec_audio_kiss_fft.cpp
                    tinyDAV/src/codecs/mixer/tdav_codec_audio_kiss_fftr.cpp
                    tinyDAV/src/codecs/mixer/tdav_codec_audio_fftwrap.cpp
                    tinyDAV/src/codecs/h264/tdav_codec_h264.cpp
                    tinyDAV/src/codecs/h264/tdav_codec_h264_rtp.cpp
                    tinyDAV/src/video/tdav_consumer_video.c
                    tinyDAV/src/video/tdav_producer_video.c
                    tinyDAV/src/video/tdav_runnable_video.c
                    tinyDAV/src/video/tdav_session_video.cpp
                    tinyDAV/src/video/jb/tdav_video_frame.c
                    tinyDAV/src/video/jb/tdav_video_jb.cpp
                    tinyDAV/src/video/jb/youme_video_jitter.cpp
                    tinyDAV/src/video/common/video.cpp
                    tinyDAV/src/video/common/video_consumer.cpp
                    tinyDAV/src/video/common/video_device_impl.cpp
                    tinyDAV/src/video/common/video_producer.cpp
                    tinyDAV/src/tdav_win32.c
                    tinyDAV/src/tdav_session_av.c
                    tinyDAV/src/tdav.c
                    tinyDAV/src/common/tdav_rscode.c
                    tinyDAV/src/common/tdav_audio_rscode.c
                    tinyDAV/src/codecs/rtp_extension/tdav_codec_rtp_extension.cpp
                    )
SOURCE_GROUP("tinyDAV\\src\\codecs" FILES ${tinyDAV_codecs} )
IF(__LINUX__)
list(REMOVE_ITEM tinyDAV_codecs tinyDAV/src/tdav_win32.c)
ENDIF()

FILE (GLOB tinyDAV_audio tinyDAV/src/audio/*.h  tinyDAV/src/audio/*.c tinyDAV/src/audio/*.cpp)
SOURCE_GROUP("tinyDAV\\src\\audio" FILES ${tinyDAV_audio})


FILE (GLOB tinyDAV_audio_win32 tinyDAV/src/audio/waveapi/*.h  tinyDAV/src/audio/waveapi/*.c tinyDAV/src/audio/waveapi/*.cpp tinyDAV/src/audio/dmoapi/*.h  tinyDAV/src/audio/dmoapi/*.c tinyDAV/src/audio/dmoapi/*.cpp)
SOURCE_GROUP("tinyDAV\\src\\audio\\win32" FILES ${tinyDAV_audio_win32})

FILE (GLOB tinyDAV_audio_alsa tinyDAV/src/audio/alsa/*.c tinyDAV/src/audio/alsa/*.cpp)
SOURCE_GROUP("tinyDAV\\src\\audio\\alsa" FILES ${tinyDAV_audio_alsa})

#zlib
FILE (GLOB zlib thirdparties/common/src/zlib/*.h thirdparties/common/src/zlib/*.cpp thirdparties/common/src/zlib/*.c)
SOURCE_GROUP("thirdparties\\common\\src\\zlib" FILES ${zlib})

#Add the appropriate files for different platforms
IF(__LINUX__)
list(REMOVE_ITEM webrtc_system_wrapers_source thirdparties/common/src/webrtc/system_wrappers/source/critical_section_win.cpp)
list(APPEND webrtc_system_wrapers_source thirdparties/common/src/webrtc/system_wrappers/source/critical_section_posix.cpp)

list(REMOVE_ITEM tinyNET tinyNET/src/tnet_transport_win32.c)
list(APPEND tinyNET tinyNET/src/tnet_transport_poll.c)

IF(${ABI_INFO} STREQUAL "mips")
list(REMOVE_ITEM webrtc_common_audio_signal_resampler thirdparties/common/src/webrtc/common_audio/resampler/sinc_resampler_sse.cpp)
list(REMOVE_ITEM webrtc_modules_audio_processing thirdparties/common/src/webrtc/modules/audio_processing/aec/aec_core_sse2.cpp
                                                 thirdparties/common/src/webrtc/modules/audio_processing/aec_new/aec_core_sse2_new.cpp
                                                 thirdparties/common/src/webrtc/modules/audio_processing/aec_new/aec_rdft_sse2_new.cpp
                                                 thirdparties/common/src/webrtc/modules/audio_processing/utility/ooura_fft_sse2.cpp)
ENDIF()
ENDIF()

#extern library path
IF (WIN_X86_32_2013MT)
IF (CMAKE_CONFIGURATION_TYPES MATCHES "Release")
	LINK_DIRECTORIES(${COMMON_LIB}/../youme_voice_engine/youme_common/lib/windows
	${COMMON_LIB}/../youme_voice_engine/ffmpegPlayer/libs/windows/x86/lib
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/DShow/x86/
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/DShow/x86/Release 
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/SDL2/x86/lib
	${COMMON_LIB}/../youme_voice_engine/youme_common/thirdlib/libscreen/x86
	)
ELSEIF (CMAKE_CONFIGURATION_TYPES MATCHES "Debug")
	LINK_DIRECTORIES(${COMMON_LIB}/../youme_voice_engine/youme_common/lib/windows
	${COMMON_LIB}/../youme_voice_engine/ffmpegPlayer/libs/windows/x86/lib
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/DShow/x86
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/DShow/x86/Debug 
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/SDL2/x86/lib
	${COMMON_LIB}/../youme_voice_engine/youme_common/thirdlib/libscreen/x86
	)
ENDIF()
ELSEIF (WIN_X86_64_2013MT)
IF (CMAKE_CONFIGURATION_TYPES MATCHES "Release")
	LINK_DIRECTORIES(${COMMON_LIB}/../youme_voice_engine/youme_common/lib/x86_64_2013
	${COMMON_LIB}/../youme_voice_engine/ffmpegPlayer/libs/windows/x86_64/lib 
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/DShow/x64/
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/DShow/x64/Release	
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/SDL2/x64/lib
	${COMMON_LIB}/../youme_voice_engine/youme_common/thirdlib/libscreen/x64
    )
ELSEIF (CMAKE_CONFIGURATION_TYPES MATCHES "Debug")
	LINK_DIRECTORIES(${COMMON_LIB}/../youme_voice_engine/youme_common/lib/x86_64_2013
	${COMMON_LIB}/../youme_voice_engine/ffmpegPlayer/libs/windows/x86_64/lib 
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/DShow/x64/
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/DShow/x64/Debug	
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/SDL2/x64/lib
	${COMMON_LIB}/../youme_voice_engine/youme_common/thirdlib/libscreen/x64
    )
ENDIF()	
	ADD_DEFINITIONS(-DPOINTER_32=_ptr32)
ELSEIF (WIN_X86_64_2015MD)
IF (CMAKE_CONFIGURATION_TYPES MATCHES "Release")
	LINK_DIRECTORIES(${COMMON_LIB}/../youme_voice_engine/youme_common/lib/x86_64
	${COMMON_LIB}/../youme_voice_engine/ffmpegPlayer/libs/windows/x86_64/lib 
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/DShow/x64/ 
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/DShow/x64/Release
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/SDL2/x64/lib
	${COMMON_LIB}/../youme_voice_engine/youme_common/thirdlib/libscreen/x64
    )
ELSEIF (CMAKE_CONFIGURATION_TYPES MATCHES "Debug")
	LINK_DIRECTORIES(${COMMON_LIB}/../youme_voice_engine/youme_common/lib/x86_64
	${COMMON_LIB}/../youme_voice_engine/ffmpegPlayer/libs/windows/x86_64/lib 
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/DShow/x64/ 
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/DShow/x64/Debug
	${COMMON_LIB}/../youme_voice_engine/bindings/cocos2d-x/win/SDL2/x64/lib
	${COMMON_LIB}/../youme_voice_engine/youme_common/thirdlib/libscreen/x64
    )
ENDIF()
	ADD_DEFINITIONS(-DPOINTER_32=_ptr32)
ELSEIF (__LINUX__)
    LINK_DIRECTORIES(${COMMON_LIB}/../youme_voice_engine/youme_common/lib/linux/${ABI_INFO}
                    ${COMMON_LIB}/../youme_voice_engine/ffmpegPlayer/libs/linux/lib)
ENDIF(WIN_X86_32_2013MT)

IF(TRANSSCRIBER)
LINK_DIRECTORIES(
    ${COMMON_LIB}/../youme_voice_engine/youme_common/thirdlib/libNls/windows/14.0/x64/Debug
    ${COMMON_LIB}/../youme_voice_engine/youme_common/thirdlib/libNls/windows/14.0/x64/pthread
    )
ENDIF()

#${protobuf} ${baseWrapper} ${ne10_dsp} ${ne10_inc} ${soundtouch_include} ${soundtouch_SoundStretch} ${soundtouch_SoundTouch} ${opus_celt_arm} ${opus_celt} ${opus_include} ${opus_silk_arm} ${opus_silk_fixed} ${opus_silk} ${opus_src} ${webrtc} ${webrtc_base} ${webrtc_system_wrapers_source} ${webrtc_system_wrapers_include} ${webrtc_common_audio_vad} ${webrtc_common_audio_vad_include} ${webrtc_common_audio_signal_processing_include} ${webrtc_common_audio_signal_processing} ${webrtc_common_audio} ${webrtc_common_audio_signal_resampler}

IF(WIN32)
IF (CMAKE_CONFIGURATION_TYPES MATCHES "Release")
	SET(CMAKE_SHARED_LINKER_FLAGS /NODEFAULTLIB:"MSVCRT.lib")
ENDIF()

	ADD_LIBRARY(youme_voice_engine SHARED ${header_files} ${source_files} ${zlib} ${tinySAK} ${tinyNET} ${tinySDP} ${tinySDP_parsers} ${tinySDP_headers} ${tinyMEDIA} ${tinyMEDIA_Content} ${tinyRTP} ${tinyRTP_rtp} ${tinyRTP_rtcp} ${baseWrapper} ${tinyDAV} ${tinyDAV_codecs} ${tinyDAV_audio} ${tinyDAV_audio_win32}  ${soundtouch_SoundTouch}  ${soundtouch_SoundStretch} ${opus_celt} ${opus_silk} ${opus_silk_fixed} ${opus_src} ${webrtc} ${webrtc_base} ${webrtc_system_wrapers_source} ${webrtc_common_audio} ${webrtc_common_audio_signal_processing} ${webrtc_common_audio_vad} ${webrtc_modules_audio_coding_codecs} ${webrtc_modules_audio_coding_main} ${webrtc_modules_audio_processing} ${webrtc_common_h264} ${bindings_common} ${bindings_cocos2d_classes} ${bindings_cocos2d_interface} ${protobuf} ${webrtc_common_audio_signal_resampler} ${bindings_cocos2d_win} ${ffmpegPlayer_player} )
	
	TARGET_LINK_LIBRARIES (youme_voice_engine ${YOUME_COMMON_LIB_NAME} wldap32.lib  Version.lib Setupapi.lib ws2_32 Winmm.lib Iphlpapi.lib Shlwapi.lib Winmm.lib Msdmo.lib dmoguids.lib uuid.lib amstrmid.lib wmcodecdspuuid.lib avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib swresample.lib swscale.lib libx264.lib SDL2.lib SDL2main.lib strmbase.lib libscreen.lib libjpeg.lib)
    IF(TRANSSCRIBER)
        TARGET_LINK_LIBRARIES (youme_voice_engine nlsCommonSdk.lib nlsCppOpu.lib nlsCppSdk.lib jsoncpp-0.y.z.lib  libeay32.lib opus.lib ssleay32.lib)
        IF($ABI_INFO STREQUAL "x86")
        TARGET_LINK_LIBRARIES (youme_voice_engine pthreadVC2.lib pthreadVCE2.lib pthreadVSE2.lib )
        ELSE()
        TARGET_LINK_LIBRARIES (youme_voice_engine pthreadVC2.lib )
        ENDIF()
        IF (CMAKE_CONFIGURATION_TYPES MATCHES "Release")
            SET(CMAKE_SHARED_LINKER_FLAGS /NODEFAULTLIB:"MSVCRT.lib")
            TARGET_LINK_LIBRARIES(youme_voice_engine libcurl.lib)
        ELSE ()
            TARGET_LINK_LIBRARIES(youme_voice_engine libcurld.lib)
        ENDIF()
    ENDIF()


ELSEIF (__ANDROID__) 
	ADD_LIBRARY(youme_voice_engine SHARED ${header_files} ${source_files} ${protobuf} ${baseWrapper} ${Android}  ${ne10_dsp} ${ne10_inc} ${soundtouch_include} ${soundtouch_SoundStretch} ${soundtouch_SoundTouch} ${opus_celt_arm} ${opus_celt} ${opus_include} ${opus_silk_arm} ${opus_silk_fixed} ${opus_silk} ${opus_src} ${webrtc} ${webrtc_base} ${webrtc_system_wrapers_source} ${webrtc_system_wrapers_include} ${webrtc_common_audio_vad} ${webrtc_common_audio_vad_include} ${webrtc_common_audio_signal_processing_include} ${webrtc_common_audio_signal_processing} ${webrtc_common_audio} ${tinySAK})
	
    TARGET_LINK_LIBRARIES (youme_voice_engine ${YOUME_COMMON_LIB_NAME} z log)
ELSEIF (__LINUX__) 
     ADD_LIBRARY(youme_voice_engine SHARED ${header_files} ${source_files} ${zlib} ${tinySAK} ${tinyNET} ${tinySDP} ${tinySDP_parsers} ${tinySDP_headers} ${tinyMEDIA} ${tinyMEDIA_Content} ${tinyRTP} ${tinyRTP_rtp} ${tinyRTP_rtcp} ${baseWrapper} ${tinyDAV} ${tinyDAV_codecs} ${tinyDAV_audio} ${tinyDAV_audio_alsa} ${soundtouch_SoundTouch}  ${soundtouch_SoundStretch} ${opus_celt} ${opus_silk} ${opus_silk_fixed} ${opus_src} ${webrtc} ${webrtc_base} ${webrtc_system_wrapers_source} ${webrtc_common_audio} ${webrtc_common_audio_signal_processing} ${webrtc_common_audio_vad} ${webrtc_modules_audio_coding_codecs} ${webrtc_modules_audio_coding_main} ${webrtc_modules_audio_processing} ${webrtc_common_h264} ${bindings_common} ${bindings_cocos2d_classes} ${bindings_cocos2d_interface} ${protobuf} ${webrtc_common_audio_signal_resampler} ${bindings_cocos2d_linux} )
	
    IF($ABI_INFO STREQUAL "x86")
    TARGET_LINK_LIBRARIES (youme_voice_engine ${YOUME_COMMON_LIB_NAME} z pthread dl)
    ELSE()
    TARGET_LINK_LIBRARIES (youme_voice_engine ${YOUME_COMMON_LIB_NAME} pthread dl)
    ENDIF()
    
    #FILE (GLOB linux_demo ../SDKTest/linux/*.cpp ../SDKTest/linux/*.h)
    #ADD_EXECUTABLE(youme_test ${linux_demo} ${header_files} ${source_files} ${zlib} ${tinySAK} ${tinyNET} ${tinySDP} ${tinySDP_parsers} ${tinySDP_headers} ${tinyMEDIA} ${tinyMEDIA_Content} ${tinyRTP} ${tinyRTP_rtp} ${tinyRTP_rtcp} ${baseWrapper} ${tinyDAV} ${tinyDAV_codecs} ${tinyDAV_audio} ${soundtouch_SoundTouch}  ${soundtouch_SoundStretch} ${opus_celt} ${opus_silk} ${opus_silk_fixed} ${opus_src} ${webrtc} ${webrtc_base} ${webrtc_system_wrapers_source} ${webrtc_common_audio} ${webrtc_common_audio_signal_processing} ${webrtc_common_audio_vad} ${webrtc_modules_audio_coding_codecs} ${webrtc_modules_audio_coding_main} ${webrtc_modules_audio_processing} ${bindings_common} ${bindings_cocos2d_classes} ${bindings_cocos2d_interface} ${protobuf} ${webrtc_common_audio_signal_resampler} ${bindings_cocos2d_linux} )


ELSEIF (__IOS__)
	ADD_LIBRARY (youme_voice_engine ${header_files} ${source_files} ${protobuf} ${baseWrapper}  ${ne10_dsp} ${ne10_inc}  ${soundtouch_include} ${soundtouch_SoundStretch} ${soundtouch_SoundTouch} ${opus_celt_arm} ${opus_celt} ${opus_include} ${opus_silk_arm} ${opus_silk_fixed} ${opus_silk} ${opus_src} ${webrtc} ${webrtc_base} ${webrtc_system_wrapers_source} ${webrtc_system_wrapers_include} ${webrtc_common_audio_vad} ${webrtc_common_audio_vad_include} ${webrtc_common_audio_signal_processing_include} ${webrtc_common_audio_signal_processing} ${webrtc_common_audio} ${tinySAK})
ELSEIF (__IOSSIMULATOR__)
	ADD_LIBRARY (youme_voice_engine ${header_files} ${source_files} ${protobuf} ${baseWrapper}  ${ne10_dsp} ${ne10_inc}  ${soundtouch_include} ${soundtouch_SoundStretch} ${soundtouch_SoundTouch} ${opus_celt_arm} ${opus_celt} ${opus_include} ${opus_silk_arm} ${opus_silk_fixed} ${opus_silk} ${opus_src} ${webrtc} ${webrtc_base} ${webrtc_system_wrapers_source} ${webrtc_system_wrapers_include} ${webrtc_common_audio_vad} ${webrtc_common_audio_vad_include} ${webrtc_common_audio_signal_processing_include} ${webrtc_common_audio_signal_processing} ${webrtc_common_audio} ${tinySAK})
ELSEIF (__OSX__)	
	ADD_LIBRARY (youme_voice_engine ${header_files} ${source_files} ${protobuf} ${baseWrapper}  ${ne10_dsp} ${ne10_inc}  ${soundtouch_include} ${soundtouch_SoundStretch} ${soundtouch_SoundTouch} ${opus_celt_arm} ${opus_celt} ${opus_include} ${opus_silk_arm} ${opus_silk_fixed} ${opus_silk} ${opus_src} ${webrtc} ${webrtc_base} ${webrtc_system_wrapers_source} ${webrtc_system_wrapers_include} ${webrtc_common_audio_vad} ${webrtc_common_audio_vad_include} ${webrtc_common_audio_signal_processing_include} ${webrtc_common_audio_signal_processing} ${webrtc_common_audio} ${tinySAK})
ENDIF(WIN32)


