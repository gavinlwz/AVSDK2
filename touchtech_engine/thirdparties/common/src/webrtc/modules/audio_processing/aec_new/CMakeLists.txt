CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
PROJECT(webrtc_aec)

#Record for test only
ADD_DEFINITIONS("-DDEBUG_RECORD")

SET(SRC_PATH ./)
SET(ORIGINAL_APM_PATH ../../../../uvo_public/trunk/kcrtc/src/modules/audio_processing)

INCLUDE_DIRECTORIES(
    include 
    ../utility
    ../logging
    ${ORIGINAL_APM_PATH}/../../ 
    ${ORIGINAL_APM_PATH}/../../common_audio/signal_processing/include
)

SET(LIB_SRC
    ${SRC_PATH}/echo_cancellation.c
    ${SRC_PATH}/aec_resampler.c
    ${SRC_PATH}/aec_core.c
    ${SRC_PATH}/aec_rdft.c
    )

IF(RTC_WIN32 OR RTC_MAC)
    SET(LIB_SRC_PLUS
        ${SRC_PATH}/aec_core_sse2.c
        ${SRC_PATH}/aec_rdft_sse2.c
        )
    SET(LIB_SRC ${LIB_SRC} ${LIB_SRC_PLUS})
ELSEIF(RTC_ANDROID)
    IF(ARMEABI_V7A OR ARM64_V8A)
        SET(LIB_SRC_PLUS
            ${SRC_PATH}/aec_core_neon.c
            ${SRC_PATH}/aec_rdft_neon.c
            ${SRC_PATH}/aec_rdft_neon1.c
        )
    ELSEIF(X86)
        SET(LIB_SRC_PLUS
            ${SRC_PATH}/aec_core_sse2.c
            ${SRC_PATH}/aec_rdft_sse2.c
        )
    ELSEIF(MIPS)
        SET(LIB_SRC_PLUS
            ${SRC_PATH}/aec_core_mips.c
            ${SRC_PATH}/aec_rdft_mips.c
        )        
    ENDIF()

    SET(LIB_SRC ${LIB_SRC} ${LIB_SRC_PLUS})
ELSEIF(RTC_IOS)
    SET(LIB_SRC_PLUS
        ${SRC_PATH}/aec_core_neon.c
        ${SRC_PATH}/aec_rdft_neon.c
        ${SRC_PATH}/aec_rdft_neon1.c
        )
    SET(LIB_SRC ${LIB_SRC} ${LIB_SRC_PLUS})
ENDIF()

ADD_LIBRARY(webrtc_aec OBJECT ${LIB_SRC})



