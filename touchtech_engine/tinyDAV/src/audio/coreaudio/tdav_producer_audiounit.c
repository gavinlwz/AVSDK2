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
#include "tinydav/audio/coreaudio/tdav_producer_audiounit.h"
#include "tinydav/codecs/mixer/speex_resampler.h"

// http://developer.apple.com/library/ios/#documentation/MusicAudio/Conceptual/AudioUnitHostingGuide_iOS/Introduction/Introduction.html%23//apple_ref/doc/uid/TP40009492-CH1-SW1

#ifdef __APPLE__

#include <mach/mach.h>
#import <sys/sysctl.h>

#include "tsk_string.h"
#include "tsk_memory.h"
#include "tsk_thread.h"
#include "tsk_debug.h"
#include "tsk_time.h"
#include "XTimerCWrapper.h"

#include <MediaToolbox/MediaToolbox.h>

#include "tinydav/tdav_apple.h"
#include "YouMeConstDefine.h"
#include "tinymedia/tmedia_defaults.h"
#define kRingPacketCount 10

static OSStatus __handle_input_buffer (void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
    OSStatus status = noErr;
    tdav_producer_audiounit_t *producer = (tdav_producer_audiounit_t *)inRefCon;

    tdav_apple_monitor_add(0);
    
    // holder
    AudioBuffer buffer;
    buffer.mData = tsk_null;
    buffer.mDataByteSize = 0;
    buffer.mNumberChannels = TMEDIA_PRODUCER (producer)->audio.channels;

    // list of holders
    AudioBufferList buffers;
    buffers.mNumberBuffers = 1;
    buffers.mBuffers[0] = buffer;

    // render to get frames from the system
    status = AudioUnitRender (tdav_audiounit_handle_get_instance (producer->audioUnitHandle), ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, &buffers);
    if (status == 0)
    {
        // must not be done on async thread: doing it gives bad audio quality when audio+video call
        // is done with CPU consuming codec (e.g. speex or g729)
        youme_buffer_write (producer->ring.buffer, buffers.mBuffers[0].mData, buffers.mBuffers[0].mDataByteSize);
        int avail = youme_buffer_get_available (producer->ring.buffer);
        while (producer->started && avail >= producer->ring.chunck.size)
        {
            avail -= youme_buffer_read (producer->ring.buffer, (void *)producer->ring.chunck.buffer, (int)producer->ring.chunck.size);
            // In some cases (e.g. turn on airplay and then off), the AudioUnit will failed to mute.
            // So memset the buffer to all 0 to make sure mute will always work. 
            if (producer->muted) {
                memset(producer->ring.chunck.buffer, 0, producer->ring.chunck.size);
            }
            
            //=0表示就是按audio.rate来采集的，不需要重采样,等于audio.rate也不需要重采样
            if( producer->ring.resampleBuffer.recordSampleRate == 0 ||
               producer->ring.resampleBuffer.recordSampleRate == TMEDIA_PRODUCER( producer)->audio.rate )
            {
                TMEDIA_PRODUCER (producer)->enc_cb.callback (TMEDIA_PRODUCER (producer)->enc_cb.callback_data, producer->ring.chunck.buffer, producer->ring.chunck.size);
            }
            else
            {
                uint32_t inSample  = producer->ring.chunck.size / sizeof(int16_t);
                uint32_t outSample = producer->ring.resampleBuffer.size / sizeof(int16_t);
                
                speex_resampler_process_int( producer->ring.resampleBuffer.resampler, 0,
                                            (const spx_int16_t*)producer->ring.chunck.buffer, &inSample,
                                            (spx_int16_t*)producer->ring.resampleBuffer.buffer, &outSample);
                
                TMEDIA_PRODUCER (producer)->enc_cb.callback (TMEDIA_PRODUCER (producer)->enc_cb.callback_data, producer->ring.resampleBuffer.buffer, producer->ring.resampleBuffer.size);
            }
        }
    }

    return status;
}

static void* _silence_producer_thread(void* pParam)
{
    tdav_producer_audiounit_t *producer = (tdav_producer_audiounit_t *)pParam;
    struct timeval  curTimeVal;
    struct timespec timeoutSpec;
    
    uint32_t ptime_ms = TMEDIA_PRODUCER (producer)->audio.ptime;
    uint32_t pcm_buf_size = ((uint32_t)(TMEDIA_PRODUCER (producer)->audio.channels)
                             * (uint32_t)(TMEDIA_PRODUCER (producer)->audio.rate)
                             * (uint32_t)ptime_ms) / 1000
                             * sizeof(int16_t);
    void* pcm_buf = calloc(pcm_buf_size, 1);
    if (!pcm_buf) {
        TSK_DEBUG_ERROR("Failed to allocate pcm buffer size:%u", pcm_buf_size);
        return NULL;
    }

    if (NULL == TMEDIA_PRODUCER (producer)->enc_cb.callback) {
        free(pcm_buf);
        pcm_buf = 0;
        TSK_DEBUG_ERROR("enc_cb.callback is NULL");
        return NULL;
    }
    
    TSK_DEBUG_INFO("#############################################");
    TSK_DEBUG_INFO("## _silence_producer_thread enter, pcm_buf_size:%u", pcm_buf_size);
    TSK_DEBUG_INFO("#############################################");
    
    uint64_t start_time = tsk_gettimeofday_ms();
    uint64_t curr_time = 0, last_time = 0;
    uint64_t right_time;
    uint64_t time_count = 1;
    
    do {
        if (producer->started) {
            memset(pcm_buf, 0, pcm_buf_size);
            TMEDIA_PRODUCER (producer)->enc_cb.callback (TMEDIA_PRODUCER (producer)->enc_cb.callback_data, pcm_buf, pcm_buf_size);
        }

        right_time = start_time + time_count * ptime_ms;
        curr_time = tsk_gettimeofday_ms();
        if (curr_time < right_time) {
            xt_timer_wait(right_time - curr_time);
        }
        last_time = tsk_gettimeofday_ms();
        // TSK_DEBUG_INFO("_silence_producer_thread tick cost_time:%llu, avg:%f", last_time - start_time, (double)(last_time - start_time)/ time_count);
        ++time_count;
    } while (producer->started);

    free(pcm_buf);
    pcm_buf = 0;
    
bail:
    TSK_DEBUG_INFO("#############################################");
    TSK_DEBUG_INFO("## _silence_producer_thread exit");
    TSK_DEBUG_INFO("#############################################");
    
    return NULL;
}

// Note: for each instance, this function should only be called once.
static int _start_silence_producer_thread(tdav_producer_audiounit_t *producer)
{
    int ret = 0;
    
    if (!producer) {
        return -1;
    }

    if (0 != pthread_mutex_init(&producer->silenceThreadMutex, NULL)) {
        return -1;
    }
    if (0 != pthread_cond_init(&producer->silenceThreadCond, NULL)) {
        return -1;
    }
    
    //ret = pthread_create(&producer->silenceThreadId, NULL, &_silence_producer_thread, (void*)producer);
    ret = tsk_thread_create(&producer->silenceThreadId, _silence_producer_thread, (void*)producer);
    if ((0 != ret) && (!producer->silenceThreadId)) {
        TSK_DEBUG_INFO("Failed to start _silence_producer_thread");
        return -1;
    }
    ret = tsk_thread_set_priority(producer->silenceThreadId, TSK_THREAD_PRIORITY_TIME_CRITICAL);
    producer->silenceThreadStarted = 1;
    
    return 0;
}

static void _stop_silence_producer_thread(tdav_producer_audiounit_t *producer)
{
    if (!producer) {
        return;
    }

    // Signal the thread to wake up if it's sleeping
    pthread_mutex_lock(&producer->silenceThreadMutex);
    pthread_cond_signal(&producer->silenceThreadCond);
    pthread_mutex_unlock(&producer->silenceThreadMutex);
    
    //pthread_join(producer->silenceThreadId, NULL);
    tsk_thread_join(&producer->silenceThreadId);
    TSK_DEBUG_INFO("stop _silence_producer_thread ok");
    producer->silenceThreadStarted = 0;
    pthread_cond_destroy(&producer->silenceThreadCond);
    pthread_mutex_destroy(&producer->silenceThreadMutex);
}

/* ============ Media Producer Interface ================= */
static int tdav_producer_audiounit_set (tmedia_producer_t *self, const tmedia_param_t *param)
{
    tdav_producer_audiounit_t *producer = (tdav_producer_audiounit_t *)self;
    if (param->plugin_type == tmedia_ppt_producer)
    {
        if (param->value_type == tmedia_pvt_int32)
        {
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MICROPHONE_MUTE))
            {
                producer->muted = TSK_TO_INT32 ((uint8_t *)param->value);
                TSK_DEBUG_INFO ("Set mic mute:%d", producer->muted);
                return tdav_audiounit_handle_mute (((tdav_producer_audiounit_t *)self)->audioUnitHandle, producer->muted);
            }
            else if (tsk_striequals (param->key, "interrupt"))
            {
                int32_t interrupt = *((uint8_t *)param->value) ? 1 : 0;
                return tdav_audiounit_handle_interrupt (producer->audioUnitHandle, interrupt);
            }
        }
    }
    return tdav_producer_audio_set (TDAV_PRODUCER_AUDIO (self), param);
}

static int tdav_producer_audiounit_get (tmedia_producer_t *self, tmedia_param_t *param)
{
    //tdav_producer_audiounit_t *producer = (tdav_producer_audiounit_t *)self;
    if (param->plugin_type == tmedia_ppt_producer)
    {
        if (param->value_type == tmedia_pvt_int32)
        {
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_RECORDING_ERROR))
            {
                int recPermission = tdav_apple_getRecordPermission();
                if (1 == recPermission) {
                    *(int32_t*)param->value = YOUME_SUCCESS;
                } else if (0 == recPermission){
                    *(int32_t*)param->value = YOUME_ERROR_REC_NO_PERMISSION;
                } else if (-1 == recPermission) {
                    *(int32_t*)param->value = YOUME_ERROR_REC_PERMISSION_UNDEFINED;
                }
                return 0;
            
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_RECORDING_ERROR_EXTRA))
            {
                *(int32_t*)param->value = 0;
                return 0;
            }
        }
    }
    
    return -1;
}

int tdav_producer_audiounit_has_input_device( )
{
#ifdef MAC_OS
    int curinput =  tdav_mac_cur_record_device();
    return curinput != 0;
#else
    return true;
#endif
}

static int tdav_producer_audiounit_prepare (tmedia_producer_t *self, const tmedia_codec_t *codec)
{
    static UInt32 flagOne = 1;
    UInt32 param;
// static UInt32 flagZero = 0;
#define kInputBus 1

    tdav_producer_audiounit_t *producer = (tdav_producer_audiounit_t *)self;
    OSStatus status = noErr;
    AudioStreamBasicDescription audioFormat;
    AudioStreamBasicDescription deviceFormat;

    if (!producer || !codec || !codec->plugin)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (!producer->audioUnitHandle)
    {
        if (!(producer->audioUnitHandle = tdav_audiounit_handle_create (TMEDIA_PRODUCER (producer)->session_id, TMEDIA_PRODUCER (producer)->audio.useHalMode )))
        {
            TSK_DEBUG_ERROR ("Failed to get audio unit instance for session with id=%lld", TMEDIA_PRODUCER (producer)->session_id);
            return -3;
        }
    }

    if (tdav_IsPlayOnlyCategory() || !tdav_producer_audiounit_has_input_device()  ) {
        TMEDIA_PRODUCER (producer)->audio.channels = TMEDIA_CODEC_CHANNELS_AUDIO_ENCODING (codec);
        TMEDIA_PRODUCER (producer)->audio.rate = TMEDIA_CODEC_RATE_ENCODING (codec);
        TMEDIA_PRODUCER (producer)->audio.ptime = TMEDIA_CODEC_PTIME_AUDIO_ENCODING (codec);
        
        TSK_DEBUG_INFO ("AudioUnit producer: channels=%d, rate=%d, ptime=%d", TMEDIA_PRODUCER (producer)->audio.channels, TMEDIA_PRODUCER (producer)->audio.rate, TMEDIA_PRODUCER (producer)->audio.ptime);
        TSK_DEBUG_INFO ("AudioUnit producer prepared with faked recording");
        return tdav_audiounit_handle_signal_producer_prepared (producer->audioUnitHandle);
    }
    
    // enable,
#if TARGET_OS_IPHONE  // mac,kAudioUnitSubType_VoiceProcessingIO默认打开input，而且不许改，所以只有ios需要设置
    status = AudioUnitSetProperty (tdav_audiounit_handle_get_instance (producer->audioUnitHandle), kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, kInputBus, &flagOne, sizeof (flagOne));
    if (status != noErr)
    {
        TSK_DEBUG_ERROR ("AudioUnitSetProperty(kAudioOutputUnitProperty_EnableIO) failed with status=%ld", (signed long)status);
        return -4;
    }
#endif //TARGET_OS_IPHONE
    
    {
        Float64  deviceSampleRate = 0 ;
#if TARGET_OS_OSX
        if( TMEDIA_PRODUCER (producer)->audio.useHalMode )
        {
            //关闭输入，打开输出
            UInt32 param = 0;
            status = AudioUnitSetProperty (tdav_audiounit_handle_get_instance (producer->audioUnitHandle), kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, 0, &param, sizeof (UInt32));
            if (status)
            {
                TSK_DEBUG_ERROR ("AudioUnitSetProperty(kAudioOutputUnitProperty_EnableIO output) failed with status=%d", (int32_t)status);
                return -4;
            }
            param = 1;
            status = AudioUnitSetProperty (tdav_audiounit_handle_get_instance (producer->audioUnitHandle), kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, kInputBus, &param, sizeof (UInt32));
            if (status)
            {
                TSK_DEBUG_ERROR ("AudioUnitSetProperty(kAudioOutputUnitProperty_EnableIO input) failed with status=%d", (int32_t)status);
                return -4;
            }
            
            //设置输出设备
//            param = sizeof (AudioDeviceID);
//            AudioDeviceID inputDeviceID;
//            status = AudioHardwareGetProperty (kAudioHardwarePropertyDefaultInputDevice, &param, &inputDeviceID);
//            if (status)
//            {
//                TSK_DEBUG_ERROR ("AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice ) failed with status=%d", (int32_t)status);
//                return -4;
//            }
            AudioDeviceID inputDeviceID = tdav_mac_cur_record_device();
            
            TSK_DEBUG_ERROR("RecordDevice: mic use Device: %d", inputDeviceID );
            status = AudioUnitSetProperty (tdav_audiounit_handle_get_instance (producer->audioUnitHandle), kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, kInputBus, &inputDeviceID, sizeof (AudioDeviceID));
            if (status)
            {
                TSK_DEBUG_ERROR ("AudioUnitSetProperty(kAudioOutputUnitProperty_CurrentDevice ) failed with status=%d", (int32_t)status);
                return -4;
            }
            
            //HAL模式，无法进行重采样，所以需要获取设备的采样率
            param = sizeof( deviceFormat );
            status = AudioUnitGetProperty (tdav_audiounit_handle_get_instance (producer->audioUnitHandle), kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, kInputBus, &deviceFormat, &param );
            if (status)
            {
                TSK_DEBUG_ERROR ("AudioUnitGetProperty(kAudioUnitProperty_SampleRate ) failed with status=%d", (int32_t)status);
                return -4;
            }
            else{
                deviceSampleRate = deviceFormat.mSampleRate;
            }
        }
        else{
//            AudioDeviceID inputDeviceID = 0;
//            UInt32 size = sizeof(AudioDeviceID);
//            status = AudioUnitGetProperty(tdav_audiounit_handle_get_instance (producer->audioUnitHandle), kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, kInputBus, &inputDeviceID, &size );
//            TSK_DEBUG_ERROR("Pinkydodo: get CurrentDevice:%d", inputDeviceID );
            
            AudioDeviceID inputDeviceID = tdav_mac_cur_record_device();
            TSK_DEBUG_INFO("RecordDevice: mic use Device: %d", inputDeviceID );
            status = AudioUnitSetProperty (tdav_audiounit_handle_get_instance (producer->audioUnitHandle), kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, kInputBus, &inputDeviceID, sizeof (AudioDeviceID));
            if (status)
            {
                TSK_DEBUG_ERROR ("AudioUnitSetProperty(kAudioOutputUnitProperty_CurrentDevice ) failed with status=%d", (int32_t)status);
                return -4;
            }
        }
#endif //TARGET_OS_OSX

        /* codec should have ptime */
        TMEDIA_PRODUCER (producer)->audio.channels = TMEDIA_CODEC_CHANNELS_AUDIO_ENCODING (codec);
        TMEDIA_PRODUCER (producer)->audio.rate = TMEDIA_CODEC_RATE_ENCODING (codec);
        TMEDIA_PRODUCER (producer)->audio.ptime = TMEDIA_CODEC_PTIME_AUDIO_ENCODING (codec);

        TSK_DEBUG_INFO ("AudioUnit producer: channels=%d, rate=%d, ptime=%d", TMEDIA_PRODUCER (producer)->audio.channels, TMEDIA_PRODUCER (producer)->audio.rate, TMEDIA_PRODUCER (producer)->audio.ptime);

        // set format
        if( deviceSampleRate == 0 )
        {
            audioFormat.mSampleRate = TMEDIA_PRODUCER (producer)->audio.rate;
        }
        else{
            audioFormat.mSampleRate = deviceSampleRate;
            producer->ring.resampleBuffer.recordSampleRate = deviceSampleRate;
            
            if( producer->ring.resampleBuffer.recordSampleRate != TMEDIA_PRODUCER (producer)->audio.rate )
            {
                producer->ring.resampleBuffer.resampler = speex_resampler_init( TMEDIA_PRODUCER (producer)->audio.channels, audioFormat.mSampleRate,
                                                                               TMEDIA_PRODUCER (producer)->audio.rate, 3, NULL);
                
            }
        }
        
        audioFormat.mFormatID = kAudioFormatLinearPCM;
        audioFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved;
        audioFormat.mChannelsPerFrame = TMEDIA_PRODUCER (producer)->audio.channels;
        audioFormat.mFramesPerPacket = 1;
        audioFormat.mBitsPerChannel = TMEDIA_PRODUCER (producer)->audio.bits_per_sample;
        audioFormat.mBytesPerPacket = audioFormat.mBitsPerChannel / 8 * audioFormat.mChannelsPerFrame;
        audioFormat.mBytesPerFrame = audioFormat.mBytesPerPacket;
        audioFormat.mReserved = 0;
        if (audioFormat.mFormatID == kAudioFormatLinearPCM && audioFormat.mChannelsPerFrame == 1)
        {
            audioFormat.mFormatFlags &= ~kLinearPCMFormatFlagIsNonInterleaved;
        }

        status = AudioUnitSetProperty (tdav_audiounit_handle_get_instance (producer->audioUnitHandle), kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, kInputBus, &audioFormat, sizeof (audioFormat));
        if (status)
        {
            TSK_DEBUG_ERROR ("AudioUnitSetProperty(kAudioUnitProperty_StreamFormat) failed with status=%ld", (signed long)status);
            return -5;
        }
        else
        {

            // configure
            if (tdav_audiounit_handle_configure (producer->audioUnitHandle, tsk_false, TMEDIA_PRODUCER (producer)->audio.ptime, &audioFormat))
            {
                TSK_DEBUG_ERROR ("tdav_audiounit_handle_set_rate(%d) failed", TMEDIA_PRODUCER (producer)->audio.rate);
                return -4;
            }

            // set callback function
            AURenderCallbackStruct callback;
            callback.inputProc = __handle_input_buffer;
            callback.inputProcRefCon = producer;
            status =
            AudioUnitSetProperty (tdav_audiounit_handle_get_instance (producer->audioUnitHandle), kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Global, kInputBus, &callback, sizeof (callback));
            if (status)
            {
                TSK_DEBUG_ERROR ("AudioUnitSetProperty(kAudioOutputUnitProperty_SetInputCallback) "
                                 "failed with status=%ld",
                                 (signed long)status);
                return -6;
            }
            else
            {
                producer->ring.chunck.size = (TMEDIA_PRODUCER (producer)->audio.ptime * audioFormat.mSampleRate * audioFormat.mBytesPerFrame) / 1000;
                // allocate our chunck buffer
                if (!(producer->ring.chunck.buffer = tsk_realloc (producer->ring.chunck.buffer, producer->ring.chunck.size)))
                {
                    TSK_DEBUG_ERROR ("Failed to allocate new buffer");
                    return -7;
                }
                
                if( TMEDIA_PRODUCER (producer)->audio.useHalMode && TMEDIA_PRODUCER (producer)->audio.rate != audioFormat.mSampleRate )
                {
                    producer->ring.resampleBuffer.size = (TMEDIA_PRODUCER (producer)->audio.ptime * TMEDIA_PRODUCER (producer)->audio.rate * audioFormat.mBytesPerFrame) / 1000;
                    // allocate our chunck buffer
                    if (!(producer->ring.resampleBuffer.buffer = tsk_realloc (producer->ring.resampleBuffer.buffer, producer->ring.resampleBuffer.size)))
                    {
                        TSK_DEBUG_ERROR ("Failed to allocate resample buffer");
                        return -7;
                    }
                }
                
                // create ringbuffer
                producer->ring.size = kRingPacketCount * producer->ring.chunck.size;
                if (!producer->ring.buffer)
                {
                    producer->ring.buffer = youme_buffer_init ((int)producer->ring.size);
                }
                else
                {
                    int ret = 0;
                    if ((ret = youme_buffer_resize (producer->ring.buffer, (int)producer->ring.size)) < 0)
                    {
                        TSK_DEBUG_ERROR ("youme_buffer_resize(%d) failed with error code=%d", (int)producer->ring.size, ret);
                        return ret;
                    }
                }
                if (!producer->ring.buffer)
                {
                    TSK_DEBUG_ERROR ("Failed to create a new ring buffer with size = %d", (int)producer->ring.size);
                    return -9;
                }
                

                {
                    uint32_t Enable  = 1;
                    uint32_t Disable = 0;
                    TSK_DEBUG_INFO("IOS communication mode set %s", tmedia_defaults_get_comm_mode_enabled() ? "ON" : "OFF");
                    if (!tmedia_defaults_get_comm_mode_enabled()) {
                        // Bypass all voice pre-processing in record mode
                        AudioUnitSetProperty(tdav_audiounit_handle_get_instance (producer->audioUnitHandle),
                                             kAUVoiceIOProperty_BypassVoiceProcessing,
                                             kAudioUnitScope_Global,
                                             kInputBus,
                                             &Enable,
                                             sizeof(Enable));
                    } else {
                        AudioUnitSetProperty(tdav_audiounit_handle_get_instance (producer->audioUnitHandle),
                                             kAUVoiceIOProperty_BypassVoiceProcessing,
                                             kAudioUnitScope_Global,
                                             kInputBus,
                                             &Disable,
                                             sizeof(Disable));
                    }
                    AudioUnitSetProperty(tdav_audiounit_handle_get_instance (producer->audioUnitHandle),
                                         kAUVoiceIOProperty_VoiceProcessingEnableAGC,
                                         kAudioUnitScope_Global,
                                         kInputBus,
                                         &Enable,
                                         sizeof(Enable));
#if TARGET_OS_IPHONE
                    AudioUnitSetProperty(tdav_audiounit_handle_get_instance (producer->audioUnitHandle),
                                         kAUVoiceIOProperty_DuckNonVoiceAudio,
                                         kAudioUnitScope_Global,
                                         kInputBus,
                                         &Disable,
                                         sizeof(Disable));
#endif //TARGET_OS_IPHONE
                }
            }
        }
    }

    TSK_DEBUG_INFO ("AudioUnit producer prepared");
    return tdav_audiounit_handle_signal_producer_prepared (producer->audioUnitHandle);
}

static int tdav_producer_audiounit_start (tmedia_producer_t *self)
{
    tdav_producer_audiounit_t *producer = (tdav_producer_audiounit_t *)self;

    if (!producer)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (producer->paused)
    {
        producer->paused = tsk_false;
        return tsk_false;
    }

    int ret;
    if (producer->started)
    {
        TSK_DEBUG_WARN ("Already started");
        return 0;
    }
    
    if (tdav_IsPlayOnlyCategory() || !tdav_producer_audiounit_has_input_device()  ) {
        producer->started = tsk_true;
        _start_silence_producer_thread(producer);
        TSK_DEBUG_INFO ("AudioUnit producer started with faked recording");
        return 0;
    }

    ret = tdav_audiounit_handle_start (producer->audioUnitHandle);
    if (ret)
    {
        //如果start失败，stop是不会清理audioUnitHandle的，所以这里要清理了
        if (producer->audioUnitHandle)
        {
            tdav_audiounit_handle_destroy (&producer->audioUnitHandle);
            producer->audioUnitHandle = NULL;
        }
        
        TSK_DEBUG_ERROR ("tdav_audiounit_handle_start failed with error code=%d", ret);
        return ret;
    }
    producer->started = tsk_true;

    // apply parameters (because could be lost when the producer is restarted -handle recreated-)
    ret = tdav_audiounit_handle_mute (producer->audioUnitHandle, producer->muted);

    TSK_DEBUG_INFO ("AudioUnit producer started");
    return 0;
}

static int tdav_producer_audiounit_pause (tmedia_producer_t *self)
{
    tdav_producer_audiounit_t *producer = (tdav_producer_audiounit_t *)self;
    if (!producer)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    producer->paused = tsk_true;
    TSK_DEBUG_INFO ("AudioUnit producer paused");
    return 0;
}

static int tdav_producer_audiounit_stop (tmedia_producer_t *self)
{
    tdav_producer_audiounit_t *producer = (tdav_producer_audiounit_t *)self;

    if (!producer)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (!producer->started)
    {
        TSK_DEBUG_INFO ("Not started");
        return 0;
    }
    else
    {
        int ret = tdav_audiounit_handle_stop (producer->audioUnitHandle);
        if (ret)
        {
            TSK_DEBUG_ERROR ("tdav_audiounit_handle_stop failed with error code=%d", ret);
            // do not return even if failed => we MUST stop the thread!
        }
//#if TARGET_OS_IPHONE
        // https://devforums.apple.com/thread/118595
        if (producer->audioUnitHandle)
        {
            tdav_audiounit_handle_destroy (&producer->audioUnitHandle);
        }
//#endif
    }
    producer->started = tsk_false;

    // This must be called after producer->started was set to false
    if (producer->silenceThreadStarted) {
        _stop_silence_producer_thread(producer);
    }
    
    TSK_DEBUG_INFO ("AudioUnit producer stoppped");
    return 0;
}


//
//	CoreAudio producer object definition
//
/* constructor */
static tsk_object_t *tdav_producer_audiounit_ctor (tsk_object_t *self, va_list *app)
{
    tdav_producer_audiounit_t *producer = self;
    if (producer)
    {
        /* init base */
        tdav_producer_audio_init (TDAV_PRODUCER_AUDIO (producer));
        /* init self */
        producer->muted = 1;
        TSK_DEBUG_INFO ("Initial mic mute:%d", producer->muted);
        
        producer->ring.resampleBuffer.size = 0;
        producer->ring.resampleBuffer.buffer = NULL;
        producer->ring.resampleBuffer.resampler = NULL;
        producer->ring.resampleBuffer.recordSampleRate = 0;
    }
    return self;
}
/* destructor */
static tsk_object_t *tdav_producer_audiounit_dtor (tsk_object_t *self)
{
    tdav_producer_audiounit_t *producer = self;
    if (producer)
    {
        // Stop the producer if not done
        if (producer->started)
        {
            tdav_producer_audiounit_stop (self);
        }

        // Free all buffers and dispose the queue
        if (producer->audioUnitHandle)
        {
            tdav_audiounit_handle_destroy (&producer->audioUnitHandle);
        }
        TSK_FREE (producer->ring.chunck.buffer);
        TSK_FREE (producer->ring.resampleBuffer.buffer );
        if (producer->ring.buffer)
        {
            youme_buffer_destroy (producer->ring.buffer);
        }
        /* deinit base */
        tdav_producer_audio_deinit (TDAV_PRODUCER_AUDIO (producer));
        
        if( producer->ring.resampleBuffer.resampler )
        {
            speex_resampler_destroy( producer->ring.resampleBuffer.resampler );
            producer->ring.resampleBuffer.resampler = NULL;
        }

        TSK_DEBUG_INFO ("*** AudioUnit Producer destroyed ***");
    }

    return self;
}
/* object definition */
static const tsk_object_def_t tdav_producer_audiounit_def_s = {
    sizeof (tdav_producer_audiounit_t),
    tdav_producer_audiounit_ctor,
    tdav_producer_audiounit_dtor,
    tdav_producer_audio_cmp,
};
/* plugin definition*/
static const tmedia_producer_plugin_def_t tdav_producer_audiounit_plugin_def_s = { &tdav_producer_audiounit_def_s,

                                                                                   tmedia_audio,
                                                                                   "Apple CoreAudio producer (AudioUnit)",

                                                                                   tdav_producer_audiounit_set,
                                                                                   tdav_producer_audiounit_get,
                                                                                   tdav_producer_audiounit_prepare,
                                                                                   tdav_producer_audiounit_start,
                                                                                   tdav_producer_audiounit_pause,
                                                                                   tdav_producer_audiounit_stop };
const tmedia_producer_plugin_def_t *tdav_producer_audiounit_plugin_def_t = &tdav_producer_audiounit_plugin_def_s;


#endif /* __APPLE__ */

