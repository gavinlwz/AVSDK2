/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音频通话引擎
 *
 *  当前版本:1.0
 *  作者:brucewang
 *  日期:2015.9.30
 *  说明:对外发布接口
 *
 *  取代版本:0.9
 *  作者:brucewang
 *  日期:2015.9.15
 *  说明:内部测试接口
 ******************************************************************/

#ifdef __ANDROID__
#include <sys/stat.h>
#endif

#define AEC_SYNC_INVOKE 0 // 1: aec buffer farend in sync, 0: aec buffer farend in async
#define AEC_BUFFER_DISCARD_DIRECTION 1 // 1: discard at the back, 0: discard at the front

#include "webrtc/modules/audio_processing/aec/echo_cancellation.h"
#include "webrtc/modules/audio_processing/aec_new/echo_cancellation_new.h"

#include "tinydav/audio/tdav_webrtc_denoise.h"
#include "tinydav/codecs/mixer/tdav_codec_audio_howling_supp.h"
#include "tinydav/codecs/mixer/tdav_codec_audio_biquad_filter.h"
#include "tinydav/codecs/mixer/tdav_codec_audio_mixer.h"
#include "tinydav/codecs/mixer/fftwrap.h"
#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/common_audio/resampler/include/push_resampler.h"

#if HAVE_WEBRTC && (!defined(HAVE_WEBRTC_DENOISE) || HAVE_WEBRTC_DENOISE)

#include "tsk_debug.h"
#include "tsk_memory.h"
#include "tsk_string.h"
#include "tinymedia/tmedia_defaults.h"

//#include "agc.h"
#ifndef NO_SOUNDTOUCH
#include "SoundTouch.h"
#endif
#include <string.h>
#include <math.h>
#include <deque>
#include "YouMeCommon/rnnoise/rnnoise-nu.h"

#if !defined(WEBRTC_AEC_AGGRESSIVE)
#define WEBRTC_AEC_AGGRESSIVE 0
#endif
#if !defined(WEBRTC_MAX_ECHO_TAIL)
#define WEBRTC_MAX_ECHO_TAIL 500
#endif
#if !defined(WEBRTC_MIN_ECHO_TAIL)
#define WEBRTC_MIN_ECHO_TAIL 20 // 0 will cause random crashes
#endif

#define DENOISER_RESAMPLER_BUFSIZE_MAX 1920

#if TDAV_UNDER_MOBILE || 1 // FIXME
typedef int16_t sample_t;
#else
typedef float sample_t;
#endif

typedef enum tdav_webrtc_denoise_vad_type
{
    VAD_QUALITY_MODE = 0,
    VAD_LOW_BIRATE_MODE,
    VAD_AGGRESSIVE_MODE,
    VAD_VERY_AGGRESSIVE_MODE,
    VAD_MODE_MAX_NUM
} tdav_webrtc_denoise_vad_type_t;

typedef struct tdav_webrtc_pin_xs
{
    uint32_t n_duration;
    uint32_t n_rate;
    uint32_t n_channels;
    uint32_t n_sample_size;
} tdav_webrtc_pin_xt;


/** WebRTC denoiser (AEC, NS, AGC...) */
typedef struct tdav_webrtc_denoise_s
{
    TMEDIA_DECLARE_DENOISE;

    void *AECM_inst;
    void *AEC_inst;
    tsk_mutex_handle_t *AEC_mutex;
	DenoiseState **sts;
    float *stsSwap;
    youme::TDAV_NsHandle *NS_inst;
    youme::TDAV_VADHandle *VAD_inst;
    youme::TDAV_CNGHandle *CNG_inst;
    TDAV_AGCHandle *AGC_inst;
    tsk_mutex_handle_t *AGC_mutex;
    howling_suppression_t *HS_inst;
    biquad_filter_t *HPF_inst;
    biquad_filter_t *VBF_inst; // voice boost filter
    reverb_filter_t *REVERB_inst;
#ifndef NO_SOUNDTOUCH
	youme::soundtouch::SoundTouch *m_pSoundTouch;
#endif
    

    uint32_t echo_tail;
    uint32_t echo_skew;


    struct
    {
        uint32_t nb_samples_per_process;
        uint32_t sampling_rate;
        uint32_t channels; // always "1"
        uint32_t sampling_rate_proc;
        uint32_t sampling_rate_rnn_proc;

        struct
        {
#ifdef USE_WEBRTC_RESAMPLE
            youme::webrtc::PushResampler<int16_t> *denoiser_resampler_up;
            youme::webrtc::PushResampler<int16_t> *denoiser_resampler_down;
#else
            SpeexResamplerState *denoiser_resampler_up;
            SpeexResamplerState *denoiser_resampler_down;
#endif
            void     *denoiser_resampler_buffer;
            int32_t  analysis_filter_state1[6];
            int32_t  analysis_filter_state2[6];
            int32_t  synthesis_filter_state1[6];
            int32_t  synthesis_filter_state2[6];
            int16_t  *inPcmSubBand[2];
            int16_t  *outPcmSubBand[2];

#ifdef USE_WEBRTC_RESAMPLE
            youme::webrtc::PushResampler<int16_t> *rnn_denoiser_resampler_to_rnn;
            youme::webrtc::PushResampler<int16_t> *rnn_denoiser_resampler_backfrom_rnn;
#else
            SpeexResamplerState *rnn_denoiser_resampler_to_rnn;
            SpeexResamplerState *rnn_denoiser_resampler_backfrom_rnn;
#endif
            void     *rnn_denoiser_resampler_buffer;
        } rec;
        
        struct
        {
#ifdef USE_WEBRTC_RESAMPLE
            youme::webrtc::PushResampler<int16_t> *denoiser_resampler_up;
            youme::webrtc::PushResampler<int16_t> *denoiser_resampler_down;
#else
            SpeexResamplerState *denoiser_resampler_up;
            SpeexResamplerState *denoiser_resampler_down;
#endif
            void     *denoiser_resampler_buffer;
            int32_t  analysis_filter_state1[6];
            int32_t  analysis_filter_state2[6];
            int32_t  synthesis_filter_state1[6];
            int32_t  synthesis_filter_state2[6];
            int16_t  *inPcmSubBand[2];
            int16_t  *outPcmSubBand[2];
        } ply;
    } neg;
    
    struct
    {
        uint32_t max_size;
        FILE* mic;
        uint32_t mic_size;
        FILE* aec;
        uint32_t aec_size;
        FILE* ns;
        uint32_t ns_size;
        FILE* vad;
        uint32_t vad_size;
        FILE* agc;
        uint32_t agc_size;
        FILE* preagc;
        uint32_t pre_agc_size;
        FILE* speaker;
        uint32_t speaker_size;
        FILE* fft;
        uint32_t fft_size;
        FILE* voice;
        uint32_t voice_size;
    } pcm_file;

    float *pfAecInputNear[2];
    float *pfAecInputFar[2];
    float *pfAecOutput[2];

    std::deque<float*> *pDequeFarFloat;

    TSK_DECLARE_SAFEOBJ;
} tdav_webrtc_denoise_t;

#define PCM_FILE_MIC        1
#define PCM_FILE_AEC        2
#define PCM_FILE_NS         3
#define PCM_FILE_VAD        4
#define PCM_FILE_AGC        5
#define PCM_FILE_SPEAKER    6
#define PCM_FILE_FFT        7
#define PCM_FILE_PREAGC     8
#define PCM_FILE_VOICE      10

static void _open_pcm_files(tdav_webrtc_denoise_t *denoiser, int type)
{
    static char dumpPcmPath[1024] = "";
    const char *pBaseDir = NULL;
    int baseLen = 0;
    
    pBaseDir = tmedia_defaults_get_app_document_path();
    
    if (NULL == pBaseDir) {
        return;
    }
    
    strncpy(dumpPcmPath, pBaseDir, sizeof(dumpPcmPath) - 1);
    baseLen = strlen(dumpPcmPath) + 1; // plus the trailing '\0'
    
    switch (type) {
        case PCM_FILE_MIC:
            strncat(dumpPcmPath, "/dump_mic.pcm", sizeof(dumpPcmPath) - baseLen);
            if (denoiser->pcm_file.mic) {
                fclose(denoiser->pcm_file.mic);
            }
            denoiser->pcm_file.mic = fopen (dumpPcmPath, "wb");
            denoiser->pcm_file.mic_size = 0;
            break;
        case PCM_FILE_AEC:
            strncat(dumpPcmPath, "/dump_aec.pcm", sizeof(dumpPcmPath) - baseLen);
            if (denoiser->pcm_file.aec) {
                fclose(denoiser->pcm_file.aec);
            }
            denoiser->pcm_file.aec = fopen (dumpPcmPath, "wb");
            denoiser->pcm_file.aec_size = 0;
            break;
        case PCM_FILE_NS:
            strncat(dumpPcmPath, "/dump_ns.pcm", sizeof(dumpPcmPath) - baseLen);
            if (denoiser->pcm_file.ns) {
                fclose(denoiser->pcm_file.ns);
            }
            denoiser->pcm_file.ns = fopen (dumpPcmPath, "wb");
            denoiser->pcm_file.ns_size = 0;
            break;
        case PCM_FILE_VAD:
            strncat(dumpPcmPath, "/dump_vad.pcm", sizeof(dumpPcmPath) - baseLen);
            if (denoiser->pcm_file.vad) {
                fclose(denoiser->pcm_file.vad);
            }
            denoiser->pcm_file.vad = fopen (dumpPcmPath, "wb");
            denoiser->pcm_file.vad_size = 0;
            break;
        case PCM_FILE_AGC:
            strncat(dumpPcmPath, "/dump_agc.pcm", sizeof(dumpPcmPath) - baseLen);
            if (denoiser->pcm_file.agc) {
                fclose(denoiser->pcm_file.agc);
            }
            denoiser->pcm_file.agc = fopen (dumpPcmPath, "wb");
            denoiser->pcm_file.agc_size = 0;
            break;
        case PCM_FILE_PREAGC:
            strncat(dumpPcmPath, "/dump_preagc.pcm", sizeof(dumpPcmPath) - baseLen);
            if (denoiser->pcm_file.preagc) {
                fclose(denoiser->pcm_file.preagc);
            }
            denoiser->pcm_file.preagc = fopen (dumpPcmPath, "wb");
            denoiser->pcm_file.pre_agc_size = 0;
            break;
        case PCM_FILE_SPEAKER:
            strncat(dumpPcmPath, "/dump_speaker.pcm", sizeof(dumpPcmPath) - baseLen);
            if (denoiser->pcm_file.speaker) {
                fclose(denoiser->pcm_file.speaker);
            }
            denoiser->pcm_file.speaker = fopen (dumpPcmPath, "wb");
            denoiser->pcm_file.speaker_size = 0;
            break;
        case PCM_FILE_FFT:
            strncat(dumpPcmPath, "/dump_fft.pcm", sizeof(dumpPcmPath) - baseLen);
            if (denoiser->pcm_file.fft) {
                fclose(denoiser->pcm_file.fft);
            }
            denoiser->pcm_file.fft = fopen (dumpPcmPath, "wb");
            denoiser->pcm_file.fft_size = 0;
            break;
        case PCM_FILE_VOICE:
            strncat(dumpPcmPath, "/dump_voice.pcm", sizeof(dumpPcmPath) - baseLen);
            if (denoiser->pcm_file.voice) {
                fclose(denoiser->pcm_file.voice);
            }
            denoiser->pcm_file.voice = fopen (dumpPcmPath, "wb");
            denoiser->pcm_file.voice_size = 0;
            break;
        default:
            return;
    }
}

static void _close_pcm_files(tdav_webrtc_denoise_t *denoiser)
{
    if (denoiser->pcm_file.mic) {
        fclose (denoiser->pcm_file.mic);
        denoiser->pcm_file.mic = NULL;
        denoiser->pcm_file.mic_size = 0;
    }
    if (denoiser->pcm_file.speaker) {
        fclose (denoiser->pcm_file.speaker);
        denoiser->pcm_file.speaker = NULL;
        denoiser->pcm_file.speaker_size = 0;
    }
    if (denoiser->pcm_file.aec) {
        fclose (denoiser->pcm_file.aec);
        denoiser->pcm_file.aec = NULL;
        denoiser->pcm_file.aec_size = 0;
    }
    if (denoiser->pcm_file.ns) {
        fclose (denoiser->pcm_file.ns);
        denoiser->pcm_file.ns = NULL;
        denoiser->pcm_file.ns_size = 0;
    }
    if (denoiser->pcm_file.vad) {
        fclose (denoiser->pcm_file.vad);
        denoiser->pcm_file.vad = NULL;
        denoiser->pcm_file.vad_size = 0;
    }
    if (denoiser->pcm_file.agc) {
        fclose (denoiser->pcm_file.agc);
        denoiser->pcm_file.agc = NULL;
        denoiser->pcm_file.agc_size = 0;
    }
    if (denoiser->pcm_file.preagc) {
        fclose (denoiser->pcm_file.preagc);
        denoiser->pcm_file.preagc = NULL;
        denoiser->pcm_file.pre_agc_size = 0;
    }
    if (denoiser->pcm_file.fft) {
        fclose (denoiser->pcm_file.fft);
        denoiser->pcm_file.fft = NULL;
        denoiser->pcm_file.fft_size = 0;
    }
    if (denoiser->pcm_file.voice) {
        fclose (denoiser->pcm_file.voice);
        denoiser->pcm_file.voice = NULL;
        denoiser->pcm_file.voice_size = 0;
    }
}

static int tdav_webrtc_denoise_set (tmedia_denoise_t *_self, const tmedia_param_t *param)
{
    tdav_webrtc_denoise_t *self = (tdav_webrtc_denoise_t *)_self;
    if (!self || !param)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if (param->value_type == tmedia_pvt_int32)
    {
        if (tsk_striequals (param->key, "echo-tail"))
        {
            int32_t echo_tail = *((int32_t *)param->value);
            self->echo_tail = TSK_CLAMP (WEBRTC_MIN_ECHO_TAIL, echo_tail, WEBRTC_MAX_ECHO_TAIL);
            TSK_DEBUG_INFO ("set_echo_tail (%d->%d)", echo_tail, self->echo_tail);
            return 0;
        }
    }
    return -1;
}

static int tdav_webrtc_denoise_open (tmedia_denoise_t *self,
                                     uint32_t record_frame_size_samples, uint32_t record_sampling_rate, uint32_t record_channels,
                                     uint32_t playback_frame_size_samples, uint32_t playback_sampling_rate, uint32_t playback_channels,
                                     uint32_t purevoice_frame_size_samples, uint32_t purevoice_sampling_rate, uint32_t purevoice_channels)
{
    tdav_webrtc_denoise_t *denoiser = (tdav_webrtc_denoise_t *)self;
    int ret = 0;

    if (!denoiser)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if (denoiser->AECM_inst || denoiser->NS_inst || denoiser->AEC_inst)
    {
        TSK_DEBUG_ERROR ("Denoiser already initialized");
        return -2;
    }

    denoiser->echo_tail = TSK_CLAMP (WEBRTC_MIN_ECHO_TAIL, TMEDIA_DENOISE (denoiser)->echo_tail, WEBRTC_MAX_ECHO_TAIL);
    denoiser->echo_skew = TMEDIA_DENOISE (denoiser)->echo_skew;
    TSK_DEBUG_INFO ("echo_tail=%d, echo_skew=%d, aec_enabled=%d, noise_supp_enabled=%d", denoiser->echo_tail, denoiser->echo_skew, self->aec_enabled, self->noise_supp_enabled);

    //
    //	DENOISER
    //


    // AECM= [8-16]k, AEC=[8-32]k
    denoiser->neg.sampling_rate = TSK_MIN (record_sampling_rate, 48000);
    // Supported by the module: "80"(10ms) and "160"(20ms)
    denoiser->neg.nb_samples_per_process = ((denoiser->neg.sampling_rate * 10) / 1000);
    denoiser->neg.channels = 1;
    if (denoiser->neg.sampling_rate == 48000) {
        denoiser->neg.sampling_rate_proc = 32000;
        denoiser->neg.nb_samples_per_process = ((denoiser->neg.sampling_rate_proc * 10) / 1000);
#ifdef USE_WEBRTC_RESAMPLE
        denoiser->neg.rec.denoiser_resampler_down = new youme::webrtc::PushResampler<int16_t>();
        denoiser->neg.rec.denoiser_resampler_down->InitializeIfNeeded(denoiser->neg.sampling_rate, denoiser->neg.sampling_rate_proc, denoiser->neg.channels);
        denoiser->neg.rec.denoiser_resampler_up = new youme::webrtc::PushResampler<int16_t>();
        denoiser->neg.rec.denoiser_resampler_up->InitializeIfNeeded(denoiser->neg.sampling_rate_proc, denoiser->neg.sampling_rate, denoiser->neg.channels);
#else
        denoiser->neg.rec.denoiser_resampler_down = speex_resampler_init(denoiser->neg.channels, denoiser->neg.sampling_rate, denoiser->neg.sampling_rate_proc, 3, NULL);
        denoiser->neg.rec.denoiser_resampler_up = speex_resampler_init(denoiser->neg.channels, denoiser->neg.sampling_rate_proc, denoiser->neg.sampling_rate, 3, NULL);
#endif
        denoiser->neg.rec.denoiser_resampler_buffer = tsk_malloc(DENOISER_RESAMPLER_BUFSIZE_MAX);
        
#ifdef USE_WEBRTC_RESAMPLE
        denoiser->neg.ply.denoiser_resampler_down = new youme::webrtc::PushResampler<int16_t>();
        denoiser->neg.ply.denoiser_resampler_down->InitializeIfNeeded(denoiser->neg.sampling_rate, denoiser->neg.sampling_rate_proc, denoiser->neg.channels);
        denoiser->neg.ply.denoiser_resampler_up = new youme::webrtc::PushResampler<int16_t>();
        denoiser->neg.ply.denoiser_resampler_up->InitializeIfNeeded(denoiser->neg.sampling_rate_proc, denoiser->neg.sampling_rate, denoiser->neg.channels);
#else
        denoiser->neg.ply.denoiser_resampler_down = speex_resampler_init(denoiser->neg.channels, denoiser->neg.sampling_rate, denoiser->neg.sampling_rate_proc, 3, NULL);
        denoiser->neg.ply.denoiser_resampler_up = speex_resampler_init(denoiser->neg.channels, denoiser->neg.sampling_rate_proc, denoiser->neg.sampling_rate, 3, NULL);
#endif
        denoiser->neg.ply.denoiser_resampler_buffer = tsk_malloc(DENOISER_RESAMPLER_BUFSIZE_MAX);
    } else {
        denoiser->neg.sampling_rate_proc = denoiser->neg.sampling_rate;
    }
    
    denoiser->neg.rec.inPcmSubBand[0] = (int16_t *)tsk_malloc(denoiser->neg.nb_samples_per_process * sizeof(int16_t));
    denoiser->neg.rec.inPcmSubBand[1] = (int16_t *)tsk_malloc(denoiser->neg.nb_samples_per_process * sizeof(int16_t));
    denoiser->neg.rec.outPcmSubBand[0] = (int16_t *)tsk_malloc(denoiser->neg.nb_samples_per_process * sizeof(int16_t));
    denoiser->neg.rec.outPcmSubBand[1] = (int16_t *)tsk_malloc(denoiser->neg.nb_samples_per_process * sizeof(int16_t));
    memset((void*)denoiser->neg.rec.analysis_filter_state1, 0, sizeof(int32_t) * 6);
    memset((void*)denoiser->neg.rec.analysis_filter_state2, 0, sizeof(int32_t) * 6);
    memset((void*)denoiser->neg.rec.synthesis_filter_state1, 0, sizeof(int32_t) * 6);
    memset((void*)denoiser->neg.rec.synthesis_filter_state2, 0, sizeof(int32_t) * 6);
    memset((void*)denoiser->neg.rec.inPcmSubBand[0], 0, sizeof(int16_t) * denoiser->neg.nb_samples_per_process);
    memset((void*)denoiser->neg.rec.inPcmSubBand[1], 0, sizeof(int16_t) * denoiser->neg.nb_samples_per_process);
    memset((void*)denoiser->neg.rec.outPcmSubBand[0], 0, sizeof(int16_t) * denoiser->neg.nb_samples_per_process);
    memset((void*)denoiser->neg.rec.outPcmSubBand[1], 0, sizeof(int16_t) * denoiser->neg.nb_samples_per_process);
    
    denoiser->neg.ply.inPcmSubBand[0] = (int16_t *)tsk_malloc(denoiser->neg.nb_samples_per_process * sizeof(int16_t));
    denoiser->neg.ply.inPcmSubBand[1] = (int16_t *)tsk_malloc(denoiser->neg.nb_samples_per_process * sizeof(int16_t));
    denoiser->neg.ply.outPcmSubBand[0] = (int16_t *)tsk_malloc(denoiser->neg.nb_samples_per_process * sizeof(int16_t));
    denoiser->neg.ply.outPcmSubBand[1] = (int16_t *)tsk_malloc(denoiser->neg.nb_samples_per_process * sizeof(int16_t));
    memset((void*)denoiser->neg.ply.analysis_filter_state1, 0, sizeof(int32_t) * 6);
    memset((void*)denoiser->neg.ply.analysis_filter_state2, 0, sizeof(int32_t) * 6);
    memset((void*)denoiser->neg.ply.synthesis_filter_state1, 0, sizeof(int32_t) * 6);
    memset((void*)denoiser->neg.ply.synthesis_filter_state2, 0, sizeof(int32_t) * 6);
    memset((void*)denoiser->neg.ply.inPcmSubBand[0], 0, sizeof(int16_t) * denoiser->neg.nb_samples_per_process);
    memset((void*)denoiser->neg.ply.inPcmSubBand[1], 0, sizeof(int16_t) * denoiser->neg.nb_samples_per_process);
    memset((void*)denoiser->neg.ply.outPcmSubBand[0], 0, sizeof(int16_t) * denoiser->neg.nb_samples_per_process);
    memset((void*)denoiser->neg.ply.outPcmSubBand[1], 0, sizeof(int16_t) * denoiser->neg.nb_samples_per_process);

    //
    //	AECM/AEC instance
    //
    if (TMEDIA_DENOISE (denoiser)->aec_enabled)
    {
        if (!(TMEDIA_DENOISE (denoiser)->aec_opt) && (denoiser->neg.sampling_rate_proc == 16000)) {
            TSK_DEBUG_INFO("Initial AECM module");
            if ((denoiser->AECM_inst = youme::TDAV_WebRtcAecm_Create ()) == tsk_null)
            {
                TSK_DEBUG_ERROR ("WebRtcAecm_Create failed with error code = %d", ret);
                return ret;
            }
            if (ret = youme::TDAV_WebRtcAecm_Init (denoiser->AECM_inst, denoiser->neg.sampling_rate_proc, denoiser->neg.sampling_rate_proc))
            {
                TSK_DEBUG_ERROR ("WebRtcAecm_Init failed with error code = %d", ret);
                return ret;
            }
            youme::AecmConfig aecConfig;
            aecConfig.echoMode = TMEDIA_DENOISE (denoiser)->aec_echomode;
            aecConfig.cngMode = youme::AecmFalse;
            if (ret = youme::WebRtcAecm_set_config (denoiser->AECM_inst, aecConfig))
            {
                TSK_DEBUG_ERROR ("WebRtcAecm_set_config failed with error code = %d", ret);
            }
        } else {
            if ((TMEDIA_DENOISE (denoiser)->aec_opt) == 2) {
                TSK_DEBUG_INFO("Initial new AEC module");
                if ((denoiser->AEC_inst = youme::webrtc_new::WebRtcAec_Create ()) == tsk_null)
                {
                    TSK_DEBUG_ERROR ("WebRtcAec_Create failed with error code = %d", ret);
                    return ret;
                }
                if ((ret = youme::webrtc_new::WebRtcAec_Init (denoiser->AEC_inst, (int32_t)denoiser->neg.sampling_rate_proc, (int32_t)denoiser->neg.sampling_rate_proc)))
                {
                    TSK_DEBUG_ERROR ("WebRtcAec_Init failed with error code = %d", ret);
                    return ret;
                }
                youme::webrtc_new::AecConfig aecConfig;
                aecConfig.nlpMode  =  TMEDIA_DENOISE (denoiser)->aec_nlpmode;
                aecConfig.skewMode = youme::webrtc_new::kAecFalse;
#ifdef __APPLE__
                aecConfig.metricsMode = youme::webrtc_new::kAecTrue;
                aecConfig.delay_logging = youme::webrtc_new::kAecTrue;
#else
                aecConfig.metricsMode = youme::webrtc_new::kAecTrue;
                aecConfig.delay_logging = youme::webrtc_new::kAecTrue;
#endif
                if ((ret = youme::webrtc_new::WebRtcAec_set_config (denoiser->AEC_inst, aecConfig)))
                {
                    TSK_DEBUG_ERROR ("WebRtcAec_set_config failed with error code = %d", ret);
                }
            } else {
                TSK_DEBUG_INFO("Initial old AEC module");
                if ((denoiser->AEC_inst = youme::webrtc::WebRtcAec_Create ()) == tsk_null)
                {
                    TSK_DEBUG_ERROR ("WebRtcAec_Create failed with error code = %d", ret);
                    return ret;
                }
                if ((ret = youme::webrtc::WebRtcAec_Init (denoiser->AEC_inst, (int32_t)denoiser->neg.sampling_rate_proc, (int32_t)denoiser->neg.sampling_rate_proc)))
                {
                    TSK_DEBUG_ERROR ("WebRtcAec_Init failed with error code = %d", ret);
                    return ret;
                }

                youme::webrtc::AecConfig aecConfig;
                aecConfig.nlpMode  =  TMEDIA_DENOISE (denoiser)->aec_nlpmode;
                aecConfig.skewMode = youme::webrtc::kAecFalse;
#ifdef __APPLE__
                aecConfig.metricsMode = youme::webrtc::kAecTrue;
                aecConfig.delay_logging = youme::webrtc::kAecTrue;
                
#else
                aecConfig.metricsMode = youme::webrtc::kAecTrue;
                aecConfig.delay_logging = youme::webrtc::kAecTrue;
#endif
                if ((ret = youme::webrtc::WebRtcAec_set_config (denoiser->AEC_inst, aecConfig)))
                {
                    TSK_DEBUG_ERROR ("WebRtcAec_set_config failed with error code = %d", ret);
                }
            }
            
            
            denoiser->pfAecInputNear[0] = (float *)tsk_malloc(sizeof(float) * denoiser->neg.sampling_rate_proc / 100);
            denoiser->pfAecInputNear[1] = (float *)tsk_malloc(sizeof(float) * denoiser->neg.sampling_rate_proc / 100);
            denoiser->pfAecInputFar[0]  = (float *)tsk_malloc(sizeof(float) * denoiser->neg.sampling_rate_proc / 100);
            denoiser->pfAecInputFar[1]  = (float *)tsk_malloc(sizeof(float) * denoiser->neg.sampling_rate_proc / 100);
            denoiser->pfAecOutput[0]    = (float *)tsk_malloc(sizeof(float) * denoiser->neg.sampling_rate_proc / 100);
            denoiser->pfAecOutput[1]    = (float *)tsk_malloc(sizeof(float) * denoiser->neg.sampling_rate_proc / 100);
            memset(denoiser->pfAecInputNear[0], 0, sizeof(float) * denoiser->neg.sampling_rate_proc / 100);
            memset(denoiser->pfAecInputNear[1], 0, sizeof(float) * denoiser->neg.sampling_rate_proc / 100);
            memset(denoiser->pfAecInputFar[0], 0, sizeof(float) * denoiser->neg.sampling_rate_proc / 100);
            memset(denoiser->pfAecInputFar[1], 0, sizeof(float) * denoiser->neg.sampling_rate_proc / 100);
            memset(denoiser->pfAecOutput[0], 0, sizeof(float) * denoiser->neg.sampling_rate_proc / 100);
            memset(denoiser->pfAecOutput[1], 0, sizeof(float) * denoiser->neg.sampling_rate_proc / 100);
            denoiser->pDequeFarFloat = new std::deque<float *>();
        }
        denoiser->AEC_mutex = tsk_mutex_create_2(tsk_false);
    }
    
    //
    //	Noise Suppression instance
    //
    if (TMEDIA_DENOISE (denoiser)->noise_supp_enabled)
    {
        if ((denoiser->NS_inst = youme::TDAV_WebRtcNs_Create ()) == tsk_null)
        {
            TSK_DEBUG_ERROR ("WebRtcNs_Create failed with error code = %d", ret);
            return ret;
        }
        if ((ret = youme::TDAV_WebRtcNs_Init (denoiser->NS_inst, denoiser->neg.sampling_rate_proc)))
        {
            TSK_DEBUG_ERROR ("WebRtcNs_Init failed with error code = %d", ret);
            return ret;
        }
        if ((ret = youme::TDAV_WebRtcNs_set_policy (denoiser->NS_inst, TMEDIA_DENOISE (denoiser)->noise_supp_level)))
        {
            TSK_DEBUG_ERROR ("WebRtcNsx_set_policy failed with error code = %d", ret);
            return ret;
        }


        TSK_DEBUG_INFO ("WebRTC denoiser opened: record:%uHz,%uchannels // playback:%uHz,%uchannels // "
                        "neg:%uHz,%uchannels",
                        record_sampling_rate, record_channels, playback_sampling_rate, playback_channels, denoiser->neg.sampling_rate_proc, denoiser->neg.channels);

	}

    // RNNNOISE = 48k
    denoiser->neg.sampling_rate_rnn_proc = 48000;
	if (TMEDIA_DENOISE(denoiser)->noise_rnn_enabled) //only support 48k samplerate now
	{
        if (denoiser->neg.sampling_rate != denoiser->neg.sampling_rate_rnn_proc) {
#ifdef USE_WEBRTC_RESAMPLE
            denoiser->neg.rec.rnn_denoiser_resampler_to_rnn = new youme::webrtc::PushResampler<int16_t>();
            denoiser->neg.rec.rnn_denoiser_resampler_to_rnn->InitializeIfNeeded(denoiser->neg.sampling_rate, denoiser->neg.sampling_rate_rnn_proc, denoiser->neg.channels);
            denoiser->neg.rec.rnn_denoiser_resampler_backfrom_rnn = new youme::webrtc::PushResampler<int16_t>();
            denoiser->neg.rec.rnn_denoiser_resampler_backfrom_rnn->InitializeIfNeeded(denoiser->neg.sampling_rate_rnn_proc, denoiser->neg.sampling_rate, denoiser->neg.channels);
#else
            denoiser->neg.rec.rnn_denoiser_resampler_to_rnn = speex_resampler_init(denoiser->neg.channels, denoiser->neg.sampling_rate, denoiser->neg.sampling_rate_rnn_proc, 3, NULL);
            denoiser->neg.rec.rnn_denoiser_resampler_backfrom_rnn = speex_resampler_init(denoiser->neg.channels, denoiser->neg.sampling_rate_rnn_proc, denoiser->neg.sampling_rate, 3, NULL);
#endif
            denoiser->neg.rec.rnn_denoiser_resampler_buffer = tsk_malloc(DENOISER_RESAMPLER_BUFSIZE_MAX);
        }

		int sample_rate = denoiser->neg.sampling_rate_rnn_proc;
		if (sample_rate <= 0) sample_rate = 48000;

		float max_attenuation = pow(10, - TMEDIA_DENOISE(denoiser)->noise_rnn_db / 10.f); //20db
		std::string models[] = { "orig", "cb", "bd", "lq", "mp", "sh" };

		if (TMEDIA_DENOISE(denoiser)->noise_rnn_model > 5 ){
			TSK_DEBUG_WARN("Model index not found: %d", TMEDIA_DENOISE(denoiser)->noise_rnn_model);
			TMEDIA_DENOISE(denoiser)->noise_rnn_model = 0;
		}
		RNNModel *model = rnnoise_get_model(models[TMEDIA_DENOISE(denoiser)->noise_rnn_model].c_str()); // support mode: orig / cb / bd / lq /mp /sh
		if (!model) {
			TSK_DEBUG_ERROR("Model not found!");
			return 1;
		}

		denoiser->sts = (DenoiseState **)tsk_malloc(denoiser->neg.channels * sizeof(DenoiseState *));
		if (!denoiser->sts) {
			TSK_DEBUG_ERROR("DenoiseState malloc error");
			return 1;
		}
        denoiser->stsSwap = (float *)tsk_malloc(denoiser->neg.channels * sample_rate /100 * sizeof(float));
        if (!denoiser->stsSwap) {
            TSK_DEBUG_ERROR("stsSwap malloc error");
            return 1;
        }
		for (int i = 0; i < denoiser->neg.channels; i++) {
			denoiser->sts[i] = rnnoise_create(model);
			rnnoise_set_param(denoiser->sts[i], RNNOISE_PARAM_MAX_ATTENUATION, max_attenuation);
			rnnoise_set_param(denoiser->sts[i], RNNOISE_PARAM_SAMPLE_RATE, sample_rate);
		}
        
        TSK_DEBUG_INFO ("rnn denoiser opened: mode %d db %d %f",
                        TMEDIA_DENOISE(denoiser)->noise_rnn_model,TMEDIA_DENOISE(denoiser)->noise_rnn_db,max_attenuation);
	}


    //
    //	VAD instance
    //
    if (TMEDIA_DENOISE (denoiser)->vad_enabled)
    {
        if ((denoiser->VAD_inst = youme::TDAV_WebRtcVad_Create ()) == tsk_null)
        {
            TSK_DEBUG_ERROR ("TDAV_WebRtcVad_Create failed with error code = %d", ret);
            return ret;
        }
        youme::TDAV_WebRtcVad_Init (denoiser->VAD_inst);


        if ((ret = youme::TDAV_WebRtcVad_SetMode (denoiser->VAD_inst, VAD_VERY_AGGRESSIVE_MODE)))
        {
            TSK_DEBUG_ERROR ("TDAV_WebRtcVad_SetMode failed with error code = %d", ret);
            return ret;
        }
        TMEDIA_DENOISE (denoiser)->pre_frame_is_silence = tsk_true;
    }

    //
    //	AGC instance
    //
    if (TMEDIA_DENOISE (denoiser)->agc_enabled)
    {
        denoiser->AGC_inst = youme::TDAV_WebRtcAgc_Create ();
        if (denoiser->AGC_inst == tsk_null)
        {
            TSK_DEBUG_ERROR ("TDAV_WebRtcAgc_Create failed with error code = %d", ret);
            return ret;
        }
        //int agcMode = youme::kAgcModeAdaptiveAnalog;

        if ((ret = youme::TDAV_WebRtcAgc_Init (denoiser->AGC_inst, TMEDIA_DENOISE (denoiser)->agc_level_min, TMEDIA_DENOISE (denoiser)->agc_level_max, TMEDIA_DENOISE (denoiser)->agc_mode, denoiser->neg.sampling_rate_proc)))
        {
            TSK_DEBUG_ERROR ("TDAV_WebRtcCng_Init failed with error code = %d", ret);
            return ret;
        }
        TMEDIA_DENOISE (denoiser)->agc_mic_levelin  = 0;
        TMEDIA_DENOISE (denoiser)->agc_mic_levelout = 0;
        
        youme::WebRtcAgcConfig agcUserConfig;
        agcUserConfig.compressionGaindB = TMEDIA_DENOISE (denoiser)->agc_compress_gain;
        agcUserConfig.targetLevelDbfs = TMEDIA_DENOISE (denoiser)->agc_target_level;
        agcUserConfig.limiterEnable = youme::kAgcTrue;
        if ((ret = youme::TDAV_WebRtcAgc_Set_Config (denoiser->AGC_inst, agcUserConfig))) {
            TSK_DEBUG_ERROR ("TDAV_WebRtcAgc_Set_Config failed with error code = %d", ret);
            return ret;
        }
        denoiser->AGC_mutex = tsk_mutex_create_2(tsk_false);
    }
    
#if 0
    //
    // Howling Suppression instance
    //
    if (TMEDIA_DENOISE (denoiser)->howling_supp_enabled) {
        denoiser->HS_inst                  = (howling_suppression_t *)tsk_malloc(sizeof(howling_suppression_t));
        denoiser->HS_inst->HS_RingBuffer   = WebRtc_youme_CreateBuffer(2 * FFT_SIZE, sizeof(int16_t));
        denoiser->HS_inst->HS_FFTInBuffer  = tsk_malloc(FFT_SIZE * sizeof(int16_t));
        denoiser->HS_inst->HS_FFTOutBuffer = tsk_malloc(FFT_SIZE * sizeof(int16_t));
        denoiser->HS_inst->HS_tbl          = spx_fft_init(FFT_SIZE);
        denoiser->HS_inst->pSpecPow        = (int32_t *)tsk_malloc((FFT_SIZE / 2 + 1) * sizeof(int32_t));
        denoiser->HS_inst->u4SampleRate    = record_sampling_rate;
        denoiser->HS_inst->u4Channel       = record_channels;
        denoiser->HS_inst->pNotchFilter    = (howling_suppression_notch_filter_t *)tsk_malloc(sizeof(howling_suppression_notch_filter_t));
        denoiser->HS_inst->pNotchFilter->i2Fc          = 0;
        denoiser->HS_inst->pNotchFilter->i2Fs          = record_sampling_rate;
        denoiser->HS_inst->pNotchFilter->i2Scale       = 0;
        denoiser->HS_inst->pNotchFilter->i2TimeCounter = 0;
        memset((void *)denoiser->HS_inst->pNotchFilter->i4FilterCoeffs, 0, 5 * sizeof(int16_t));
        memset((void *)denoiser->HS_inst->pNotchFilter->i2History, 0, 4 * sizeof(int16_t));
    }
#endif
    
    //
    // Highpass Filter instance
    //
    if (TMEDIA_DENOISE (denoiser)->highpass_filter_enabled) {
        denoiser->HPF_inst          = (biquad_filter_t *)tsk_malloc(sizeof(biquad_filter_t));
        denoiser->HPF_inst->i2Fc    = 50; // Highpass filter FC fix to 50Hz
        denoiser->HPF_inst->i2Fs    = denoiser->neg.sampling_rate_proc;
        denoiser->HPF_inst->i2Scale = 0;
        vBiqTblCreate(denoiser->HPF_inst->i4FilterCoeffs, denoiser->HPF_inst->i2Fs, denoiser->HPF_inst->i2Fc, 0, 0.707f, &(denoiser->HPF_inst->i2Scale), BIQUAD_HPF);
        memset((void *)denoiser->HPF_inst->i2History, 0, 4 * sizeof(int16_t));
        memset((void *)denoiser->HPF_inst->fHistory, 0, 4 * sizeof(float));
    }
    
    //
    // Voice boost instance
    //
    if (TMEDIA_DENOISE (denoiser)->voice_boost_enabled) {
        denoiser->VBF_inst          = (biquad_filter_t *)tsk_malloc(sizeof(biquad_filter_t));
        denoiser->VBF_inst->i2Fc    = 1200; // voice boost filter FC fix to 1200Hz
        denoiser->VBF_inst->i2Fs    = denoiser->neg.sampling_rate_proc;
        denoiser->VBF_inst->i2Scale = 0;
        vBiqTblCreate(denoiser->VBF_inst->i4FilterCoeffs, denoiser->VBF_inst->i2Fs, denoiser->VBF_inst->i2Fc, TMEDIA_DENOISE (denoiser)->voice_boost_dbgain, 0.95f, &(denoiser->VBF_inst->i2Scale), BIQUAD_PEQ);
        memset((void *)denoiser->VBF_inst->i2History, 0, 4 * sizeof(int16_t));
    }
    
    //
    // Reverb Filter instance
    //
    if (TMEDIA_DENOISE (denoiser)->reverb_filter_enabled) {
        int i;
        denoiser->REVERB_inst = (reverb_filter_t *)tsk_malloc(sizeof(reverb_filter_t));
        switch (denoiser->neg.sampling_rate_proc) {
            case 16000:
                memcpy((void*)denoiser->REVERB_inst->i2ConstComDelayTbl, (void*)i2CombFiltDelay16K, sizeof(int16_t) * REVERB_COMB_FILT_NUM);
                memcpy((void*)denoiser->REVERB_inst->i2ConstAllpassDelayTbl, (void*)i2AllpassFiltDelay16K, sizeof(int16_t) * REVERB_ALLPASS_FILT_NUM);
                break;
            case 32000:
                memcpy((void*)denoiser->REVERB_inst->i2ConstComDelayTbl, (void*)i2CombFiltDelay32K, sizeof(int16_t) * REVERB_COMB_FILT_NUM);
                memcpy((void*)denoiser->REVERB_inst->i2ConstAllpassDelayTbl, (void*)i2AllpassFiltDelay32K, sizeof(int16_t) * REVERB_ALLPASS_FILT_NUM);
                break;
            case 48000:
                memcpy((void*)denoiser->REVERB_inst->i2ConstComDelayTbl, (void*)i2CombFiltDelay48K, sizeof(int16_t) * REVERB_COMB_FILT_NUM);
                memcpy((void*)denoiser->REVERB_inst->i2ConstAllpassDelayTbl, (void*)i2AllpassFiltDelay48K, sizeof(int16_t) * REVERB_ALLPASS_FILT_NUM);
                break;
            default:
                memcpy((void*)denoiser->REVERB_inst->i2ConstComDelayTbl, (void*)i2CombFiltDelay16K, sizeof(int16_t) * REVERB_COMB_FILT_NUM);
                memcpy((void*)denoiser->REVERB_inst->i2ConstAllpassDelayTbl, (void*)i2AllpassFiltDelay16K, sizeof(int16_t) * REVERB_ALLPASS_FILT_NUM);
                break;
        }
        for (i = 0; i < REVERB_COMB_FILT_NUM; i++) {
            denoiser->REVERB_inst->pi2CombBuf[i] = (int16_t *)tsk_malloc(sizeof(int16_t) * denoiser->REVERB_inst->i2ConstComDelayTbl[i]);
            memset(denoiser->REVERB_inst->pi2CombBuf[i], 0, sizeof(int16_t) * denoiser->REVERB_inst->i2ConstComDelayTbl[i]);
            denoiser->REVERB_inst->i2CombHistoryIdx[i] = 0;
            denoiser->REVERB_inst->i2CombHistoryData[i] = 0;
        }
        for (i = 0; i < REVERB_ALLPASS_FILT_NUM; i++) {
            denoiser->REVERB_inst->pi2AllpassBuf[i] = (int16_t *)tsk_malloc(sizeof(int16_t) * denoiser->REVERB_inst->i2ConstAllpassDelayTbl[i]);
            memset(denoiser->REVERB_inst->pi2AllpassBuf[i], 0, sizeof(int16_t) * denoiser->REVERB_inst->i2ConstAllpassDelayTbl[i]);
            denoiser->REVERB_inst->i2AllpassHistoryIdx[i] = 0;
        }
        denoiser->REVERB_inst->bBufferRefresh = tsk_false;
    }
    
    if (denoiser->pcm_file.max_size > 0)
    {
        TSK_DEBUG_INFO("Start denoise dumping pcm, max_size:%u", denoiser->pcm_file.max_size);

        _open_pcm_files(denoiser, PCM_FILE_MIC);
        _open_pcm_files(denoiser, PCM_FILE_SPEAKER);
        _open_pcm_files(denoiser, PCM_FILE_VOICE);
        
        if (TMEDIA_DENOISE (denoiser)->aec_enabled) {
            _open_pcm_files(denoiser, PCM_FILE_AEC);
        }
        if (TMEDIA_DENOISE (denoiser)->noise_supp_enabled) {
            _open_pcm_files(denoiser, PCM_FILE_NS);
        }
        if (TMEDIA_DENOISE (denoiser)->vad_enabled) {
            _open_pcm_files(denoiser, PCM_FILE_VAD);
        }
        if (TMEDIA_DENOISE (denoiser)->agc_enabled) {
            _open_pcm_files(denoiser, PCM_FILE_AGC);
            _open_pcm_files(denoiser, PCM_FILE_PREAGC);
        }
        if (TMEDIA_DENOISE (denoiser)->howling_supp_enabled) {
            _open_pcm_files(denoiser, PCM_FILE_FFT);
        }
    }

    return ret;
}

static int tdav_webrtc_denoise_echo_playback (tmedia_denoise_t *self, const void *echo_frame, uint32_t echo_frame_size_bytes, const void *echo_voice_frame, uint32_t echo_voice_frame_size_bytes)
{
    // 注意，这里的echo_frame和echo_voice_frame的采样率可能是不一样的
    // echo_frame是基于playback sample rate的，echo_voice_frame是基于record sample rate的
    // 所以他们的buffer size也是可以不一样的
    
    tdav_webrtc_denoise_t *p_self = (tdav_webrtc_denoise_t *)self;
    int ret = 0;
    int bandNum = 0;
    int sampleNum = 0;

    if (p_self->pcm_file.speaker)
    {
        if (p_self->pcm_file.speaker_size > p_self->pcm_file.max_size) {
            _open_pcm_files(p_self, PCM_FILE_SPEAKER);
        }
        if (p_self->pcm_file.speaker) {
            fwrite (echo_frame, 1, echo_frame_size_bytes, p_self->pcm_file.speaker);
            p_self->pcm_file.speaker_size += echo_frame_size_bytes;
        }
    }

    // tsk_safeobj_lock (p_self);
    if (echo_frame && echo_frame_size_bytes && echo_voice_frame && echo_voice_frame_size_bytes)
    {
        //const sample_t *_echo_frame = (const sample_t *)echo_frame;
        //tsk_size_t _echo_frame_size_bytes = echo_frame_size_bytes;
        //tsk_size_t _echo_frame_size_samples = (_echo_frame_size_bytes / sizeof (int16_t));
        
        const sample_t *_echo_voice_frame = (const sample_t *)echo_voice_frame;
        tsk_size_t _echo_voice_frame_size_bytes = echo_voice_frame_size_bytes;
        tsk_size_t _echo_voice_frame_size_samples = (_echo_voice_frame_size_bytes / sizeof (int16_t));
        
        if (p_self->neg.sampling_rate_proc == 32000 && p_self->neg.sampling_rate == 48000) {
            if (p_self->neg.ply.denoiser_resampler_down && p_self->neg.ply.denoiser_resampler_buffer) {
                uint32_t inSample = _echo_voice_frame_size_samples;
                uint32_t outSample = inSample * p_self->neg.sampling_rate_proc / p_self->neg.sampling_rate;
#ifdef USE_WEBRTC_RESAMPLE
                int src_nb_samples_per_process = ((p_self->neg.ply.denoiser_resampler_down->GetSrcSampleRateHz() * 10) / 1000);
                int dst_nb_samples_per_process = ((p_self->neg.ply.denoiser_resampler_down->GetDstSampleRateHz() * 10) / 1000);
                for (int i = 0; i * src_nb_samples_per_process < inSample; i++) {
                    p_self->neg.ply.denoiser_resampler_down->Resample((const int16_t*)echo_voice_frame + i * src_nb_samples_per_process, src_nb_samples_per_process, (int16_t*)p_self->neg.ply.denoiser_resampler_buffer + i * dst_nb_samples_per_process, 0);
                }
#else
                speex_resampler_process_int(p_self->neg.ply.denoiser_resampler_down, 0, (const spx_int16_t*)echo_voice_frame, &inSample, (spx_int16_t*)p_self->neg.ply.denoiser_resampler_buffer, &outSample);
#endif
                _echo_voice_frame_size_samples = outSample;
                _echo_voice_frame = (const sample_t *)p_self->neg.ply.denoiser_resampler_buffer;
            } else {
                TSK_DEBUG_ERROR("Invalid parameters");
                return -1;
            }
        }
        
        if (p_self->pcm_file.voice) {
            if (p_self->pcm_file.voice_size > p_self->pcm_file.max_size) {
                _open_pcm_files(p_self, PCM_FILE_VOICE);
            }
            if (p_self->pcm_file.voice) {
                fwrite (_echo_voice_frame, 1, _echo_voice_frame_size_samples * sizeof(int16_t), p_self->pcm_file.voice);
                p_self->pcm_file.voice_size += _echo_voice_frame_size_samples * sizeof(int16_t);
            }
        }

        // PROCESS
        uint32_t _samples;
        for (_samples = 0; _samples < _echo_voice_frame_size_samples; _samples += p_self->neg.nb_samples_per_process)
        {
            if (p_self->neg.sampling_rate_proc == 32000) {
                // 子带切割
                YoumeWebRtcSpl_AnalysisQMF(&_echo_voice_frame[_samples],
                                           p_self->neg.nb_samples_per_process,
                                           p_self->neg.ply.inPcmSubBand[0],
                                           p_self->neg.ply.inPcmSubBand[1],
                                           p_self->neg.ply.analysis_filter_state1,
                                           p_self->neg.ply.analysis_filter_state2);
                bandNum = 2;
                sampleNum = p_self->neg.nb_samples_per_process / 2;
            } else {
                memcpy(p_self->neg.ply.inPcmSubBand[0], &_echo_voice_frame[_samples], sizeof(int16_t) * p_self->neg.nb_samples_per_process);
                bandNum = 1;
                sampleNum = p_self->neg.nb_samples_per_process;
            }

            if (TMEDIA_DENOISE (self)->aec_enabled && p_self->AEC_mutex)
            {
                tsk_mutex_lock(p_self->AEC_mutex);
                if ((p_self->AECM_inst) && (!TMEDIA_DENOISE (self)->aec_opt) && (p_self->neg.sampling_rate_proc == 16000)) {
                    if ((ret = youme::TDAV_WebRtcAecm_BufferFarend (p_self->AECM_inst, p_self->neg.ply.inPcmSubBand[0], sampleNum)))
                    {
                        TSK_DEBUG_ERROR ("WebRtcAec_BufferFarend failed with error code = %d, nb_samples_per_process=%u", ret, sampleNum);
                        tsk_mutex_unlock(p_self->AEC_mutex);
                        goto bail;
                    }
                } 
                if ((p_self->AEC_inst) && (TMEDIA_DENOISE (self)->aec_opt)) {
                    int i;
                    if (sampleNum > p_self->neg.sampling_rate_proc / 100) {
                        p_self->pfAecInputFar[0] = (float *)tsk_realloc(p_self->pfAecInputFar, sampleNum * sizeof(float));
                        p_self->pfAecInputFar[1] = (float *)tsk_realloc(p_self->pfAecInputFar, sampleNum * sizeof(float));
                    }
                    for (i = 0; i < sampleNum; i++) {
                        p_self->pfAecInputFar[0][i] = (float)p_self->neg.ply.inPcmSubBand[0][i];
                        p_self->pfAecInputFar[1][i] = (float)p_self->neg.ply.inPcmSubBand[1][i];
                    }

#if (AEC_SYNC_INVOKE == 1)
                    float* temp = (float *)tsk_malloc(sizeof(float) * sampleNum);
                    memcpy((void*)temp, p_self->pfAecInputFar[0], sizeof(float) * sampleNum);
                    p_self->pDequeFarFloat->push_back(temp);
                    if (p_self->pDequeFarFloat->size() > (TMEDIA_DENOISE (self)->aec_buffer_farend_max_size)) {
//                        TSK_DEBUG_WARN("WebRtcAec beyond buffer farend max size, current:%d max:%d", p_self->pDequeFarFloat->size(), (TMEDIA_DENOISE (self)->aec_buffer_farend_max_size));
                        float* toDiscard = tsk_null;
#if (AEC_BUFFER_DISCARD_DIRECTION == 1)
                        toDiscard = p_self->pDequeFarFloat->back();
                        p_self->pDequeFarFloat->pop_back();
#else
                        toDiscard = p_self->pDequeFarFloat->front();
                        p_self->pDequeFarFloat->pop_front();
#endif
                        if(toDiscard) {
                            TSK_FREE(toDiscard);
                        }
                    }
#else

                    if ((TMEDIA_DENOISE (self)->aec_opt) == 2 ) {
                        if ((ret = youme::webrtc_new::WebRtcAec_BufferFarend (p_self->AEC_inst, p_self->pfAecInputFar[0], sampleNum)))
                        {
                            TSK_DEBUG_ERROR ("WebRtcAec_BufferFarend failed with error code = %d, nb_samples_per_process=%u", ret, sampleNum);
                            tsk_mutex_unlock(p_self->AEC_mutex);
                            goto bail;
                        }
                    }else{
                        if ((ret = youme::webrtc::WebRtcAec_BufferFarend (p_self->AEC_inst, p_self->pfAecInputFar[0], sampleNum)))
                        {
                            TSK_DEBUG_ERROR ("WebRtcAec_BufferFarend failed with error code = %d, nb_samples_per_process=%u", ret, sampleNum);
                            tsk_mutex_unlock(p_self->AEC_mutex);
                            goto bail;
                        }
                    }
#endif

                }
                tsk_mutex_unlock(p_self->AEC_mutex);
            }

            if (TMEDIA_DENOISE (p_self)->agc_enabled && p_self->AGC_inst && p_self->AGC_mutex)
            {
                tsk_mutex_lock(p_self->AGC_mutex);
                if ((ret = youme::TDAV_WebRtcAgc_AddFarend (p_self->AGC_inst,
                                                            p_self->neg.ply.inPcmSubBand[0],
                                                            sampleNum)))
                {
                    TSK_DEBUG_ERROR ("WebRtcAgc_AddFarend failed with error code = %d, nb_samples_per_process=%u", ret, sampleNum);
                    tsk_mutex_unlock(p_self->AGC_mutex);
                    goto bail;
                }
                tsk_mutex_unlock(p_self->AGC_mutex);
            }
            
            if (p_self->neg.sampling_rate_proc == 32000) {
                // 子带融合
                YoumeWebRtcSpl_SynthesisQMF(p_self->neg.ply.outPcmSubBand[0],
                                            p_self->neg.ply.outPcmSubBand[1],
                                            p_self->neg.nb_samples_per_process / 2,
                                            (int16_t*)&_echo_voice_frame[_samples],
                                            p_self->neg.ply.synthesis_filter_state1,
                                            p_self->neg.ply.synthesis_filter_state2);
            } else {
                memcpy((void*)&_echo_voice_frame[_samples], p_self->neg.ply.outPcmSubBand[0], sizeof(int16_t) * p_self->neg.nb_samples_per_process);
            }
        }
    }
bail:
    // tsk_safeobj_unlock (p_self);
    return ret;
}

static int tdav_webrtc_denoise_process_record (tmedia_denoise_t *self, void *audio_frame, uint32_t audio_frame_size_bytes, tsk_bool_t *silence_or_noise)
{
    tdav_webrtc_denoise_t *p_self = (tdav_webrtc_denoise_t *)self;
    int ret = 0;
    tsk_bool_t bFadeState[2];
    int bandNum = 0;
    int sampleNum = 0;

    if (!self || !audio_frame || !silence_or_noise) {
        return -1;
    }
    
    if (p_self->pcm_file.mic) {
        if (p_self->pcm_file.mic_size > p_self->pcm_file.max_size) {
            _open_pcm_files(p_self, PCM_FILE_MIC);
        }
        if (p_self->pcm_file.mic) {
            fwrite (audio_frame, 1, audio_frame_size_bytes, p_self->pcm_file.mic);
            p_self->pcm_file.mic_size += audio_frame_size_bytes;
        }
    }
    
    *silence_or_noise = tsk_false;
    bFadeState[0] = tsk_false;
    bFadeState[1] = tsk_false;

    tsk_safeobj_lock (p_self);

    if (audio_frame && audio_frame_size_bytes)
    {
        const sample_t *_audio_frame = (const sample_t *)audio_frame;
        tsk_size_t _audio_frame_size_bytes = audio_frame_size_bytes;
        tsk_size_t _audio_frame_size_samples = (_audio_frame_size_bytes / sizeof (int16_t));
        if ((p_self->neg.sampling_rate_proc == 32000) && (p_self->neg.sampling_rate == 48000)) {

            if ((p_self->neg.rec.denoiser_resampler_down) && (p_self->neg.rec.denoiser_resampler_buffer)) {
                uint32_t inSample = _audio_frame_size_samples;
                uint32_t outSample = inSample * p_self->neg.sampling_rate_proc / p_self->neg.sampling_rate;
#ifdef USE_WEBRTC_RESAMPLE
                int src_nb_samples_per_process = ((p_self->neg.rec.denoiser_resampler_down->GetSrcSampleRateHz() * 10) / 1000);
                int dst_nb_samples_per_process = ((p_self->neg.rec.denoiser_resampler_down->GetDstSampleRateHz() * 10) / 1000);

                for (int i = 0; i * src_nb_samples_per_process < inSample; i++) {
					/* if (TMEDIA_DENOISE(self)->noise_rnn_enabled)
					{
						int16_t *audio_frame_start = (int16_t*)audio_frame + i * src_nb_samples_per_process;
						int r = 0;
						int sampleNum = src_nb_samples_per_process;
                        float *x = p_self->stsSwap;

						if (x != NULL) {
							for (int ci = 0; ci < p_self->neg.channels; ci++) {
								for (r = 0; r < sampleNum; r++)
								{
									x[r] = audio_frame_start[r * p_self->neg.channels + ci];
								}

								rnnoise_process_frame(p_self->sts[ci], x, x);
								for (r = 0; r < sampleNum; r++)
								{
									audio_frame_start[r* p_self->neg.channels + ci] = x[r];
								}
							}
						}
						
						if (p_self->pcm_file.ns) {
                            if (p_self->pcm_file.ns_size > p_self->pcm_file.max_size) {
                                _open_pcm_files(p_self, PCM_FILE_NS);
                            }
                            if (p_self->pcm_file.ns) {
                                fwrite(audio_frame_start, 1, sampleNum * sizeof(int16_t), p_self->pcm_file.ns);
                                p_self->pcm_file.ns_size += sampleNum * sizeof(int16_t);
                            }
						}
						
					}
                     */
                    p_self->neg.rec.denoiser_resampler_down->Resample((const int16_t*)audio_frame + i * src_nb_samples_per_process, src_nb_samples_per_process, (int16_t*)p_self->neg.rec.denoiser_resampler_buffer + i * dst_nb_samples_per_process, 0);
                }
#else
                speex_resampler_process_int(p_self->neg.rec.denoiser_resampler_down, 0, (const spx_int16_t*)audio_frame, &inSample, (spx_int16_t*)p_self->neg.rec.denoiser_resampler_buffer, &outSample);
#endif
                _audio_frame_size_samples = outSample;
                _audio_frame = (const sample_t *)p_self->neg.rec.denoiser_resampler_buffer;
            } else {
                TSK_DEBUG_ERROR("Invalid parameters");
                tsk_safeobj_unlock (p_self);
                return -1;
            }
        }

        int _samples = 0;
        for (_samples = 0; _samples < _audio_frame_size_samples; _samples += p_self->neg.nb_samples_per_process) {
            // HPF
            if ((TMEDIA_DENOISE (self)->highpass_filter_enabled) && (p_self->HPF_inst) && (!_samples)) // 20ms processing
            {
                vBiqFilterProcFloat((int16_t *)_audio_frame,
                                    p_self->HPF_inst->fHistory,
                                    p_self->HPF_inst->i4FilterCoeffs,
                                    p_self->HPF_inst->i2Scale,
                                    _audio_frame_size_samples);
            }
            
            if (p_self->neg.sampling_rate_proc == 32000) {
                // 子带切割
                YoumeWebRtcSpl_AnalysisQMF(&_audio_frame[_samples],
                                           p_self->neg.nb_samples_per_process,
                                           p_self->neg.rec.inPcmSubBand[0],
                                           p_self->neg.rec.inPcmSubBand[1],
                                           p_self->neg.rec.analysis_filter_state1,
                                           p_self->neg.rec.analysis_filter_state2);
                bandNum = 2;
                sampleNum = p_self->neg.nb_samples_per_process / 2;
            } else {
                memcpy(p_self->neg.rec.inPcmSubBand[0], &_audio_frame[_samples], sizeof(int16_t) * p_self->neg.nb_samples_per_process);
                bandNum = 1;
                sampleNum = p_self->neg.nb_samples_per_process;
            }
            
            // AEC
            if ((TMEDIA_DENOISE (self)->aec_enabled) &&
                (p_self->AEC_inst) && (p_self->AEC_mutex) && ((TMEDIA_DENOISE (self)->aec_opt) || (p_self->neg.sampling_rate_proc == 32000))) // 10ms processing
            {
                tsk_mutex_lock(p_self->AEC_mutex);
                int i;
                if (sampleNum > p_self->neg.sampling_rate_proc / 100) {
                    p_self->pfAecInputNear[0] = (float *)tsk_realloc(p_self->pfAecInputNear[0], sampleNum * sizeof(float));
                    p_self->pfAecInputNear[1] = (float *)tsk_realloc(p_self->pfAecInputNear[1], sampleNum * sizeof(float));
                    p_self->pfAecOutput[0]    = (float *)tsk_realloc(p_self->pfAecOutput[0], sampleNum * sizeof(float));
                    p_self->pfAecOutput[1]    = (float *)tsk_realloc(p_self->pfAecOutput[1], sampleNum * sizeof(float));
                }
                for (i = 0; i < sampleNum; i++) {
                    p_self->pfAecInputNear[0][i] = (float)p_self->neg.rec.inPcmSubBand[0][i];
                    p_self->pfAecInputNear[1][i] = (float)p_self->neg.rec.inPcmSubBand[1][i];
                }

#if (AEC_SYNC_INVOKE == 1)
                if (!p_self->pDequeFarFloat->empty()) {
                        float* farend = p_self->pDequeFarFloat->front();
                        if ((TMEDIA_DENOISE (self)->aec_opt) == 2) {
                            ret = youme::webrtc_new::WebRtcAec_BufferFarend (p_self->AEC_inst, farend, sampleNum);
                        } else {
                            ret = youme::webrtc::WebRtcAec_BufferFarend (p_self->AEC_inst, farend, sampleNum);
                        }
                        if(farend) {
                            TSK_FREE(farend);
                        }
                        p_self->pDequeFarFloat->pop_front();
                        if (ret) {
                            TSK_DEBUG_ERROR ("WebRtcAec_BufferFarend failed with error code = %d, nb_samples_per_process=%u", ret, p_self->neg.nb_samples_per_process);
                            tsk_mutex_unlock(p_self->AEC_mutex);
                            goto bail;
                        }
                    }
#endif
                if (TMEDIA_DENOISE (self)->aec_opt == 2) {
                    youme::webrtc_new::WebRtcAec_Process (p_self->AEC_inst,
                                                        p_self->pfAecInputNear,
                                                        bandNum,
                                                        p_self->pfAecOutput,
                                                        sampleNum,
                                                        p_self->echo_tail,
                                                        p_self->echo_skew);
                } else {
                    youme::webrtc::WebRtcAec_Process (p_self->AEC_inst,
                                                        p_self->pfAecInputNear,
                                                        bandNum,
                                                        p_self->pfAecOutput,
                                                        sampleNum,
                                                        p_self->echo_tail,
                                                        p_self->echo_skew);
                }
                for (i = 0; i < sampleNum; i++) {
                    // float -> int16 + rounding
                    p_self->neg.rec.outPcmSubBand[0][i] = (int16_t)(p_self->pfAecOutput[0][i] + 0.5f);
                    p_self->neg.rec.outPcmSubBand[1][i] = (int16_t)(p_self->pfAecOutput[1][i] + 0.5f);
                }
                memcpy(p_self->neg.rec.inPcmSubBand[0], p_self->neg.rec.outPcmSubBand[0], sizeof(int16_t) * sampleNum);
                memcpy(p_self->neg.rec.inPcmSubBand[1], p_self->neg.rec.outPcmSubBand[1], sizeof(int16_t) * sampleNum);
                
                if (p_self->pcm_file.aec) {
                    if (p_self->pcm_file.aec_size > p_self->pcm_file.max_size) {
                        _open_pcm_files(p_self, PCM_FILE_AEC);
                    }
                    if (p_self->pcm_file.aec) {
                        fwrite (p_self->neg.rec.inPcmSubBand[0], 1, sampleNum * sizeof(int16_t), p_self->pcm_file.aec);
                        p_self->pcm_file.aec_size += sampleNum * sizeof(int16_t);
                    }
                }
                tsk_mutex_unlock(p_self->AEC_mutex);
            }

            // NOISE SUPPRESSION
            // WebRTC NoiseSupp only accept 10ms frames Our encoder will always output 20ms frames ==> execute 2x noise_supp
            if (!(TMEDIA_DENOISE(self)->noise_rnn_enabled) && (TMEDIA_DENOISE (self)->noise_supp_enabled) && (p_self->NS_inst)) // 10ms processing
            {
                youme::TDAV_WebRtcNs_Process (p_self->NS_inst, p_self->neg.rec.inPcmSubBand, bandNum, p_self->neg.rec.outPcmSubBand);
                memcpy(p_self->neg.rec.inPcmSubBand[0], p_self->neg.rec.outPcmSubBand[0], sizeof(int16_t) * sampleNum);
                memcpy(p_self->neg.rec.inPcmSubBand[1], p_self->neg.rec.outPcmSubBand[1], sizeof(int16_t) * sampleNum);
                
                if (p_self->pcm_file.ns) {
                    if (p_self->pcm_file.ns_size > p_self->pcm_file.max_size) {
                        _open_pcm_files(p_self, PCM_FILE_NS);
                    }
                    if (p_self->pcm_file.ns) {
                        fwrite (p_self->neg.rec.inPcmSubBand[0], 1, sampleNum * sizeof(int16_t), p_self->pcm_file.ns);
                        p_self->pcm_file.ns_size += sampleNum * sizeof(int16_t);
                    }
                }
				
			}
            
            // AECM
            if ((TMEDIA_DENOISE (self)->aec_enabled) &&
                (p_self->AECM_inst) && (p_self->AEC_mutex) && (!TMEDIA_DENOISE (self)->aec_opt) &&
                (p_self->neg.sampling_rate_proc == 16000)) // 10ms processing
            {
                tsk_mutex_lock(p_self->AEC_mutex);
                youme::TDAV_WebRtcAecm_Process (p_self->AECM_inst,
                                                p_self->neg.rec.inPcmSubBand[0],
                                                tsk_null,
                                                p_self->neg.rec.outPcmSubBand[0],
                                                tsk_null,
                                                sampleNum,
                                                p_self->echo_tail,
                                                p_self->echo_skew);
                memcpy(p_self->neg.rec.inPcmSubBand[0], p_self->neg.rec.outPcmSubBand[0], sizeof(int16_t) * sampleNum);
                memcpy(p_self->neg.rec.inPcmSubBand[1], p_self->neg.rec.outPcmSubBand[1], sizeof(int16_t) * sampleNum);
                
                if (p_self->pcm_file.aec) {
                    if (p_self->pcm_file.aec_size > p_self->pcm_file.max_size) {
                        _open_pcm_files(p_self, PCM_FILE_AEC);
                    }
                    if (p_self->pcm_file.aec) {
                        fwrite (p_self->neg.rec.inPcmSubBand[0], 1, sampleNum * sizeof(int16_t), p_self->pcm_file.aec);
                        p_self->pcm_file.aec_size += sampleNum * sizeof(int16_t);
                    }
                }
                tsk_mutex_unlock(p_self->AEC_mutex);
            }

#if 0
            if ((TMEDIA_DENOISE(self)->howling_supp_enabled) && (p_self->HS_inst)) // 10ms processing
            {
                int16_t  i;
                int32_t  i4Core, i4SideBole1, i4SideBole2;
    #if 0
                int16_t  i2CoreBits, i2SideBoleBits1, i2SideBoleBits2;
    #endif
                double   dBLeft, dBRight;
                int16_t  i2HowlingFreq;
                if (WebRtc_youme_available_write(p_self->HS_inst->HS_RingBuffer) >= p_self->neg.nb_samples_per_process) {
                    WebRtc_WriteBuffer(p_self->HS_inst->HS_RingBuffer, (void *)&_audio_frame[_samples], p_self->neg.nb_samples_per_process);
                }
                if (WebRtc_youme_available_read(p_self->HS_inst->HS_RingBuffer) >= FFT_SIZE) {
                    WebRtc_youme_ReadBuffer(p_self->HS_inst->HS_RingBuffer, NULL, p_self->HS_inst->HS_FFTInBuffer, FFT_SIZE);
                    // Hanning window processing
                    tdav_codec_audio_prefft_window_processing((int16_t *)(p_self->HS_inst->HS_FFTInBuffer),
                                                              (int16_t *)(p_self->HS_inst->HS_FFTInBuffer),
                                                              FFT_SIZE,
                                                              WINDOW_HANNING);
                    // Time domain to frequency domain(512-point FFT)
                    // Input: 512 point time signal, Output: 512/2 point complex-FFT result, real[0]/imag[0]/real[1]/imag[1].../real[255]/imag[255], total 512 output "int16" point
                    // 512 point complex-FFT result related to 0 ~ fs/2, therefore, frequency resolution is fs/2/256(8000/256=31.2Hz @ fs=16K)
                    spx_fft(p_self->HS_inst->HS_tbl, (spx_word16_t *)(p_self->HS_inst->HS_FFTInBuffer), (spx_word16_t *)(p_self->HS_inst->HS_FFTOutBuffer));
                    // Calculate the spetrum power
                    tdav_codec_audio_howling_spectrum_power((int16_t *)(p_self->HS_inst->HS_FFTOutBuffer),
                                                            (int32_t *)(p_self->HS_inst->pSpecPow),
                                                            FFT_SIZE);
                    // Remember the spetrum power is always positive
                    for (i = 1; i < FFT_SIZE / 2 - 1; i++) {
                        i4Core = *((int32_t *)(p_self->HS_inst->pSpecPow) + i);
                        i4SideBole1 = *((int32_t *)(p_self->HS_inst->pSpecPow) + i - 1);
                        i4SideBole2 = *((int32_t *)(p_self->HS_inst->pSpecPow) + i + 1);
    #if 0
                        i2CoreBits  = tdav_codec_audio_howling_bitsU32(u4Core);
                        i2SideBoleBits1 = tdav_codec_audio_howling_bitsU32(u4SideBole1);
                        i2SideBoleBits2 = tdav_codec_audio_howling_bitsU32(u4SideBole2);
                        // Rounding
                        i2CoreBits = ((u4Core >> (i2CoreBits - 1)) >= 0x8) ? i2CoreBits + 1 : i2CoreBits;
                        i2SideBoleBits1 = ((u4SideBole1 >> (i2SideBoleBits1 - 1)) >= 0x8) ? i2SideBoleBits1 + 1 : i2SideBoleBits1;
                        i2SideBoleBits2 = ((u4SideBole2 >> (i2SideBoleBits2 - 1)) >= 0x8) ? i2SideBoleBits2 + 1 : i2SideBoleBits2;
    #endif
                        dBLeft  = 10.0f * log10((double)i4Core / i4SideBole1);
                        dBRight = 10.0f * log10((double)i4Core / i4SideBole2);
                        if ((dBLeft > 22.0f) && (dBRight > 22.0f) && (i >= 96)) { // 3K
                            i2HowlingFreq = (int16_t)(i * 16000 / FFT_SIZE);
                            TSK_DEBUG_INFO("Detect the howling, fs = %d(Hz), dBLeft=%f, dBRight=%f, i4Core=%d, i4SideBole1=%d, i4SideBole2=%d", i2HowlingFreq, dBLeft, dBRight, i4Core, i4SideBole1, i4SideBole2);
                            // 1. Check if it's duplicate
                            if ((TSK_ABS(p_self->HS_inst->pNotchFilter->i2Fc - i2HowlingFreq) > 300) || (p_self->HS_inst->pNotchFilter->i2Fc == 0))
                            {
                                p_self->HS_inst->pNotchFilter->i2Fc = i2HowlingFreq;
                                vBiqTblCreate(p_self->HS_inst->pNotchFilter->i4FilterCoeffs,
                                              p_self->HS_inst->pNotchFilter->i2Fs,
                                              p_self->HS_inst->pNotchFilter->i2Fc,
                                              -24,
                                              1.5f,
                                              &(p_self->HS_inst->pNotchFilter->i2Scale),
                                              BIQUAD_PEQ);
                                memset((void *)(p_self->HS_inst->pNotchFilter->i2History), 0, sizeof(int16_t) * 4);
                                p_self->HS_inst->pNotchFilter->i2TimeCounter = 300; // 1s processing
                            }
                        }
                    }
                    
                    if (p_self->pcm_file.fft) {
                        if (p_self->pcm_file.fft_size > p_self->pcm_file.max_size) {
                            _open_pcm_files(p_self, PCM_FILE_FFT);
                        }
                        if (p_self->pcm_file.fft) {
                            fwrite(p_self->HS_inst->HS_FFTOutBuffer, 1, FFT_SIZE * sizeof(int16_t), p_self->pcm_file.fft);
                            p_self->pcm_file.fft_size += (FFT_SIZE * sizeof(int16_t));
                        }
                    }
                }
                
                if (p_self->HS_inst->pNotchFilter->i2TimeCounter) {
                    vBiqFilterProc((int16_t *)&_audio_frame[_samples],
                                   p_self->HS_inst->pNotchFilter->i2History,
                                   p_self->HS_inst->pNotchFilter->i4FilterCoeffs,
                                   p_self->HS_inst->pNotchFilter->i2Scale,
                                   p_self->neg.nb_samples_per_process);
                    for (i = 0; i < p_self->neg.nb_samples_per_process; i++) {
                        *((int16_t *)&_audio_frame[_samples] + i) >>= 6;
                    }
                    p_self->HS_inst->pNotchFilter->i2TimeCounter--;
                    //TSK_DEBUG_INFO("Filter the mic data~~~~~");
                }
            }
#endif
            // AGC
            if ((TMEDIA_DENOISE (self)->agc_enabled) && (p_self->AGC_inst) && (p_self->AGC_mutex)) // 10ms processing
            {
                tsk_mutex_lock(p_self->AGC_mutex);
                unsigned char saturationWarning = 0;
                int outMicLevel = 0;
                ret = youme::TDAV_WebRtcAgc_Process (p_self->AGC_inst,
                                                     p_self->neg.rec.inPcmSubBand,
                                                     bandNum,
                                                     sampleNum,
                                                     p_self->neg.rec.outPcmSubBand,
                                                     TMEDIA_DENOISE (self)->agc_mic_levelout,
                                                     &outMicLevel,
                                                     0,
                                                     &saturationWarning);
                if (ret || saturationWarning)
                {
                    TSK_DEBUG_ERROR ("WebRtcAgc_Process with error code = %d, nb_samples_per_process=%u,saturationWarning=%d", ret, p_self->neg.nb_samples_per_process, saturationWarning);
                    tsk_mutex_unlock(p_self->AGC_mutex);
                    goto bail;
                }
                TMEDIA_DENOISE (self)->agc_mic_levelout = outMicLevel; // Update mic level out
                if (TMEDIA_DENOISE (self)->agc_mode == youme::kAgcModeAdaptiveAnalog) {
                    TMEDIA_DENOISE (self)->agc_mic_levelin = TMEDIA_DENOISE (self)->agc_mic_levelout; //Update mic level in under adaptive analog mode
                }
            
                if (p_self->pcm_file.agc) {
                    if (p_self->pcm_file.agc_size > p_self->pcm_file.max_size) {
                        _open_pcm_files(p_self, PCM_FILE_AGC);
                    }
                    if (p_self->pcm_file.agc) {
                        fwrite (p_self->neg.rec.outPcmSubBand[0], 1, sampleNum * sizeof(int16_t), p_self->pcm_file.agc);
                        p_self->pcm_file.agc_size += sampleNum * sizeof(int16_t);
                    }
                }
                tsk_mutex_unlock(p_self->AGC_mutex);
            }
            
            if (p_self->neg.sampling_rate_proc == 32000) {
                // 子带融合
                YoumeWebRtcSpl_SynthesisQMF(p_self->neg.rec.outPcmSubBand[0],
                                            p_self->neg.rec.outPcmSubBand[1],
                                            p_self->neg.nb_samples_per_process / 2,
                                            (int16_t*)&_audio_frame[_samples],
                                            p_self->neg.rec.synthesis_filter_state1,
                                            p_self->neg.rec.synthesis_filter_state2);
            } else {
                memcpy((void*)&_audio_frame[_samples], p_self->neg.rec.outPcmSubBand[0], sizeof(int16_t) * p_self->neg.nb_samples_per_process);
            }

            // VAD
            if ((TMEDIA_DENOISE (self)->vad_enabled) && (p_self->VAD_inst)) // 10ms processing
            {
                int32_t i4CurGain, i4PrevGain;
                ret = youme::TDAV_WebRtcVad_Process (p_self->VAD_inst, p_self->neg.sampling_rate_proc, (int16_t *)&_audio_frame[_samples], p_self->neg.nb_samples_per_process);
                *silence_or_noise = (ret == 0) ? tsk_true : tsk_false;
                if (self->vad_smooth_proc) {
                    if (*silence_or_noise) {
                        if (self->pre_frame_is_silence) { // Means memory set to 0 without doubt
                            memset ((void *)&_audio_frame[_samples], 0, p_self->neg.nb_samples_per_process * sizeof(int16_t));
                        } else {                                  // Means fade out current bank
                            i4CurGain = 0x0;
                            i4PrevGain = 0x7fffffff;
                            tdav_codec_volume_smooth_gain ((void *)&_audio_frame[_samples], p_self->neg.nb_samples_per_process, 150, i4CurGain, &i4PrevGain);
                            bFadeState[_samples/p_self->neg.nb_samples_per_process] = tsk_true;
                        }
                    } else {
                        if (self->pre_frame_is_silence) { // Means fade in current bank
                            i4CurGain = 0x7fffffff;
                            i4PrevGain = 0x0;
                            tdav_codec_volume_smooth_gain ((void *)&_audio_frame[_samples], p_self->neg.nb_samples_per_process, 150, i4CurGain, &i4PrevGain);
                            bFadeState[_samples/p_self->neg.nb_samples_per_process] = tsk_true;
                        } else {                                  // Means do nothing
                        }
                    }
                }
                self->pre_frame_is_silence = *silence_or_noise;
                //TSK_DEBUG_INFO("Voice detect result: %d", ret);
                
                if (p_self->pcm_file.vad) {
                    if (p_self->pcm_file.vad_size > p_self->pcm_file.max_size) {
                        _open_pcm_files(p_self, PCM_FILE_VAD);
                    }
                    if (p_self->pcm_file.vad) {
                        fwrite (&_audio_frame[_samples], 1, p_self->neg.nb_samples_per_process * sizeof(int16_t), p_self->pcm_file.vad);
                        p_self->pcm_file.vad_size += p_self->neg.nb_samples_per_process * sizeof(int16_t);
                    }
                }
            }
            
            // Reverb
            if ((TMEDIA_DENOISE (self)->reverb_filter_enabled) && (p_self->REVERB_inst)) // 10ms processing
            {
                if ((TMEDIA_DENOISE (self)->reverb_filter_enabled_ap)) {
                    vReverbFilterProc(p_self->REVERB_inst, (int16_t *)&_audio_frame[_samples], p_self->neg.nb_samples_per_process);
                    p_self->REVERB_inst->bBufferRefresh = tsk_true;
                } else {
                    int i;
                    if (p_self->REVERB_inst->bBufferRefresh) {
                        for (i = 0; i < REVERB_COMB_FILT_NUM; i++) {
                            memset(p_self->REVERB_inst->pi2CombBuf[i], 0, sizeof(int16_t) * p_self->REVERB_inst->i2ConstComDelayTbl[i]);
                            p_self->REVERB_inst->i2CombHistoryIdx[i] = 0;
                            p_self->REVERB_inst->i2CombHistoryData[i] = 0;
                        }
                        for (i = 0; i < REVERB_ALLPASS_FILT_NUM; i++) {
                            memset(p_self->REVERB_inst->pi2AllpassBuf[i], 0, sizeof(int16_t) * p_self->REVERB_inst->i2ConstAllpassDelayTbl[i]);
                            p_self->REVERB_inst->i2AllpassHistoryIdx[i] = 0;
                        }
                        p_self->REVERB_inst->bBufferRefresh = tsk_false;
                    }
                }
            }
            
            // VBF
            if ((TMEDIA_DENOISE (self)->voice_boost_enabled) && (p_self->VBF_inst) && (_samples)) // 20ms processing
            {
                vBiqFilterProc((int16_t *)_audio_frame,
                               p_self->VBF_inst->i2History,
                               p_self->VBF_inst->i4FilterCoeffs,
                               p_self->VBF_inst->i2Scale,
                               _audio_frame_size_samples);
            }
        }
        
        // Handle VAD feedback result
        if (bFadeState[0] | bFadeState[1]) {
            *silence_or_noise = tsk_false;
        }
        //TSK_DEBUG_INFO("Silence or noise detect result: %d", *silence_or_noise);
        
        if ((p_self->neg.sampling_rate_proc == 32000) && (p_self->neg.sampling_rate == 48000)) {
            if ((p_self->neg.rec.denoiser_resampler_up) && (p_self->neg.rec.denoiser_resampler_buffer)) {
                uint32_t inSample = _audio_frame_size_samples;
                uint32_t outSample = inSample * p_self->neg.sampling_rate / p_self->neg.sampling_rate_proc;
#ifdef USE_WEBRTC_RESAMPLE
                int src_nb_samples_per_process = ((p_self->neg.rec.denoiser_resampler_up->GetSrcSampleRateHz() * 10) / 1000);
                int dst_nb_samples_per_process = ((p_self->neg.rec.denoiser_resampler_up->GetDstSampleRateHz() * 10) / 1000);
                for (int i = 0; i * src_nb_samples_per_process < inSample; i++) {
                    p_self->neg.rec.denoiser_resampler_up->Resample((const int16_t*)_audio_frame + i * src_nb_samples_per_process, src_nb_samples_per_process, (int16_t*)audio_frame + i * dst_nb_samples_per_process, 0);
                }
#else
                speex_resampler_process_int(p_self->neg.rec.denoiser_resampler_up, 0, (const spx_int16_t*)_audio_frame, &inSample, (spx_int16_t*)audio_frame, &(outSample));
#endif

                _audio_frame_size_samples = outSample;
                _audio_frame = (const sample_t *)audio_frame;
            } else {
                TSK_DEBUG_ERROR("Invalid parameters");
                tsk_safeobj_unlock (p_self);
                return -1;
            }
        }
        
        //RNNNOISE
        if (TMEDIA_DENOISE(self)->noise_rnn_enabled)
        {
            int16_t* audio_process_frame = nullptr;
            int nb_samples_per_process = ((p_self->neg.sampling_rate_rnn_proc * 10) / 1000); // 10ms per process
            if (p_self->neg.sampling_rate_rnn_proc != p_self->neg.sampling_rate) {
                if ((p_self->neg.rec.rnn_denoiser_resampler_to_rnn) && (p_self->neg.rec.rnn_denoiser_resampler_buffer)) {
                    uint32_t inSample = _audio_frame_size_samples;
                    uint32_t outSample = _audio_frame_size_samples * p_self->neg.sampling_rate_rnn_proc / p_self->neg.sampling_rate;
#ifdef USE_WEBRTC_RESAMPLE
                    int src_nb_samples_per_process = ((p_self->neg.rec.rnn_denoiser_resampler_to_rnn->GetSrcSampleRateHz() * 10) / 1000);
                    int dst_nb_samples_per_process = ((p_self->neg.rec.rnn_denoiser_resampler_to_rnn->GetDstSampleRateHz() * 10) / 1000);
                    for (int i = 0; i * src_nb_samples_per_process < inSample; i++) {
                        p_self->neg.rec.rnn_denoiser_resampler_to_rnn->Resample((const int16_t*)_audio_frame + i * src_nb_samples_per_process, src_nb_samples_per_process, (int16_t*)p_self->neg.rec.rnn_denoiser_resampler_buffer + i * dst_nb_samples_per_process, 0);
                    }
#else
                    speex_resampler_process_int(p_self->neg.rec.rnn_denoiser_resampler_to_rnn, 0, (const spx_int16_t*)_audio_frame, &inSample, (spx_int16_t*)p_self->neg.rec.rnn_denoiser_resampler_buffer, &(outSample));
#endif
                    _audio_frame_size_samples = outSample;
                    audio_process_frame = (int16_t*)p_self->neg.rec.rnn_denoiser_resampler_buffer;
                } else {
                    TSK_DEBUG_ERROR("Invalid parameters");
                    tsk_safeobj_unlock (p_self);
                    return -1;
                }
            }else {
                audio_process_frame = (int16_t*)_audio_frame;
            }
            int _samples = 0;
            for (_samples = 0; _samples < _audio_frame_size_samples; _samples += nb_samples_per_process) {
                int16_t *audio_frame_start = audio_process_frame + _samples;
                int r = 0;
                int sampleNum = nb_samples_per_process;
                float *x = p_self->stsSwap;
                
                if (x != NULL) {
                    for (int ci = 0; ci < p_self->neg.channels; ci++) {
                        for (r = 0; r < sampleNum; r++)
                        {
                            x[r] = audio_frame_start[r * p_self->neg.channels + ci];
                        }
                        
                        rnnoise_process_frame(p_self->sts[ci], x, x);
                        for (r = 0; r < sampleNum; r++)
                        {
                            audio_frame_start[r* p_self->neg.channels + ci] = x[r];
                        }
                    }
                }
            }
            
            if (p_self->neg.sampling_rate_rnn_proc != p_self->neg.sampling_rate) {
                if ((p_self->neg.rec.rnn_denoiser_resampler_backfrom_rnn) && (p_self->neg.rec.rnn_denoiser_resampler_buffer)) {
                    uint32_t inSample = _audio_frame_size_samples;
                    uint32_t outSample = _audio_frame_size_samples * p_self->neg.sampling_rate / p_self->neg.sampling_rate_rnn_proc;
#ifdef USE_WEBRTC_RESAMPLE
                    int src_nb_samples_per_process = ((p_self->neg.rec.rnn_denoiser_resampler_backfrom_rnn->GetSrcSampleRateHz() * 10) / 1000);
                    int dst_nb_samples_per_process = ((p_self->neg.rec.rnn_denoiser_resampler_backfrom_rnn->GetDstSampleRateHz() * 10) / 1000);
                    for (int i = 0; i * src_nb_samples_per_process < inSample; i++) {
                        p_self->neg.rec.rnn_denoiser_resampler_backfrom_rnn->Resample((const int16_t*)p_self->neg.rec.rnn_denoiser_resampler_buffer + i * src_nb_samples_per_process, src_nb_samples_per_process, (int16_t*)_audio_frame + i * dst_nb_samples_per_process, 0);
                    }
#else
                    speex_resampler_process_int(p_self->neg.rec.rnn_denoiser_resampler_backfrom_rnn, 0, (const spx_int16_t*)p_self->neg.rec.rnn_denoiser_resampler_buffer, &inSample, (spx_int16_t*)_audio_frame, &(outSample));
#endif
                    _audio_frame_size_samples = outSample;
                } else {
                    TSK_DEBUG_ERROR("Invalid parameters");
                    tsk_safeobj_unlock (p_self);
                    return -1;
                }
            }
            
            if (p_self->pcm_file.ns) {
                if (p_self->pcm_file.ns_size > p_self->pcm_file.max_size) {
                    _open_pcm_files(p_self, PCM_FILE_NS);
                }
                if (p_self->pcm_file.ns) {
                    fwrite(_audio_frame, 1, _audio_frame_size_samples * sizeof(int16_t), p_self->pcm_file.ns);
                    p_self->pcm_file.ns_size += _audio_frame_size_samples * sizeof(int16_t);
                }
            }
        }

#ifndef NO_SOUNDTOUCH
		//变声处理
		if (TMEDIA_DENOISE(p_self)->soundtouch_enabled && tsk_null == p_self->m_pSoundTouch)
		{
			p_self->m_pSoundTouch = new youme::soundtouch::SoundTouch();
			p_self->m_pSoundTouch->setChannels(p_self->neg.channels);
			p_self->m_pSoundTouch->setSampleRate(p_self->neg.sampling_rate);
			p_self->m_pSoundTouch->setTempo(TMEDIA_DENOISE(p_self)->soundtouch_tempo);
			p_self->m_pSoundTouch->setRate(TMEDIA_DENOISE(p_self)->soundtouch_rate);
			p_self->m_pSoundTouch->setPitch(TMEDIA_DENOISE(p_self)->soundtouch_pitch);
		}

		if (TMEDIA_DENOISE(p_self)->soundtouch_enabled && TMEDIA_DENOISE(p_self)->soundtouch_paraChanged)
		{
			p_self->m_pSoundTouch->setTempo(TMEDIA_DENOISE(p_self)->soundtouch_tempo);
			p_self->m_pSoundTouch->setRate(TMEDIA_DENOISE(p_self)->soundtouch_rate);
			p_self->m_pSoundTouch->setPitch(TMEDIA_DENOISE(p_self)->soundtouch_pitch);
			TMEDIA_DENOISE(p_self)->soundtouch_paraChanged = tsk_false;
		}

		if (TMEDIA_DENOISE(p_self)->soundtouch_enabled && (
            TMEDIA_DENOISE(p_self)->soundtouch_tempo != 1.0f ||
            TMEDIA_DENOISE(p_self)->soundtouch_rate != 1.0f ||
            TMEDIA_DENOISE(p_self)->soundtouch_pitch != 1.0f))
		{
			p_self->m_pSoundTouch->putSamples((const short *)&_audio_frame[0], _audio_frame_size_samples);
			if (p_self->m_pSoundTouch->numSamples() >= _audio_frame_size_samples)
			{
				p_self->m_pSoundTouch->receiveSamples((sample_t *)_audio_frame, _audio_frame_size_samples);
			}
			else
			{
				memset((sample_t *)&_audio_frame[0], 0, _audio_frame_size_samples);
			}
		}
#endif
      
    }
bail:
    tsk_safeobj_unlock (p_self);
    return ret;
}


static int tdav_webrtc_denoise_close (tmedia_denoise_t *self)
{
    tdav_webrtc_denoise_t *denoiser = (tdav_webrtc_denoise_t *)self;

    tsk_safeobj_lock (denoiser);
    if (TMEDIA_DENOISE (denoiser)->aec_enabled) {
        if (denoiser->AEC_mutex) {
            tsk_mutex_lock(denoiser->AEC_mutex);
        }
        if (denoiser->AECM_inst)
        {
            youme::TDAV_WebRtcAecm_Free (denoiser->AECM_inst);
            denoiser->AECM_inst = tsk_null;
        }
        if (denoiser->AEC_inst)
        {
            if ((TMEDIA_DENOISE (denoiser)->aec_opt) == 2) {
                youme::webrtc_new::WebRtcAec_Free (denoiser->AEC_inst);
            } else {
                youme::webrtc::WebRtcAec_Free (denoiser->AEC_inst);
            }
            denoiser->AEC_inst = tsk_null;
            if (denoiser->pfAecOutput[0]) {
                TSK_FREE(denoiser->pfAecOutput[0]);
                denoiser->pfAecOutput[0] = tsk_null;
            }
            if (denoiser->pfAecOutput[1]) {
                TSK_FREE(denoiser->pfAecOutput[1]);
                denoiser->pfAecOutput[1] = tsk_null;
            }
            if (denoiser->pfAecInputFar[0]) {
                TSK_FREE(denoiser->pfAecInputFar[0]);
                denoiser->pfAecInputFar[0] = tsk_null;
            }
            if (denoiser->pfAecInputFar[1]) {
                TSK_FREE(denoiser->pfAecInputFar[1]);
                denoiser->pfAecInputFar[1] = tsk_null;
            }
            if (denoiser->pfAecInputNear[0]) {
                TSK_FREE(denoiser->pfAecInputNear[0]);
                denoiser->pfAecInputNear[0] = tsk_null;
            }
            if (denoiser->pfAecInputNear[1]) {
                TSK_FREE(denoiser->pfAecInputNear[1]);
                denoiser->pfAecInputNear[1] = tsk_null;
            }
            if (denoiser->pDequeFarFloat) {
                std::deque<float *>::iterator it;
                for(it = denoiser->pDequeFarFloat->begin();it != denoiser->pDequeFarFloat->end();it++) {
                    float* temp = *it;
                    if (temp) {
                        TSK_FREE(temp);
                    }
                }
                delete denoiser->pDequeFarFloat;
                denoiser->pDequeFarFloat = nullptr;
            }
        }
        if (denoiser->AEC_mutex) {
            tsk_mutex_unlock(denoiser->AEC_mutex);
            tsk_mutex_destroy(&denoiser->AEC_mutex);
            denoiser->AEC_mutex = tsk_null;
        }
    }
    if (denoiser->NS_inst)
    {
        youme::TDAV_WebRtcNs_Free (denoiser->NS_inst);
        denoiser->NS_inst = tsk_null;
    }
	if (denoiser->sts)
	{
		for (int i = 0; i < denoiser->neg.channels; i++)
			rnnoise_destroy(denoiser->sts[i]);
		TSK_FREE(denoiser->sts);
        if(denoiser->stsSwap) TSK_FREE(denoiser->stsSwap);
		denoiser->sts = tsk_null;
	}
    if (denoiser->neg.rec.rnn_denoiser_resampler_to_rnn) {
#ifdef USE_WEBRTC_RESAMPLE
        delete denoiser->neg.rec.rnn_denoiser_resampler_to_rnn;
        denoiser->neg.rec.rnn_denoiser_resampler_to_rnn = nullptr;
#else
        speex_resampler_destroy(denoiser->neg.rec.rnn_denoiser_resampler_to_rnn);
        denoiser->neg.rec.rnn_denoiser_resampler_to_rnn = nullptr;
#endif
    }
    if (denoiser->neg.rec.rnn_denoiser_resampler_backfrom_rnn) {
#ifdef USE_WEBRTC_RESAMPLE
        delete denoiser->neg.rec.rnn_denoiser_resampler_backfrom_rnn;
        denoiser->neg.rec.rnn_denoiser_resampler_backfrom_rnn = nullptr;
#else
        speex_resampler_destroy(denoiser->neg.rec.rnn_denoiser_resampler_backfrom_rnn);
        denoiser->neg.rec.rnn_denoiser_resampler_backfrom_rnn = nullptr;
#endif
    }
    if (denoiser->neg.rec.rnn_denoiser_resampler_buffer) {
        TSK_SAFE_FREE(denoiser->neg.rec.rnn_denoiser_resampler_buffer);
    }
    if (denoiser->VAD_inst)
    {
        youme::TDAV_WebRtcVad_Free (denoiser->VAD_inst);
        denoiser->VAD_inst = tsk_null;
    }
    if (denoiser->AGC_inst)
    {
        youme::TDAV_WebRtcAgc_Free (denoiser->AGC_inst);
        denoiser->AGC_inst = tsk_null;
        if (denoiser->AGC_mutex) {
            tsk_mutex_unlock(denoiser->AGC_mutex);
            tsk_mutex_destroy(&denoiser->AGC_mutex);
            denoiser->AGC_mutex = tsk_null;
        }
    }
#ifndef NO_SOUNDTOUCH
	if (denoiser->m_pSoundTouch != tsk_null)
	{
		delete denoiser->m_pSoundTouch;
		denoiser->m_pSoundTouch = tsk_null;
	}
#endif
   
    if (denoiser->HS_inst) {
        if (denoiser->HS_inst->HS_RingBuffer) {
            WebRtc_youme_FreeBuffer(denoiser->HS_inst->HS_RingBuffer);
        }
        if (denoiser->HS_inst->HS_tbl) {
            spx_fft_destroy(denoiser->HS_inst->HS_tbl);
        }
        if (denoiser->HS_inst->HS_FFTInBuffer) {
            TSK_SAFE_FREE(denoiser->HS_inst->HS_FFTInBuffer);
        }
        if (denoiser->HS_inst->HS_FFTOutBuffer) {
            TSK_SAFE_FREE(denoiser->HS_inst->HS_FFTOutBuffer);
        }
        if (denoiser->HS_inst->pSpecPow) {
            TSK_SAFE_FREE(denoiser->HS_inst->pSpecPow);
        }
        if (denoiser->HS_inst->pNotchFilter) {
            TSK_SAFE_FREE(denoiser->HS_inst->pNotchFilter);
        }
        TSK_SAFE_FREE(denoiser->HS_inst);
    }
    if (denoiser->HPF_inst) {
        TSK_SAFE_FREE(denoiser->HPF_inst);
    }
    if (denoiser->VBF_inst) {
        TSK_SAFE_FREE(denoiser->VBF_inst);
    }
    if (denoiser->REVERB_inst) {
        int i;
        for (i = 0; i < REVERB_COMB_FILT_NUM; i++) {
            TSK_SAFE_FREE(denoiser->REVERB_inst->pi2CombBuf[i]);
        }
        for (i = 0; i < REVERB_ALLPASS_FILT_NUM; i++) {
            TSK_SAFE_FREE(denoiser->REVERB_inst->pi2AllpassBuf[i]);
        }
        TSK_SAFE_FREE(denoiser->REVERB_inst);
    }
    
    if (denoiser->neg.rec.denoiser_resampler_up) {
#ifdef USE_WEBRTC_RESAMPLE
        delete denoiser->neg.rec.denoiser_resampler_up;
        denoiser->neg.rec.denoiser_resampler_up = nullptr;
#else
        speex_resampler_destroy(denoiser->neg.rec.denoiser_resampler_up);
        denoiser->neg.rec.denoiser_resampler_up = nullptr;
#endif
    }
    if (denoiser->neg.rec.denoiser_resampler_down) {
#ifdef USE_WEBRTC_RESAMPLE
        delete denoiser->neg.rec.denoiser_resampler_down;
        denoiser->neg.rec.denoiser_resampler_down = nullptr;
#else
        speex_resampler_destroy(denoiser->neg.rec.denoiser_resampler_down);
        denoiser->neg.rec.denoiser_resampler_down = nullptr;
#endif
    }
    if (denoiser->neg.rec.denoiser_resampler_buffer) {
        TSK_SAFE_FREE(denoiser->neg.rec.denoiser_resampler_buffer);
    }
    if (denoiser->neg.rec.inPcmSubBand[0]) {
        TSK_SAFE_FREE(denoiser->neg.rec.inPcmSubBand[0]);
    }
    if (denoiser->neg.rec.inPcmSubBand[1]) {
        TSK_SAFE_FREE(denoiser->neg.rec.inPcmSubBand[1]);
    }
    if (denoiser->neg.rec.outPcmSubBand[0]) {
        TSK_SAFE_FREE(denoiser->neg.rec.outPcmSubBand[0]);
    }
    if (denoiser->neg.rec.outPcmSubBand[1]) {
        TSK_SAFE_FREE(denoiser->neg.rec.outPcmSubBand[1]);
    }
    
    if (denoiser->neg.ply.denoiser_resampler_up) {
#ifdef USE_WEBRTC_RESAMPLE
        delete denoiser->neg.ply.denoiser_resampler_up;
        denoiser->neg.ply.denoiser_resampler_up = nullptr;
#else
        speex_resampler_destroy(denoiser->neg.ply.denoiser_resampler_up);
        denoiser->neg.ply.denoiser_resampler_up = nullptr;
#endif
    }
    if (denoiser->neg.ply.denoiser_resampler_down) {
#ifdef USE_WEBRTC_RESAMPLE
        delete denoiser->neg.ply.denoiser_resampler_down;
        denoiser->neg.ply.denoiser_resampler_down = nullptr;
#else
        speex_resampler_destroy(denoiser->neg.ply.denoiser_resampler_down);
        denoiser->neg.ply.denoiser_resampler_down=nullptr;
#endif
    }
    if (denoiser->neg.ply.denoiser_resampler_buffer) {
        TSK_SAFE_FREE(denoiser->neg.ply.denoiser_resampler_buffer);
    }
    if (denoiser->neg.ply.inPcmSubBand[0]) {
        TSK_SAFE_FREE(denoiser->neg.ply.inPcmSubBand[0]);
    }
    if (denoiser->neg.ply.inPcmSubBand[1]) {
        TSK_SAFE_FREE(denoiser->neg.ply.inPcmSubBand[1]);
    }
    if (denoiser->neg.ply.outPcmSubBand[0]) {
        TSK_SAFE_FREE(denoiser->neg.ply.outPcmSubBand[0]);
    }
    if (denoiser->neg.ply.outPcmSubBand[1]) {
        TSK_SAFE_FREE(denoiser->neg.ply.outPcmSubBand[1]);
    }

    _close_pcm_files(denoiser);
    
    tsk_safeobj_unlock (denoiser);

    return 0;
}


//
//	WEBRTC denoiser Plugin definition
//

/* constructor */
static tsk_object_t *tdav_webrtc_denoise_ctor (tsk_object_t *_self, va_list *app)
{
    tdav_webrtc_denoise_t *self = (tdav_webrtc_denoise_t *)_self;
    if (self)
    {
        /* init base */
        tmedia_denoise_init (TMEDIA_DENOISE (self));
        //变声相关
#ifndef NO_SOUNDTOUCH
		self->m_pSoundTouch = tsk_null;
#endif
      

        /* init self */
        tsk_safeobj_init (self);
        self->neg.channels = 1;
        self->neg.rec.denoiser_resampler_up = tsk_null;
        self->neg.rec.denoiser_resampler_down = tsk_null;
        self->neg.rec.denoiser_resampler_buffer = tsk_null;
        self->neg.ply.denoiser_resampler_up = tsk_null;
        self->neg.ply.denoiser_resampler_down = tsk_null;
        self->neg.ply.denoiser_resampler_buffer = tsk_null;
        
        /* pcm dumpping files */
        self->pcm_file.max_size       = tmedia_defaults_get_pcm_file_size_kb() * 1024;
        self->pcm_file.mic            = NULL;
        self->pcm_file.mic_size       = 0;
        self->pcm_file.aec            = NULL;
        self->pcm_file.aec_size       = 0;
        self->pcm_file.ns             = NULL;
        self->pcm_file.ns_size        = 0;
        self->pcm_file.vad            = NULL;
        self->pcm_file.vad_size       = 0;
        self->pcm_file.agc            = NULL;
        self->pcm_file.agc_size       = 0;
        self->pcm_file.preagc         = NULL;
        self->pcm_file.pre_agc_size   = 0;
        self->pcm_file.speaker        = NULL;
        self->pcm_file.speaker_size   = 0;
        self->pcm_file.fft            = NULL;
        self->pcm_file.fft_size       = 0;
        self->pcm_file.voice          = NULL;
        self->pcm_file.voice_size     = 0;
        
        // AEC related
        self->pfAecOutput[0]    = tsk_null;
        self->pfAecOutput[1]    = tsk_null;
        self->pfAecInputFar[0]  = tsk_null;
        self->pfAecInputFar[1]  = tsk_null;
        self->pfAecInputNear[0] = tsk_null;
        self->pfAecInputNear[1] = tsk_null;
        
        self->NS_inst = tsk_null;
        self->AEC_inst = tsk_null;
        self->AECM_inst = tsk_null;
        self->AGC_inst = tsk_null;
        self->CNG_inst = tsk_null;
        self->VAD_inst = tsk_null;
        self->HPF_inst = tsk_null;
        self->HS_inst = tsk_null;
        self->REVERB_inst = tsk_null;
        self->VBF_inst = tsk_null;
        self->sts = tsk_null;
        self->stsSwap = tsk_null;
        
        TSK_DEBUG_INFO ("Create WebRTC denoiser");
    }
    return self;
}
/* destructor */
static tsk_object_t *tdav_webrtc_denoise_dtor (tsk_object_t *_self)
{
    tdav_webrtc_denoise_t *self = (tdav_webrtc_denoise_t *)_self;
    if (self)
    {
        /* deinit base (will close the denoise if not done yet) */
        tmedia_denoise_deinit (TMEDIA_DENOISE (self));
        /* deinit self */
        tdav_webrtc_denoise_close (TMEDIA_DENOISE (self));
        tsk_safeobj_deinit (self);

        TSK_DEBUG_INFO ("*** Destroy WebRTC denoiser ***");
    }

    return self;
}
/* object definition */
static const tsk_object_def_t tdav_webrtc_denoise_def_s = {
    sizeof (tdav_webrtc_denoise_t), tdav_webrtc_denoise_ctor, tdav_webrtc_denoise_dtor, tsk_null,
};
/* plugin definition*/
static const tmedia_denoise_plugin_def_t tdav_webrtc_denoise_plugin_def_s = {
    &tdav_webrtc_denoise_def_s,

    "Audio Denoiser based on Google WebRTC",

    tdav_webrtc_denoise_set,
    tdav_webrtc_denoise_open,
    tdav_webrtc_denoise_echo_playback,
    tdav_webrtc_denoise_process_record,
    NULL,
    tdav_webrtc_denoise_close,
};
const tmedia_denoise_plugin_def_t *tdav_webrtc_denoise_plugin_def_t = &tdav_webrtc_denoise_plugin_def_s;


#endif /* HAVE_WEBRTC */
