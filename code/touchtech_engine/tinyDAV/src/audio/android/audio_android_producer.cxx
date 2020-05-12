/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音频通话引擎
 *
 *  当前版本:1.0
 *  作者:paul
 *  日期:2016.9.19
 *  说明:对外发布接口
 ******************************************************************/
#include "audio_android_producer.h"
#include "audio_android.h"

#include "tinydav/audio/tdav_producer_audio.h"

#include "tsk_string.h"
#include "tsk_memory.h"
#include "tsk_debug.h"
#include "tinymedia/tmedia_defaults.h"
#include "YouMeConstDefine.h"

#include <pthread.h>
typedef int (*RecDataCb_t)(const audio_producer_android_t *_self, const void *audioSamples, int nSamples,
                            int nBytesPerSample, int samplesPerSec, int nChannels);

class AndroidAudioSilenceProducer
{
public:
    ////////////////////////////////
    AndroidAudioSilenceProducer()
    {
        m_bInited          = false;
        m_bCancelled       = false;
        m_bRunning         = false;
        m_frameSampleNum   = 320;
        m_sampleRateHz     = 16000;
        m_channels         = 1;
        m_recDataCb        = NULL;
        m_recDataCbContext = NULL;
        m_pPcmBuf          = NULL;

        if (0 != pthread_mutex_init(&m_cancelFlagLock, NULL)) {
            goto bail;
        }
        
        if (0 != pthread_cond_init(&m_waitCond, NULL)) {
            goto bail;
        }

        m_bInited = true;
        return;
        
    bail:
        TSK_DEBUG_ERROR("AndroidAudioSilenceProducer failed to construct");
    }
    
    ////////////////////////////////
    virtual ~AndroidAudioSilenceProducer()
    {
        stop();
        pthread_cond_destroy(&m_waitCond);
        pthread_mutex_destroy(&m_cancelFlagLock);
        if (m_pPcmBuf) {
            delete[] m_pPcmBuf;
            m_pPcmBuf = NULL;
        }
    }
    
    ////////////////////////////////
    bool start(int frameSampleNum, int bytesPerSample, int sampleRateHz, int channels,
               RecDataCb_t recDataCb, void* recDataCbContext)
    {
        bool bRet = false;
        
        TSK_DEBUG_INFO("## frameSampleNum:%d, bytesPerSample:%d, sampleRateHz:%d, channels:%d",
                       frameSampleNum, bytesPerSample, sampleRateHz, channels);
        if (!m_bInited || m_bRunning || (frameSampleNum <= 0) || (sampleRateHz <= 0)
            || (channels <= 0) || (NULL == recDataCb) || (NULL == recDataCbContext)) {
            return bRet;
        }
        
        m_frameSampleNum    = frameSampleNum;
        m_bytesPerSample    = bytesPerSample;
        m_sampleRateHz      = sampleRateHz;
        m_channels          = channels;
        m_recDataCb         = recDataCb;
        m_recDataCbContext  = recDataCbContext;
        if (m_pPcmBuf) {
            delete[] m_pPcmBuf;
        }
        int32_t bufSize = frameSampleNum * bytesPerSample * channels;
        m_pPcmBuf = (uint8_t*)tsk_calloc(1, bufSize);
        if (NULL == m_pPcmBuf) {
            TSK_DEBUG_ERROR("Not enough memory");
            return bRet;
        }
        memset(m_pPcmBuf, 0, bufSize);
        
        m_bRunning = true;
        int ret = pthread_create(&m_threadId, NULL, &AndroidSilenceProducerThread, (void*)this);
        if (0 == ret) {
            bRet = true;
        } else {
            m_bRunning = false;
            TSK_DEBUG_INFO("Failed to start SilenceProducerThread");
        }
        
        return bRet;
    }

    ////////////////////////////////
    void stop()
    {
        if (!m_bInited || !m_bRunning) {
            return;
        }
        
        pthread_mutex_lock(&m_cancelFlagLock);
        m_bCancelled = true;
        pthread_cond_signal(&m_waitCond);
        pthread_mutex_unlock(&m_cancelFlagLock);
        
        pthread_join(m_threadId, NULL);
        m_bRunning = false;
        TSK_DEBUG_INFO("stop SilenceProducerThread ok");
    }
    
    static void* AndroidSilenceProducerThread(void* pParam)
    {
        AndroidAudioSilenceProducer* pThis = static_cast<AndroidAudioSilenceProducer *>(pParam);
        struct timespec timeoutSpec;
        int frameDurationMs = (pThis->m_frameSampleNum * 1000) / pThis->m_sampleRateHz;
        
        TSK_DEBUG_INFO("#############################################");
        TSK_DEBUG_INFO("## AndroidSilenceProducerThread enter...");
        TSK_DEBUG_INFO("#############################################");
        
        // There's no data from the recorder, start output silence data to push the flow move.
        // This is intended to simulate the behavior as iOS.
        do {
            if (pThis->m_recDataCb && pThis->m_pPcmBuf) {
                pThis->m_recDataCb((audio_producer_android_t*)pThis->m_recDataCbContext, pThis->m_pPcmBuf, pThis->m_frameSampleNum,
                                   pThis->m_bytesPerSample, pThis->m_sampleRateHz, pThis->m_channels);
            }
            
            // Wait for as long as a frame duration.
            pthread_mutex_lock(&(pThis->m_cancelFlagLock));
            // Before waiting, we should check the flag first, otherwise, we may miss a signal.
            if (!pThis->m_bCancelled) {
                getTimeoutSpec(&timeoutSpec, frameDurationMs);
                pthread_cond_timedwait(&(pThis->m_waitCond), &(pThis->m_cancelFlagLock), &timeoutSpec);
            }
            if (pThis->m_bCancelled) {
                pthread_mutex_unlock(&(pThis->m_cancelFlagLock));
                break;
            }
            pthread_mutex_unlock(&(pThis->m_cancelFlagLock));
        } while(true);
        
    bail:
        TSK_DEBUG_INFO("#############################################");
        TSK_DEBUG_INFO("## AndroidSilenceProducerThread exit");
        TSK_DEBUG_INFO("#############################################");
        
        return NULL;
    }
    
private:
    // Construct a "struct timespec" with the specified timeoutMs
    static void getTimeoutSpec(struct timespec *ts, int timeoutMs)
    {
        struct timeval  curTimeVal;
        gettimeofday(&curTimeVal, NULL);
        ts->tv_sec = curTimeVal.tv_sec + ((curTimeVal.tv_usec +  (timeoutMs * 1000 - 300)) / 1000000);
        ts->tv_nsec = ((curTimeVal.tv_usec +  (timeoutMs * 1000 - 300)) % 1000000) * 1000;
    }
    
    pthread_t           m_threadId;
    
    pthread_cond_t      m_waitCond;
    pthread_mutex_t     m_cancelFlagLock;
    bool                m_bCancelled;
    
    bool                m_bInited;
    bool                m_bRunning;
    
    int                 m_frameSampleNum;
    int                 m_bytesPerSample;
    int                 m_sampleRateHz;
    int                 m_channels;
    RecDataCb_t         m_recDataCb;
    void*               m_recDataCbContext;
    
    uint8_t*            m_pPcmBuf;
};

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

AndroidAudioSilenceProducer *g_pSilenceProducer = NULL;

int audio_producer_android_handle_data_20ms (const audio_producer_android_t *_self, const void *audioSamples, int nSamples, int nBytesPerSample, int samplesPerSec, int nChannels)
{
    if (!_self || !audioSamples || !nSamples)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (!TMEDIA_PRODUCER (_self)->enc_cb.callback)
    {
        TSK_DEBUG_WARN ("No callback function is registered for the producer");
        return 0;
    }
    if ((nSamples != (samplesPerSec / 100 * 2))) // 20ms data
    {
        TSK_DEBUG_ERROR ("Not producing 20ms samples (nSamples=%d, samplesPerSec=%d)", nSamples, samplesPerSec);
        return -2;
    }
    if ((nBytesPerSample != (TMEDIA_PRODUCER (_self)->audio.bits_per_sample >> 3)))
    {
        TSK_DEBUG_ERROR ("%d not valid bytes/samples", nBytesPerSample);
        return -3;
    }
    if ((nChannels != TMEDIA_PRODUCER (_self)->audio.channels))
    {
        TSK_DEBUG_ERROR ("Recording - %d not the expected number of channels but should be %d", nChannels, TMEDIA_PRODUCER (_self)->audio.channels);
        return -4;
    }

    int nSamplesInBytes = (nSamples * nBytesPerSample);
    if (_self->buffer.index + nSamplesInBytes > _self->buffer.size)
    {
        TSK_DEBUG_ERROR ("Buffer overflow");
        return -5;
    }

    audio_producer_android_t *self = const_cast<audio_producer_android_t *> (_self);

    if (self->isMuted) // Software mute
    {
        memset ((((uint8_t *)self->buffer.ptr) + self->buffer.index), 0, nSamplesInBytes);
    }
    else
    {
        memcpy ((((uint8_t *)self->buffer.ptr) + self->buffer.index), audioSamples, nSamplesInBytes);
    }
    self->buffer.index += nSamplesInBytes;

    if (self->buffer.index == self->buffer.size)
    {
        self->buffer.index = 0;
        TMEDIA_PRODUCER (self)->enc_cb.callback (TMEDIA_PRODUCER (self)->enc_cb.callback_data, self->buffer.ptr, self->buffer.size);
    }

    return 0;
}


/* ============ Media Producer Interface ================= */
static int audio_producer_android_set (tmedia_producer_t *_self, const tmedia_param_t *param)
{
    audio_producer_android_t *self = (audio_producer_android_t *)_self;
    if (param->plugin_type == tmedia_ppt_producer)
    {
        if (param->value_type == tmedia_pvt_int32)
        {
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MICROPHONE_MUTE))
            {
                self->isMuted = (*((int32_t *)param->value) != 0);
                TSK_DEBUG_INFO ("Set mic mute:%d", self->isMuted);
                return 0;
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MIC_VOLUME))
            {
                // TODO!! Currently no use mic volume. Can easily use software volume if needed
                return 0;
            }
        }
    }
    return tdav_producer_audio_set (TDAV_PRODUCER_AUDIO (self), param);
}

static int audio_producer_android_get (tmedia_producer_t *_self, tmedia_param_t *param)
{
    audio_producer_android_t *self = (audio_producer_android_t *)_self;
    if (param->plugin_type == tmedia_ppt_producer)
    {
        if (param->value_type == tmedia_pvt_int32)
        {
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_RECORDING_ERROR)) {
                int initStatus = JNI_Get_Audio_Record_Status();
                *((int32_t*)(param->value)) = YOUME_SUCCESS;
                switch (initStatus) {
                    case AUDIO_RECORD_BAD_VALUE:
                        *((int32_t*)(param->value)) = YOUME_ERROR_REC_NO_PERMISSION;
                        break;
                    case AUDIO_RECORD_UNINITIALIZED:
                        *((int32_t*)(param->value)) = YOUME_ERROR_REC_INIT_FAILED;
                        break;
                    case AUDIO_RECORD_ERROR:
                    case AUDIO_RECORD_OPERATION:
                        *((int32_t*)(param->value)) = YOUME_ERROR_REC_OTHERS;
                        break;
                    case AUDIO_RECORD_PERMISSION:
                        *((int32_t*)(param->value)) = YOUME_ERROR_REC_NO_PERMISSION;
                        break;
                    case AUDIO_RECORD_NO_DATA:
                        *((int32_t*)(param->value)) = YOUME_ERROR_REC_NO_DATA;
                        break;
                    default:
                        *((int32_t*)(param->value)) = YOUME_SUCCESS;
                        break;
                }
            } else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_RECORDING_ERROR_EXTRA)) {
                *((int32_t*)(param->value)) = YOUME_SUCCESS;
            }
        }
    }
    
    return 0;
}

static int audio_producer_android_prepare (tmedia_producer_t *_self, const tmedia_codec_t *codec)
{
    audio_producer_android_t *self = (audio_producer_android_t *)_self;
    if (!self || !codec)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    // create audio instance
    if (!(self->audioInstHandle = audio_android_instance_create (TMEDIA_PRODUCER (self)->session_id, 1)))
    {
        TSK_DEBUG_ERROR ("Failed to create audio instance handle");
        return -2;
    }

    // check that ptime is mutiple of 10
    if ((codec->plugin->audio.ptime % 10))
    {
        TSK_DEBUG_ERROR ("ptime=%d not multiple of 10", codec->plugin->audio.ptime);
        return -3;
    }

    // init input parameters from the codec
    TMEDIA_PRODUCER (self)->audio.channels = TMEDIA_CODEC_CHANNELS_AUDIO_ENCODING (codec); // Mono
    TMEDIA_PRODUCER (self)->audio.rate = TMEDIA_CODEC_RATE_ENCODING (codec); // 16K
    TMEDIA_PRODUCER (self)->audio.ptime = TMEDIA_CODEC_PTIME_AUDIO_ENCODING (codec);

    TSK_DEBUG_INFO ("audio_producer_android_prepare(channels=%d, rate=%d, ptime=%d)", TMEDIA_PRODUCER (self)->audio.channels,
                                                                                      TMEDIA_PRODUCER (self)->audio.rate,
                                                                                      TMEDIA_PRODUCER (self)->audio.ptime);

    // prepare playout device and update output parameters
    int ret;
    ret = audio_android_instance_prepare_producer (self->audioInstHandle, &_self);

    // now that the producer is prepared we can initialize internal buffer using device caps
    if (ret == 0)
    {
        if (tmedia_defaults_get_android_faked_recording()) {
            if (g_pSilenceProducer) {
                delete g_pSilenceProducer;
            }
            g_pSilenceProducer = new AndroidAudioSilenceProducer;
        } else {
            bool isStreamVoice = false;
            if (tmedia_defaults_get_comm_mode_enabled()) {
                isStreamVoice = true;
            }
            JNI_Init_Audio_Record(TMEDIA_PRODUCER (self)->audio.rate,
                                  TMEDIA_PRODUCER (self)->audio.channels,
                                  TMEDIA_PRODUCER (self)->audio.bits_per_sample >> 3,
                                  isStreamVoice,
                                  _self);
        }
        
        // allocate buffer
        int xsize = ((TMEDIA_PRODUCER (self)->audio.ptime * TMEDIA_PRODUCER (self)->audio.rate) / 1000) * (TMEDIA_PRODUCER (self)->audio.bits_per_sample >> 3);
        TSK_DEBUG_INFO ("producer buffer xsize = %d", xsize);
        if (!(self->buffer.ptr = tsk_realloc (self->buffer.ptr, xsize)))
        {
            TSK_DEBUG_ERROR ("Failed to allocate buffer with size = %d", xsize);
            self->buffer.size = 0;
            return -1;
        }
        self->buffer.size = xsize;
        self->buffer.index = 0;
    }
    return 0;
}

static int audio_producer_android_start (tmedia_producer_t *_self)
{
    audio_producer_android_t *self = (audio_producer_android_t *)_self;
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    TSK_DEBUG_INFO ("audio_producer_android_start");
    
    audio_android_instance_start_producer (self->audioInstHandle);
    
    if (g_pSilenceProducer) {
        g_pSilenceProducer->start((TMEDIA_PRODUCER (self)->audio.ptime * TMEDIA_PRODUCER (self)->audio.rate) / 1000,
                                  TMEDIA_PRODUCER (self)->audio.bits_per_sample >> 3,
                                  TMEDIA_PRODUCER (self)->audio.rate,
                                  TMEDIA_PRODUCER (self)->audio.channels,
                                  audio_producer_android_handle_data_20ms,
                                  self);
    } else {
        JNI_Start_Audio_Record();
    }
    
    return 0;
}

static int audio_producer_android_pause (tmedia_producer_t *self)
{
    return 0;
}

static int audio_producer_android_stop (tmedia_producer_t *_self)
{
    audio_producer_android_t *self = (audio_producer_android_t *)_self;
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    audio_android_instance_stop_producer (self->audioInstHandle);
    
    if (g_pSilenceProducer) {
        g_pSilenceProducer->stop();
    } else {
        JNI_Stop_Audio_Record();
    }
    
    return 0;
}


//
//	SLES audio producer object definition
//
/* constructor */
static tsk_object_t *audio_producer_android_ctor (tsk_object_t *_self, va_list *app)
{
    audio_producer_android_t *self = (audio_producer_android_t *)_self;
    if (self)
    {
        /* init base */
        tdav_producer_audio_init (TDAV_PRODUCER_AUDIO (self));
        /* init self */
        self->isMuted = 1;
        TSK_DEBUG_INFO ("Initial mic mute:%d", self->isMuted);

    }
    return self;
}
/* destructor */
static tsk_object_t *audio_producer_android_dtor (tsk_object_t *_self)
{
    audio_producer_android_t *self = (audio_producer_android_t *)_self;

    if (g_pSilenceProducer) {
        delete g_pSilenceProducer;
        g_pSilenceProducer = NULL;
    }
    
    if (self)
    {
        /* stop */
        audio_producer_android_stop (TMEDIA_PRODUCER (self));
        /* deinit self */
        if (self->audioInstHandle)
        {
            audio_android_instance_destroy (&self->audioInstHandle);
        }
        TSK_FREE (self->buffer.ptr);

        /* deinit base */
        tdav_producer_audio_deinit (TDAV_PRODUCER_AUDIO (self));
    }

    return self;
}
/* object definition */
static const tsk_object_def_t audio_producer_android_def_s = {
    sizeof (audio_producer_android_t),
    audio_producer_android_ctor,
    audio_producer_android_dtor,
    tdav_producer_audio_cmp,
};
/* plugin definition*/
static const tmedia_producer_plugin_def_t audio_producer_android_plugin_def_s = { &audio_producer_android_def_s,

                                                                                   tmedia_audio,
                                                                                   "ANDROID audio producer",

                                                                                   audio_producer_android_set,
                                                                                   audio_producer_android_get,
                                                                                   audio_producer_android_prepare,
                                                                                   audio_producer_android_start,
                                                                                   audio_producer_android_pause,
                                                                                   audio_producer_android_stop };
const tmedia_producer_plugin_def_t *audio_producer_android_plugin_def_t = &audio_producer_android_plugin_def_s;
