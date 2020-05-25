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
#include "tinydav/audio/coreaudio/tdav_consumer_audiounit.h"

// http://developer.apple.com/library/ios/#documentation/MusicAudio/Conceptual/AudioUnitHostingGuide_iOS/Introduction/Introduction.html%23//apple_ref/doc/uid/TP40009492-CH1-SW1
// Resampler: http://developer.apple.com/library/mac/#technotes/tn2097/_index.html

#ifdef __APPLE__

#undef DISABLE_JITTER_BUFFER
#define DISABLE_JITTER_BUFFER 0

#include "tsk_debug.h"
#include "tsk_memory.h"
#include "tsk_string.h"
#include "youme_buffer.h"
#include "tinydav/tdav_apple.h"

#include <MediaToolbox/MediaToolbox.h>
#include <AudioToolbox/AudioToolbox.h>

#define kNoDataError -1
#define kRingPacketCount +10

static tsk_size_t tdav_consumer_audiounit_get (tdav_consumer_audiounit_t *self, void *data, tsk_size_t size);

static OSStatus __handle_output_buffer (void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
    OSStatus status = noErr;
    // tsk_size_t out_size;
    tdav_consumer_audiounit_t *consumer = (tdav_consumer_audiounit_t *)inRefCon;
    
    tdav_apple_monitor_add(1);

    if (!consumer->started || consumer->paused)
    {
        for (int i = 0; i < ioData->mNumberBuffers; i++)
        {
            memset( ioData->mBuffers[i].mData, 0, ioData->mBuffers[i].mDataByteSize );
        }
        goto done;
    }

    if (!ioData)
    {
        TSK_DEBUG_ERROR ("Invalid argument");
        status = kNoDataError;
        goto done;
    }
    // read from jitter buffer and fill ioData buffers
    tsk_mutex_lock (consumer->ring.mutex);
    for (int i = 0; i < ioData->mNumberBuffers; i++)
    {
        int ret = tdav_consumer_audiounit_get (consumer, ioData->mBuffers[i].mData, ioData->mBuffers[i].mDataByteSize);
        if( ret == 0 )
        {
            memset( ioData->mBuffers[i].mData, 0, ioData->mBuffers[i].mDataByteSize );
        }
    }
    tsk_mutex_unlock (consumer->ring.mutex);

done:
    return status;
}

static tsk_size_t tdav_consumer_audiounit_get (tdav_consumer_audiounit_t *self, void *data, tsk_size_t size)
{
    tsk_ssize_t retSize = 0;

    tsk_size_t readSize = size;
    uint8_t bytes_per_sample = 2;
    
#ifdef  MAC_OS
    //mac用float，而来源都是int16_t的。所以读取的时候，/2才是int16格式所对应的大小
    readSize = readSize/2;
    int16_t *tmpBuf = (int16_t *)tsk_malloc( readSize );
#else
    readSize = size;
    int16_t* tmpBuf = data;
#endif
    
    
    
#if DISABLE_JITTER_BUFFER
    retSize = speex_buffer_read (self->ring.buffer, tmpBuf, readSize);
    if (retSize < readSize)
    {
        memset (((uint8_t *)tmpBuf) + retSize, 0, (readSize - retSize));
    }
#else
    while (self->ring.leftBytes >= self->ring.chunck.size)
    {
        retSize = (tsk_ssize_t)tdav_consumer_audio_get (TDAV_CONSUMER_AUDIO (self), self->ring.chunck.buffer, self->ring.chunck.size);
        tdav_consumer_audio_tick (TDAV_CONSUMER_AUDIO (self));
        youme_buffer_write (self->ring.buffer, self->ring.chunck.buffer, retSize);
        
        self->ring.leftBytes -= retSize;
    }
    // IMPORTANT: looks like there is a bug in speex: continously trying to read more than avail
    // many times can corrupt the buffer. At least on OS X 1.5
    if (youme_buffer_get_available (self->ring.buffer) >= readSize)
    {
        retSize = (tsk_ssize_t)youme_buffer_read (self->ring.buffer, tmpBuf, (int)readSize);
        self->ring.leftBytes += retSize;
    }
    else
    {
        memset (tmpBuf, 0, readSize);
        retSize = 0;
    }

#endif
    
#ifdef MAC_OS
    tdav_codec_int16_to_float (tmpBuf, data , &(bytes_per_sample), &(readSize), 1);
    TSK_SAFE_FREE(tmpBuf);

    retSize = retSize * 2;  //ret是以int16来算的，转换成float之后，占用2倍的byte
#endif
    
    return retSize;
}

/* ============ Media Consumer Interface ================= */
int tdav_consumer_audiounit_set_param (tmedia_consumer_t *self, const tmedia_param_t *param)
{
    tdav_consumer_audiounit_t *consumer = (tdav_consumer_audiounit_t *)self;
    if (param->plugin_type == tmedia_ppt_consumer)
    {
        if (param->value_type == tmedia_pvt_int32)
        {
            if (tsk_striequals (param->key, "interrupt"))
            {
                int32_t interrupt = *((uint8_t *)param->value) ? 1 : 0;
                return tdav_audiounit_handle_interrupt (consumer->audioUnitHandle, interrupt);
            }
        }
    }
    return tdav_consumer_audio_set_param (TDAV_CONSUMER_AUDIO (self), param);
}

int tdav_consumer_audiounit_get_param (tmedia_consumer_t *self, tmedia_param_t *param)
{
    return tdav_consumer_audio_get_param (TDAV_CONSUMER_AUDIO (self), param);
}

static int tdav_consumer_audiounit_prepare (tmedia_consumer_t *self, const tmedia_codec_t *codec)
{
    static UInt32 flagOne = 1;
    AudioStreamBasicDescription audioFormat;
    AudioStreamBasicDescription deviceFormat;
#define kOutputBus 0

    tdav_consumer_audiounit_t *consumer = (tdav_consumer_audiounit_t *)self;
    OSStatus status = noErr;

    if (!consumer || !codec || !codec->plugin)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (!consumer->audioUnitHandle)
    {
        if (!(consumer->audioUnitHandle = tdav_audiounit_handle_create (TMEDIA_CONSUMER (consumer)->session_id, TMEDIA_CONSUMER (consumer)->audio.useHalMode )))
        {
            TSK_DEBUG_ERROR ("Failed to get audio unit instance for session with id=%lld", TMEDIA_CONSUMER (consumer)->session_id);
            return -3;
        }
    }

    // enable
#if TARGET_OS_IPHONE
    status = AudioUnitSetProperty (tdav_audiounit_handle_get_instance (consumer->audioUnitHandle), kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, kOutputBus, &flagOne, sizeof (flagOne));
    if (status)
    {
        TSK_DEBUG_ERROR ("AudioUnitSetProperty(kAudioOutputUnitProperty_EnableIO) failed with status=%d", (int32_t)status);
        return -4;
    }
    
#endif //TARGET_OS_IPHONE
    
    Float64  deviceSampleRate = 0 ;
    //mac才有hal模式
#if TARGET_OS_OSX
    if( TMEDIA_CONSUMER (consumer)->audio.useHalMode )
    {

        //关闭输入，打开输出
        UInt32 param = 0;
        status = AudioUnitSetProperty (tdav_audiounit_handle_get_instance (consumer->audioUnitHandle), kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 1, &param, sizeof (UInt32));
        if (status)
        {
            TSK_DEBUG_ERROR ("AudioUnitSetProperty(kAudioOutputUnitProperty_EnableIO input) failed with status=%d", (int32_t)status);
            return -4;
        }
        param = 1;
        status = AudioUnitSetProperty (tdav_audiounit_handle_get_instance (consumer->audioUnitHandle), kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, kOutputBus, &param, sizeof (UInt32));
        if (status)
        {
            TSK_DEBUG_ERROR ("AudioUnitSetProperty(kAudioOutputUnitProperty_EnableIO output) failed with status=%d", (int32_t)status);
            return -4;
        }
        /*

        
        //设置输出设备
        param = sizeof (AudioDeviceID);
        AudioDeviceID outputDeviceID;
        status = AudioHardwareGetProperty (kAudioHardwarePropertyDefaultOutputDevice, &param, &outputDeviceID);
        if (status)
        {
            TSK_DEBUG_ERROR ("AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice ) failed with status=%d", (int32_t)status);
            return -4;
        }
        
        status = AudioUnitSetProperty (tdav_audiounit_handle_get_instance (consumer->audioUnitHandle), kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, kOutputBus, &outputDeviceID, sizeof (AudioDeviceID));
        if (status)
        {
            TSK_DEBUG_ERROR ("AudioUnitSetProperty(kAudioHardwarePropertyDefaultInputDevice ) failed with status=%d", (int32_t)status);
            return -4;
        }
        */
        //HAL模式，无法进行重采样，所以需要获取设备的采样率
        
//        param = sizeof( deviceFormat );
//        status = AudioUnitGetProperty (tdav_audiounit_handle_get_instance (consumer->audioUnitHandle), kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, kOutputBus, &deviceFormat, &param );
//        if (status)
//        {
//            TSK_DEBUG_ERROR ("AudioUnitGetProperty(kAudioUnitProperty_SampleRate ) failed with status=%d", (int32_t)status);
//            return -4;
//        }
//        else{
//            deviceSampleRate = deviceFormat.mSampleRate;
//        }
    }
#endif  //TARGET_OS_OSX

    {
        TMEDIA_CONSUMER (consumer)->audio.ptime = TMEDIA_CODEC_PTIME_AUDIO_DECODING (codec);
        TMEDIA_CONSUMER (consumer)->audio.in.channels = TMEDIA_CODEC_CHANNELS_AUDIO_DECODING (codec);

        TSK_DEBUG_INFO ("AudioUnit consumer: in.channels=%d, out.channles=%d, in.rate=%d, out.rate=%d, ptime=%d", TMEDIA_CONSUMER (consumer)->audio.in.channels,
                        TMEDIA_CONSUMER (consumer)->audio.out.channels, TMEDIA_CONSUMER (consumer)->audio.in.rate, TMEDIA_CONSUMER (consumer)->audio.out.rate, TMEDIA_CONSUMER (consumer)->audio.ptime);

        audioFormat.mSampleRate = TMEDIA_CONSUMER (consumer)->audio.out.rate;
//        if( deviceSampleRate == 0 )
//        {
//            audioFormat.mSampleRate = TMEDIA_CONSUMER (consumer)->audio.out.rate;
//        }
//        else{
//            audioFormat.mSampleRate = deviceSampleRate;
//        }
        
        audioFormat.mFormatID = kAudioFormatLinearPCM;
        
        //kAudioFormatFlagIsSignedInteger 在macos 下播不出来，不知道啥原因
#if TARGET_OS_OSX
        audioFormat.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
#else
        audioFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
#endif
        audioFormat.mChannelsPerFrame = TMEDIA_CONSUMER (consumer)->audio.in.channels;
        audioFormat.mFramesPerPacket = 1;
        audioFormat.mBitsPerChannel = TMEDIA_CONSUMER (consumer)->audio.bits_per_sample;
        audioFormat.mBytesPerPacket = audioFormat.mBitsPerChannel / 8 * audioFormat.mChannelsPerFrame;
        audioFormat.mBytesPerFrame = audioFormat.mBytesPerPacket;
        audioFormat.mReserved = 0;
        status = AudioUnitSetProperty (tdav_audiounit_handle_get_instance (consumer->audioUnitHandle), kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, kOutputBus, &audioFormat, sizeof (audioFormat));

        if (status)
        {
            TSK_DEBUG_ERROR ("AudioUnitSetProperty(kAudioUnitProperty_StreamFormat) failed with status=%ld", (signed long)status);
            return -5;
        }
        else
        {
            // configure
            if (tdav_audiounit_handle_configure (consumer->audioUnitHandle, tsk_true, TMEDIA_CONSUMER (consumer)->audio.ptime, &audioFormat))
            {
                TSK_DEBUG_ERROR ("tdav_audiounit_handle_set_rate(%d) failed", TMEDIA_CONSUMER (consumer)->audio.out.rate);
                return -4;
            }

            // set callback function
            AURenderCallbackStruct callback;
            callback.inputProc = __handle_output_buffer;
            callback.inputProcRefCon = consumer;
            status = AudioUnitSetProperty (tdav_audiounit_handle_get_instance (consumer->audioUnitHandle), kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, kOutputBus, &callback, sizeof (callback));
            if (status)
            {
                TSK_DEBUG_ERROR ("AudioUnitSetProperty(kAudioOutputUnitProperty_SetInputCallback) failed with status=%ld", (signed long)status);
                return -6;
            }
        }
    }

    // allocate the chunck buffer and create the ring
#ifdef MAC_OS
    //mac是用的float格式的，大小要大一部分。但是原始数据是16位的
    consumer->ring.chunck.size = (TMEDIA_CONSUMER (consumer)->audio.ptime * audioFormat.mSampleRate * audioFormat.mBytesPerFrame / 2) / 1000;
#else
    consumer->ring.chunck.size = (TMEDIA_CONSUMER (consumer)->audio.ptime * audioFormat.mSampleRate * audioFormat.mBytesPerFrame) / 1000;
#endif
    consumer->ring.size = kRingPacketCount * consumer->ring.chunck.size;
    if (!(consumer->ring.chunck.buffer = tsk_realloc (consumer->ring.chunck.buffer, consumer->ring.chunck.size)))
    {
        TSK_DEBUG_ERROR ("Failed to allocate new buffer");
        return -7;
    }
    if (!consumer->ring.buffer)
    {
        consumer->ring.buffer = youme_buffer_init ((int)consumer->ring.size);
        consumer->ring.leftBytes = consumer->ring.size;
    }
    else
    {
        int ret;
        if ((ret = (int)youme_buffer_resize (consumer->ring.buffer, (int)consumer->ring.size)) < 0)
        {
            TSK_DEBUG_ERROR ("speex_buffer_resize(%d) failed with error code=%d", (int)consumer->ring.size, ret);
            return ret;
        }
    }
    if (!consumer->ring.buffer)
    {
        TSK_DEBUG_ERROR ("Failed to create a new ring buffer with size = %d", (int)consumer->ring.size);
        return -8;
    }
    if (!consumer->ring.mutex && !(consumer->ring.mutex = tsk_mutex_create_2 (tsk_false)))
    {
        TSK_DEBUG_ERROR ("Failed to create mutex");
        return -9;
    }

     //set maximum frames per slice as buffer size
//     UInt32 numFrames = (UInt32)consumer->ring.chunck.size;
//     status = AudioUnitSetProperty(tdav_audiounit_handle_get_instance(consumer->audioUnitHandle),
//                                  kAudioUnitProperty_MaximumFramesPerSlice,
//                                  kAudioUnitScope_Global,
//                                  0,
//                                  &numFrames,
//                                  sizeof(numFrames));
//     if(status){
//        TSK_DEBUG_ERROR("AudioUnitSetProperty(kAudioUnitProperty_MaximumFramesPerSlice, %u) failed with status=%d", (unsigned)numFrames, (int32_t)status);
//        return -6;
//    }

    TSK_DEBUG_INFO ("AudioUnit consumer prepared");
    return tdav_audiounit_handle_signal_consumer_prepared (consumer->audioUnitHandle);
}

static int tdav_consumer_audiounit_start (tmedia_consumer_t *self)
{
    tdav_consumer_audiounit_t *consumer = (tdav_consumer_audiounit_t *)self;

    if (!consumer)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (consumer->paused)
    {
        consumer->paused = tsk_false;
    }
    if (consumer->started)
    {
        TSK_DEBUG_WARN ("Already started");
        return 0;
    }
    else
    {
        int ret = tdav_audiounit_handle_start (consumer->audioUnitHandle);
        if (ret)
        {
            //如果start失败，stop是不会清理audioUnitHandle的，所以这里要清理了
            if (consumer->audioUnitHandle)
            {
                tdav_audiounit_handle_destroy (&consumer->audioUnitHandle);
                consumer->audioUnitHandle = NULL;
            }
            TSK_DEBUG_ERROR ("tdav_audiounit_handle_start failed with error code=%d", ret);
            return ret;
        }
    }
    consumer->started = tsk_true;
    TSK_DEBUG_INFO ("AudioUnit consumer started");
    return 0;
}

static int tdav_consumer_audiounit_consume (tmedia_consumer_t *self, const void *buffer, tsk_size_t size, tsk_object_t *proto_hdr)
{
    tdav_consumer_audiounit_t *consumer = (tdav_consumer_audiounit_t *)self;
    if (!consumer || !buffer || !size || !proto_hdr)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
#if DISABLE_JITTER_BUFFER
    {
        if (consumer->ring.buffer)
        {
            tsk_mutex_lock (consumer->ring.mutex);
            speex_buffer_write (consumer->ring.buffer, (void *)buffer, size);
            tsk_mutex_unlock (consumer->ring.mutex);
            return 0;
        }
        return -2;
    }
#else
    {
        return tdav_consumer_audio_put (TDAV_CONSUMER_AUDIO (consumer), buffer, size, proto_hdr);
    }
#endif
}

static int tdav_consumer_audiounit_pause (tmedia_consumer_t *self)
{
    tdav_consumer_audiounit_t *consumer = (tdav_consumer_audiounit_t *)self;
    if (!consumer)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    consumer->paused = tsk_true;
    TSK_DEBUG_INFO ("AudioUnit consumer paused");
    return 0;
}

static int tdav_consumer_audiounit_stop (tmedia_consumer_t *self)
{
    tdav_consumer_audiounit_t *consumer = (tdav_consumer_audiounit_t *)self;

    if (!consumer)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (!consumer->started)
    {
        TSK_DEBUG_INFO ("Not started");
        return 0;
    }
    else
    {
        int ret = tdav_audiounit_handle_stop (consumer->audioUnitHandle);
        if (ret)
        {
            TSK_DEBUG_ERROR ("tdav_audiounit_handle_stop failed with error code=%d", ret);
            return ret;
        }
    }
//#if TARGET_OS_IPHONE
    // https://devforums.apple.com/thread/118595
    if (consumer->audioUnitHandle)
    {
        tdav_audiounit_handle_destroy (&consumer->audioUnitHandle);
    }
//#endif

    consumer->started = tsk_false;
    TSK_DEBUG_INFO ("AudioUnit consumer stoppped");
    return 0;
}

//
//	coreaudio consumer (AudioUnit) object definition
//
/* constructor */
static tsk_object_t *tdav_consumer_audiounit_ctor (tsk_object_t *self, va_list *app)
{
    tdav_consumer_audiounit_t *consumer = self;
    if (consumer)
    {
        /* init base */
        tdav_consumer_audio_init (TDAV_CONSUMER_AUDIO (consumer));
        /* init self */
    }
    return self;
}
/* destructor */
static tsk_object_t *tdav_consumer_audiounit_dtor (tsk_object_t *self)
{
    tdav_consumer_audiounit_t *consumer = self;
    if (consumer)
    {
        /* deinit self */
        // Stop the consumer if not done
        if (consumer->started)
        {
            tdav_consumer_audiounit_stop (self);
        }
        // destroy handle
        if (consumer->audioUnitHandle)
        {
            tdav_audiounit_handle_destroy (&consumer->audioUnitHandle);
        }
        TSK_FREE (consumer->ring.chunck.buffer);
        if (consumer->ring.buffer)
        {
            youme_buffer_destroy (consumer->ring.buffer);
        }
        if (consumer->ring.mutex)
        {
            tsk_mutex_destroy (&consumer->ring.mutex);
        }

        /* deinit base */
        tdav_consumer_audio_deinit (TDAV_CONSUMER_AUDIO (consumer));
        TSK_DEBUG_INFO ("*** AudioUnit Consumer destroyed ***");
    }

    return self;
}

/* object definition */
static const tsk_object_def_t tdav_consumer_audiounit_def_s = {
    sizeof (tdav_consumer_audiounit_t),
    tdav_consumer_audiounit_ctor,
    tdav_consumer_audiounit_dtor,
    tdav_consumer_audio_cmp,
};

/* plugin definition*/
static const tmedia_consumer_plugin_def_t tdav_consumer_audiounit_plugin_def_s = { &tdav_consumer_audiounit_def_s,

                                                                                   tmedia_audio,
                                                                                   "Apple CoreAudio consumer(AudioUnit)",

                                                                                   tdav_consumer_audiounit_set_param,
                                                                                   tdav_consumer_audiounit_get_param,
                                                                                   tdav_consumer_audiounit_prepare,
                                                                                   tdav_consumer_audiounit_start,
                                                                                   tdav_consumer_audiounit_consume,
                                                                                   tdav_consumer_audiounit_pause,
                                                                                   tdav_consumer_audiounit_stop };

const tmedia_consumer_plugin_def_t *tdav_consumer_audiounit_plugin_def_t = &tdav_consumer_audiounit_plugin_def_s;

#endif /* __APPLE__ */

