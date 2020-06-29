LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := ne10


LOCAL_C_INCLUDES :=     $(LOCAL_PATH)/../../common/ \
                        $(LOCAL_PATH)/../../inc \
                        $(LOCAL_PATH)/../../modules/math

ne10_neon_source := \
    $(LOCAL_PATH)/../../modules/math/NE10_abs.neon.s \
    $(LOCAL_PATH)/../../modules/math/NE10_addc.neon.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_addmat.neon.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_add.neon.s \
    $(LOCAL_PATH)/../../modules/math/NE10_cross.neon.s \
    $(LOCAL_PATH)/../../modules/math/NE10_detmat.neon.s \
    $(LOCAL_PATH)/../../modules/math/NE10_divc.neon.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_div.neon.s \
    $(LOCAL_PATH)/../../modules/math/NE10_dot.neon.s \
    $(LOCAL_PATH)/../../modules/math/NE10_identitymat.neon.s \
    $(LOCAL_PATH)/../../modules/math/NE10_invmat.neon.s \
    $(LOCAL_PATH)/../../modules/math/NE10_len.neon.s \
    $(LOCAL_PATH)/../../modules/math/NE10_mla.neon.s \
    $(LOCAL_PATH)/../../modules/math/NE10_mlac.neon.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_mulcmatvec.neon.s \
    $(LOCAL_PATH)/../../modules/math/NE10_mulc.neon.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_mulmat.neon.s \
    $(LOCAL_PATH)/../../modules/math/NE10_mul.neon.s \
    $(LOCAL_PATH)/../../modules/math/NE10_normalize.neon.s \
    $(LOCAL_PATH)/../../modules/math/NE10_rsbc.neon.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_setc.neon.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_subc.neon.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_submat.neon.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_sub.neon.s \
    $(LOCAL_PATH)/../../modules/math/NE10_transmat.neon.s \
    $(LOCAL_PATH)/../../common/NE10_mask_table.cpp \
    $(LOCAL_PATH)/../../modules/dsp/NE10_fft.cpp \
    $(LOCAL_PATH)/../../modules/dsp/NE10_fft_generic_float32.cpp \
    $(LOCAL_PATH)/../../modules/dsp/NE10_fft_generic_int32.cpp \
    $(LOCAL_PATH)/../../modules/dsp/NE10_rfft_float32.cpp \
    $(LOCAL_PATH)/../../modules/dsp/NE10_fft_float32.cpp \
    $(LOCAL_PATH)/../../modules/dsp/NE10_fft_int32.cpp \
    $(LOCAL_PATH)/../../modules/dsp/NE10_fft_int32.neonintrinsic.cpp \
    $(LOCAL_PATH)/../../modules/dsp/NE10_fft_generic_int32.neonintrinsic.cpp \


ne10_source_files := \
    $(LOCAL_PATH)/../../modules/math/NE10_abs.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_addc.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_addmat.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_add.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_cross.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_detmat.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_divc.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_div.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_dot.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_identitymat.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_invmat.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_len.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_mla.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_mlac.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_mulcmatvec.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_mulc.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_mulmat.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_mul.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_normalize.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_rsbc.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_setc.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_subc.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_submat.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_sub.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_transmat.asm.s \
    $(LOCAL_PATH)/../../modules/math/NE10_abs.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_addc.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_addmat.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_add.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_cross.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_detmat.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_divc.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_div.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_dot.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_identitymat.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_invmat.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_len.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_mla.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_mlac.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_mulcmatvec.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_mulc.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_mulmat.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_mul.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_normalize.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_rsbc.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_setc.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_subc.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_submat.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_sub.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_transmat.cpp \
    $(LOCAL_PATH)/../../modules/math/NE10_init_math.cpp \
    $(LOCAL_PATH)/../../modules/NE10_init.cpp \


LOCAL_SRC_FILES :=  $(ne10_source_files)
LOCAL_SRC_FILES += $(ne10_neon_source)

LOCAL_CFLAGS := -D_ARM_ASSEM_ -mfpu=neon -Os

LOCAL_CPPFLAGS +=\
-DANDROID \
-Os \

LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true

LOCAL_MODULE_TAGS := eng

include $(BUILD_STATIC_LIBRARY)




