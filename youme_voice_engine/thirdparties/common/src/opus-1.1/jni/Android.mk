LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
ifeq ($(NEON),1)
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += -DHAVE_NEON
LOCAL_CPPFLAGS += -DHAVE_NEON
endif
endif

LOCAL_MODULE := opus1.1

LOCAL_CFLAGS +=\
-DHAVE_CONFIG_H -DANDROID\
-Os \
-fvisibility=hidden\
-ffunction-sections -fdata-sections\
-DOPUS_BUILD=1 -DVAR_ARRAYS=1 -DHAVE_LRINTF=1 -DFIXED_POINT=1

LOCAL_CPPFLAGS +=\
-DHAVE_CONFIG_H -DANDROID\
-Os \
-fvisibility=hidden\
-fvisibility-inlines-hidden\
-ffunction-sections -fdata-sections\
-DOPUS_BUILD=1 -DVAR_ARRAYS=1 -DHAVE_LRINTF=1 -DFIXED_POINT=1



LOCAL_SRC_FILES := \
../celt/celt.cpp \
../celt/bands.cpp \
../celt/celt_decoder.cpp \
../celt/celt_encoder.cpp \
../celt/celt_lpc.cpp \
../celt/cwrs.cpp \
../celt/entcode.cpp \
../celt/entdec.cpp \
../celt/entenc.cpp \
../celt/kiss_fft.cpp \
../celt/laplace.cpp \
../celt/mathops.cpp \
../celt/mdct.cpp \
../celt/modes.cpp \
../celt/pitch.cpp \
../celt/quant_bands.cpp \
../celt/rate.cpp \
../celt/vq.cpp \
../silk/A2NLSF.cpp \
../silk/ana_filt_bank_1.cpp \
../silk/biquad_alt.cpp \
../silk/bwexpander.cpp \
../silk/bwexpander_32.cpp \
../silk/check_control_input.cpp \
../silk/CNG.cpp \
../silk/code_signs.cpp \
../silk/control_audio_bandwidth.cpp \
../silk/control_codec.cpp \
../silk/control_SNR.cpp \
../silk/debug.cpp \
../silk/dec_API.cpp \
../silk/decode_core.cpp \
../silk/decode_frame.cpp \
../silk/decode_indices.cpp \
../silk/decode_parameters.cpp \
../silk/decode_pitch.cpp \
../silk/decode_pulses.cpp \
../silk/decoder_set_fs.cpp \
../silk/enc_API.cpp \
../silk/encode_indices.cpp \
../silk/encode_pulses.cpp \
../silk/gain_quant.cpp \
../silk/HP_variable_cutoff.cpp \
../silk/init_decoder.cpp \
../silk/init_encoder.cpp \
../silk/inner_prod_aligned.cpp \
../silk/interpolate.cpp \
../silk/lin2log.cpp \
../silk/log2lin.cpp \
../silk/LP_variable_cutoff.cpp \
../silk/LPC_analysis_filter.cpp \
../silk/LPC_inv_pred_gain.cpp \
../silk/LPC_fit.cpp \
../silk/NLSF2A.cpp \
../silk/NLSF_decode.cpp \
../silk/NLSF_del_dec_quant.cpp \
../silk/NLSF_encode.cpp \
../silk/NLSF_stabilize.cpp \
../silk/NLSF_unpack.cpp \
../silk/NLSF_VQ.cpp \
../silk/NLSF_VQ_weights_laroia.cpp \
../silk/NSQ.cpp \
../silk/NSQ_del_dec.cpp \
../silk/pitch_est_tables.cpp \
../silk/PLC.cpp \
../silk/process_NLSFs.cpp \
../silk/quant_LTP_gains.cpp \
../silk/resampler.cpp \
../silk/resampler_down2.cpp \
../silk/resampler_down2_3.cpp \
../silk/resampler_private_AR2.cpp \
../silk/resampler_private_down_FIR.cpp \
../silk/resampler_private_IIR_FIR.cpp \
../silk/resampler_private_up2_HQ.cpp \
../silk/resampler_rom.cpp \
../silk/shell_coder.cpp \
../silk/sigm_Q15.cpp \
../silk/sort.cpp \
../silk/stereo_decode_pred.cpp \
../silk/stereo_encode_pred.cpp \
../silk/stereo_find_predictor.cpp \
../silk/stereo_LR_to_MS.cpp \
../silk/stereo_MS_to_LR.cpp \
../silk/stereo_quant_pred.cpp \
../silk/sum_sqr_shift.cpp \
../silk/table_LSF_cos.cpp \
../silk/tables_gain.cpp \
../silk/tables_LTP.cpp \
../silk/tables_NLSF_CB_NB_MB.cpp \
../silk/tables_NLSF_CB_WB.cpp \
../silk/tables_other.cpp \
../silk/tables_pitch_lag.cpp \
../silk/tables_pulses_per_block.cpp \
../silk/VAD.cpp \
../silk/VQ_WMat_EC.cpp \
../silk/fixed/apply_sine_window_FIX.cpp \
../silk/fixed/autocorr_FIX.cpp \
../silk/fixed/burg_modified_FIX.cpp \
../silk/fixed/corrMatrix_FIX.cpp \
../silk/fixed/encode_frame_FIX.cpp \
../silk/fixed/find_LPC_FIX.cpp \
../silk/fixed/find_LTP_FIX.cpp \
../silk/fixed/find_pitch_lags_FIX.cpp \
../silk/fixed/find_pred_coefs_FIX.cpp \
../silk/fixed/k2a_FIX.cpp \
../silk/fixed/k2a_Q16_FIX.cpp \
../silk/fixed/LTP_analysis_filter_FIX.cpp \
../silk/fixed/LTP_scale_ctrl_FIX.cpp \
../silk/fixed/noise_shape_analysis_FIX.cpp \
../silk/fixed/pitch_analysis_core_FIX.cpp \
../silk/fixed/process_gains_FIX.cpp \
../silk/fixed/regularize_correlations_FIX.cpp \
../silk/fixed/residual_energy16_FIX.cpp \
../silk/fixed/residual_energy_FIX.cpp \
../silk/fixed/schur64_FIX.cpp \
../silk/fixed/schur_FIX.cpp \
../silk/fixed/vector_ops_FIX.cpp \
../silk/fixed/warped_autocorrelation_FIX.cpp \
../src/opus.cpp \
../src/opus_compare.cpp \
../src/opus_decoder.cpp \
../src/opus_encoder.cpp \
../src/opus_multistream.cpp \
../src/opus_multistream_decoder.cpp \
../src/opus_multistream_encoder.cpp \
../src/repacketizer.cpp \
../src/mapping_matrix.cpp \
../src/opus_projection_encoder.cpp \
../src/opus_projection_decoder.cpp  \
../src/analysis.cpp \
../src/mlp.cpp \
../src/mlp_data.cpp


LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include\
$(LOCAL_PATH)/../celt\
$(LOCAL_PATH)/../silk\
$(LOCAL_PATH)/../silk/fixed\
$(LOCAL_PATH)/../../../../..\
$(LOCAL_PATH)/../../Ne10/inc\
$(LOCAL_PATH)/../../../../../baseWrapper/src\
$(LOCAL_PATH)/..\



ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += \
-DFIXED_POINT=1 \
-DOPUS_HAVE_RTCD=1 \
-DOPUS_ARM_ASM=1 \
-DOPUS_ARM_INLINE_EDSP=1 \
-DOPUS_ARM_MAY_HAVE_NEON=1 \
-DCPU_ARM=1 \
-DOPUS_ARM_NEON_INTR=1 \
-DHAVE_ARM_NE10=1 \
-DOPUS_ARM_EXTERNAL_ASM=1 \
-DOPUS_ARM_MAY_HAVE_NEON_INTR=1

LOCAL_CPPFLAGS += \
-DFIXED_POINT=1 \
-DOPUS_HAVE_RTCD=1 \
-DOPUS_ARM_ASM=1 \
-DOPUS_ARM_INLINE_EDSP=1 \
-DOPUS_ARM_MAY_HAVE_NEON=1 \
-DCPU_ARM=1 \
-DOPUS_ARM_NEON_INTR=1 \
-DHAVE_ARM_NE10=1 \
-DOPUS_ARM_EXTERNAL_ASM=1 \
-DOPUS_ARM_MAY_HAVE_NEON_INTR=1

LOCAL_SRC_FILES += \
../celt/arm/arm_celt_map.cpp \
../celt/arm/armcpu.cpp \
../celt/arm/celt_fft_ne10.cpp \
../celt/arm/celt_mdct_ne10.cpp \
../celt/arm/pitch_neon_intr.cpp \
../celt/arm/celt_neon_intr.cpp \
../silk/arm/arm_silk_map.cpp \
../silk/arm/biquad_alt_neon_intr.cpp \
../silk/arm/LPC_inv_pred_gain_neon_intr.cpp \
../silk/arm/NSQ_del_dec_neon_intr.cpp \
../silk/arm/NSQ_neon.cpp \
../celt/arm/celt_pitch_xcorr_arm-gnu.S \
../silk/fixed/arm/warped_autocorrelation_FIX_neon_intr.cpp

else
LOCAL_SRC_FILES += \

endif


include $(BUILD_STATIC_LIBRARY)


