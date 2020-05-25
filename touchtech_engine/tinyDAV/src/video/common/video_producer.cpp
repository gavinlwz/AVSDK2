/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音视频通话引擎
 *
 *  当前版本:1.0
 *  作者:kenpang@youme.im
 *  日期:2017.2.23
 *  说明:对外发布接口
 ******************************************************************/

#include "ReportMessageDef.h"
#include "ReportService.h"
#include "tinydav/video/android/video_android_producer.h"
#include "tinydav/video/android/video_android.h"

#include "tinydav/video/tdav_producer_video.h"
#include "tinymedia/tmedia_defaults.h"
#include "tinydav/video/android/video_android_device_impl.h"
#include "ICameraManager.hpp"
//#include "YouMeEngineManagerForQiniu.hpp"
#include "NgnTalkManager.hpp"
#include "NgnApplication.h"
#include "YouMeVideoMixerAdapter.h"

void video_converter_mirror_left_right(void *pBuf, int width, int height)
{
    int i, j;
    uint8_t *ptr, mirror;
    if (!pBuf || !width || !height) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return;
    }
    
    ptr = (uint8_t *)pBuf;
    for (i = 0; i < height; i++) { // Y
        for (j = 0; j < width / 2; j++) {
            mirror = *(ptr + i * width + j);
            *(ptr + i * width + j) = *(ptr + i * width + width - 1 - j);
            *(ptr + i * width + width - 1 - j) = mirror;
        }
    }
    ptr += width * height;
    for (i = 0; i < height / 2; i++) { // U
        for (j = 0; j < width / 4; j++) {
            mirror = *(ptr + i * width / 2 + j);
            *(ptr + i * width / 2 + j) = *(ptr + (i + 1) * width / 2 - 1 - j);
            *(ptr + (i + 1) * width / 2 - 1 - j) = mirror;
        }
    }
    ptr += width * height / 4;
    for (i = 0; i < height / 2; i++) { // V
        for (j = 0; j < width / 4; j++) {
            mirror = *(ptr + i * width / 2 + j);
            *(ptr + i * width / 2 + j) = *(ptr + (i + 1) * width / 2 - 1 - j);
            *(ptr + (i + 1) * width / 2 - 1 - j) = mirror;
        }
    }
}

void video_converter_mirror_up_down(void *pBuf, int width, int height)
{
    int i, j;
    uint8_t *ptr, mirror;
    if (!pBuf || !width || !height) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return;
    }
    
    ptr = (uint8_t *)pBuf;
    for (i = 0; i < width; i++) { // Y
        for (j = 0; j < height / 2; j++) {
            mirror = *(ptr + j * width + i);
            *(ptr + j * width + i) = *(ptr + (height - 1 - j) * width + i);
            *(ptr + (height - 1 - j) * width + i) = mirror;
        }
    }
    ptr += width * height;
    for (i = 0; i <= width / 2; i++) { // U
        for (j = 0; j < height / 4; j++) {
            mirror = *(ptr + j * width / 2 + i);
            *(ptr + j * width / 2 + i) = *(ptr + (height / 2 - 1 - j) * width / 2 + i);
            *(ptr + (height / 2 - 1 - j) * width / 2 + i) = mirror;
        }
    }
    ptr += width * height / 4;
    for (i = 0; i <= width / 2; i++) { // V
        for (j = 0; j < height / 4; j++) {
            mirror = *(ptr + j * width / 2 + i);
            *(ptr + j * width / 2 + i) = *(ptr + (height / 2 - 1 - j) * width / 2 + i);
            *(ptr + (height / 2 - 1 - j) * width / 2 + i) = mirror;
        }
    }
}

void video_converter_scaler_down_2x(void *pBuf, int src_width, int src_height, int des_width, int des_height)
{
    int i, j;
    int ptrCnt = 0;
    uint8_t *ptr, mirror;
    if (!pBuf || !src_height || !src_width || !des_width || !des_height) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return;
    }
    if (src_width != (des_width << 1) || src_height != (des_height << 1)) {
        TSK_DEBUG_ERROR("Not down 2X scaler: src_width=%d, src_height=%d, des_width=%d, des_height=%d", src_width, src_height, des_width, des_height);
        return;
    }
    
    ptr = (uint8_t *)pBuf;
    for (i = 0; i < des_height; i++) { // Y
        for (j = 0; j < des_width; j++) {
            *((uint8_t*)pBuf + ptrCnt++) = *(ptr + (i * src_width * 2) + j * 2);
        }
    }
    ptr += src_width * src_height;
    for (i = 0; i < des_height / 2; i++) { // U
        for (j = 0; j < des_width / 2; j++) {
            *((uint8_t*)pBuf + ptrCnt++) = *(ptr + (i * src_width) + j * 2);
        }
    }
    ptr += src_width * src_height / 4;
    for (i = 0; i < des_height / 2; i++) { // V
        for (j = 0; j < des_width / 2; j++) {
            *((uint8_t*)pBuf + ptrCnt++) = *(ptr + (i * src_width) + j * 2);
        }
    }
}

int video_producer_android_handle_data_x(const struct video_producer_android_s *_self,
                                         const void *videoFrame,
                                         int nFrameSize,
                                         int nWidth,
                                         int nHeight,
                                         uint64_t nTimestamp,
                                         int fmt)
{
    // TSK_DEBUG_ERROR("video_producer_android_handle_data_x fmt:%d, ts:%llu", fmt, nTimestamp);
    if (!_self || !videoFrame) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    if (!TMEDIA_PRODUCER(_self)->enc_cb.callback_new) {
        TSK_DEBUG_WARN("No callbacknew function is registered for the producer");
        return 0;
    }
    video_producer_android_t *self  = const_cast<video_producer_android_t *>(_self);
    tdav_producer_video_t *producer = (tdav_producer_video_t *)_self;
    if (!self->buffer.ptr) {
        TSK_DEBUG_WARN("Producer copy buffer is NULL");
        return 0;
    }
    if (TSK_SAFEOBJ_MUTEX(self)) {
        tsk_safeobj_lock(self);
    }
    
    if (VIDEO_FMT_H264 == fmt || VIDEO_FMT_ENCRYPT == fmt) {
        // TSK_DEBUG_WARN("Producer send h264 frame to encode module, size[%d]", nFrameSize);

        if (TMEDIA_PRODUCER(self) && TMEDIA_PRODUCER(self)->enc_cb.callback_new) {
            // 回调到session video直接分包发送
            TMEDIA_PRODUCER(self)->enc_cb.callback_new(TMEDIA_PRODUCER(self)->enc_cb.callback_data,
                                                    videoFrame,
                                                    nFrameSize,
                                                    nTimestamp,
                                                    0,
                                                    fmt,
                                                    tsk_false);
        }

        if (TSK_SAFEOBJ_MUTEX(self)) {
          tsk_safeobj_unlock(self);
        }

        return 0;
    }

    unsigned width_main = 0, height_main = 0;
    unsigned width_child = 0, height_child = 0;
    tmedia_defaults_get_video_size(&width_main, &height_main);
    tmedia_defaults_get_video_size_child(&width_child, &height_child);
    int size_main = width_main * height_main * 3/2;
    int size_child = width_child * height_child * 3/2;
    if (size_main > self->buffer.size) {
        self->buffer.ptr = tsk_realloc(self->buffer.ptr, size_main);
        self->buffer.size = size_main;
    }
    if (size_child > self->buffer_child.size) {
        self->buffer_child.ptr = tsk_realloc(self->buffer_child.ptr, size_child);
        self->buffer_child.size = size_child;
    }
    //main
    if (nWidth != width_main ||  nHeight != height_main) {
        //scaler and crop
        ICameraManager::getInstance()->scale((uint8_t*)videoFrame,
                                             nWidth,
                                             nHeight,
                                             (uint8_t *)self->buffer.ptr,
                                             width_main,
                                             height_main,
                                             0);
    } else {
        memcpy((((uint8_t *)self->buffer.ptr) + self->buffer.index), videoFrame, nFrameSize);
    }
    
  
    //child
    if(width_child && height_child && self->buffer_child.ptr){
        //scaler and crop
        if (width_child != width_main ||  height_child != height_main) {
        ICameraManager::getInstance()->scale((uint8_t*)self->buffer.ptr,
                                             width_main,
                                             height_main,
                                             (uint8_t *)self->buffer_child.ptr,
                                             width_child,
                                             height_child,
                                             0);
        } else {
            memcpy((((uint8_t *)self->buffer_child.ptr) + self->buffer_child.index), self->buffer.ptr, size_main);
        }
    }
    
    if (TMEDIA_PRODUCER(self) && TMEDIA_PRODUCER(self)->enc_cb.callback_new)
    {
        // 回调函数
        TMEDIA_PRODUCER(self)->enc_cb.callback_new(TMEDIA_PRODUCER(self)->enc_cb.callback_data,
                                                   self->buffer.ptr,
                                                   size_main,
                                                   nTimestamp,
                                                   0,
                                                   fmt,
                                                   tsk_false);
        
        if(size_child)
            TMEDIA_PRODUCER(self)->enc_cb.callback_new(TMEDIA_PRODUCER(self)->enc_cb.callback_data,
                                                   self->buffer_child.ptr,
                                                   size_child,
                                                   nTimestamp,
                                                   1,
                                                   fmt,
                                                   tsk_false);
    }

    if (TSK_SAFEOBJ_MUTEX(self)) {
        tsk_safeobj_unlock(self);
    }
    
    return 0;
}


int video_producer_android_handle_data(const struct video_producer_android_s *_self,
                                       const void *originFrameBuffer,
                                       int nOriginFrameSize,
                                       int originWidth,
                                       int originHeight,
                                       uint64_t nTimestamp,
                                       int fmt)
{
    return video_producer_android_handle_data_x(_self, originFrameBuffer, nOriginFrameSize, originWidth, originHeight, nTimestamp, fmt);
}

int video_producer_android_handle_data_gles(const struct video_producer_android_s *_self,
                                            const void *videoFrame,
                                            int nWidth,
                                            int nHeight,
                                            int videoid,
                                            uint64_t nTimestamp)
{
    video_producer_android_t *self  = const_cast<video_producer_android_t *>(_self);
    if (TMEDIA_PRODUCER(self) && TMEDIA_PRODUCER(self)->enc_cb.callback_new)
    {
        // 回调函数
      return  TMEDIA_PRODUCER(self)->enc_cb.callback_new(TMEDIA_PRODUCER(self)->enc_cb.callback_data,
                                                   videoFrame,
                                                   0,
                                                   nTimestamp,
                                                   videoid,
                                                   VIDEO_FMT_YUV420P,
                                                   tsk_false);
    }
    return 0;
}

int video_producer_android_handle_data_new(const struct video_producer_android_s *_self,
                                           const void *videoFrame,
                                           int nFrameSize,
                                           int nWidth,
                                           int nHeight,
                                           int videoid,
                                           uint64_t nTimestamp,
                                           int fmt){
    int ret = 0;
    video_producer_android_t *self  = const_cast<video_producer_android_t *>(_self);
    if (TMEDIA_PRODUCER(self) && TMEDIA_PRODUCER(self)->enc_cb.callback_new)
    {
        if (fmt != VIDEO_FMT_YUV420P) {
            FrameImage* frame = new FrameImage(nWidth, nHeight, const_cast<void *>(videoFrame), nFrameSize, nTimestamp, fmt);
            int len = ICameraManager::getInstance()->format_transfer(frame, fmt);
            frame->len = len;
            frame->fmt = VIDEO_FMT_YUV420P;
            
            // 回调函数
            ret = TMEDIA_PRODUCER(self)->enc_cb.callback_new(TMEDIA_PRODUCER(self)->enc_cb.callback_data,
                                                             frame->data,
                                                             frame->len,
                                                             nTimestamp,
                                                             videoid,
                                                             frame->fmt,
                                                             tsk_false);
            
            delete frame;
        } else {
            ret = TMEDIA_PRODUCER(self)->enc_cb.callback_new(TMEDIA_PRODUCER(self)->enc_cb.callback_data,
                                                             videoFrame,
                                                             nFrameSize,
                                                             nTimestamp,
                                                             videoid,
                                                             fmt,
                                                             tsk_false);
        }

    }
    return ret;
}

#if 0
int video_producer_android_handle_data_new(const struct video_producer_android_s *_self,
                                           const void *videoFrame,
                                           int nFrameSize,
                                           int nWidth,
                                           int nHeight,
                                           uint64_t nTimestamp)
{
    if (!_self || !videoFrame) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    if (!TMEDIA_PRODUCER(_self)->enc_cb.callback_new) {
        TSK_DEBUG_WARN("No callbacknew function is registered for the producer");
        return 0;
    }
    
    video_producer_android_t *self  = const_cast<video_producer_android_t *>(_self);
    //tdav_producer_video_t *producer = (tdav_producer_video_t *)_self;
    
    if (!self->buffer.ptr) {
        TSK_DEBUG_WARN("Producer copy buffer is NULL");
        return 0;
    }
    if (TSK_SAFEOBJ_MUTEX(self)) {
        tsk_safeobj_lock(self);
    }
    
    int nFrameInBytes = nFrameSize;
    int width = 0, height = 0;
    if (nWidth != TMEDIA_PRODUCER(_self)->camera.width ||  nHeight != TMEDIA_PRODUCER(_self)->camera.height) {
        // Need re-size to 320 x 240
        width  = TMEDIA_PRODUCER(_self)->camera.width;
        height = TMEDIA_PRODUCER(_self)->camera.height;

        //scaler and crop
        std::shared_ptr<FrameImage> origin_frame = std::shared_ptr<FrameImage>(new FrameImage(nWidth, nHeight,(void*)videoFrame));
        std::shared_ptr<FrameImage> out_frame = ICameraManager::getInstance()->scale(origin_frame, width, height,0);
        nFrameInBytes = width * height * 3 / 2;
        memcpy((((uint8_t *)self->buffer.ptr) + self->buffer.index), out_frame->data, nFrameInBytes);

    } else {
        memcpy((((uint8_t *)self->buffer.ptr) + self->buffer.index), videoFrame, nFrameInBytes);
    }
    
    nFrameInBytes = width * height * 3 / 2;
    
    if (TMEDIA_PRODUCER(self) && TMEDIA_PRODUCER(self)->enc_cb.callback_new)
    {
        // 回调函数
        TMEDIA_PRODUCER(self)->enc_cb.callback_new(TMEDIA_PRODUCER(self)->enc_cb.callback_data,
                                                   self->buffer.ptr,
                                                   nFrameInBytes,
                                                   nTimestamp);
    }
    
    if (TSK_SAFEOBJ_MUTEX(self)) {
        tsk_safeobj_unlock(self);
    }
    
    return 0;
}

int video_producer_android_handle_data(const struct video_producer_android_s *_self,
                                       const void *originFrameBuffer,
                                       int nOriginFrameSize,
                                       int nFps,
                                       int rotationDegree,
                                       int originWidth,
                                       int originHeight)
{
    if (!_self || !originFrameBuffer)
    {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    if (!TMEDIA_PRODUCER(_self)->enc_cb.callback)
    {
        TSK_DEBUG_WARN("No callback function is registered for the producer");
        return 0;
    }
    
    video_producer_android_t *self  = const_cast<video_producer_android_t *>(_self);
    tdav_producer_video_t *producer = (tdav_producer_video_t *)_self;
    
    if (!self->buffer.ptr) {
        TSK_DEBUG_WARN("Producer copy buffer is NULL");
        return 0;
    }
    
    if (TSK_SAFEOBJ_MUTEX(self)) {
        tsk_safeobj_lock(self);
    }
    
    // 原始图像数据
    std::shared_ptr<FrameImage> origin_frame = std::shared_ptr<FrameImage>(new FrameImage(originWidth, originHeight,(void*)originFrameBuffer));
    
    // 网络发送
    if (TMEDIA_PRODUCER(self) && TMEDIA_PRODUCER(self)->enc_cb.callback)
    {
        // 进行网络发送的前处理
        int netWidth  = TMEDIA_PRODUCER(_self)->video.width;
        int netHeight = TMEDIA_PRODUCER(_self)->video.height;
        int netFrameInBytes = netWidth * netHeight * 3 / 2;
        if (netFrameInBytes != self->buffer.size) {
            self->buffer.ptr = tsk_realloc(self->buffer.ptr, netFrameInBytes);
            self->buffer.size = netFrameInBytes;
            if (!self->buffer.ptr) {
                TSK_DEBUG_ERROR("Failed to allocate buffer with size = %d", netFrameInBytes);
                self->buffer.size = 0;
                goto bail;
            }
        }
        if (originWidth != TMEDIA_PRODUCER(_self)->video.width ||  originHeight != TMEDIA_PRODUCER(_self)->video.height) {
            //scaler and crop
            std::shared_ptr<FrameImage> net_frame = ICameraManager::getInstance()->scale(origin_frame, netWidth, netHeight,0);
            memcpy((((uint8_t *)self->buffer.ptr) + self->buffer.index), net_frame->data, netFrameInBytes);
            //TSK_DEBUG_INFO("-------------- net scale Origin_frame: %d,%d  out_frame:  %d,%d", origin_frame->width,origin_frame->height, net_frame->width,net_frame->height);
        } else {
            memcpy((((uint8_t *)self->buffer.ptr) + self->buffer.index), originFrameBuffer, nOriginFrameSize);
        }
        
        // 回调至网络传输
        if(ICameraManager::getInstance()->isCaptureFrontCameraEnable()) {
            video_converter_mirror_left_right (self->buffer.ptr, netWidth, netHeight);
        }
        TMEDIA_PRODUCER(self)->enc_cb.callback(TMEDIA_PRODUCER(self)->enc_cb.callback_data,
                                               self->buffer.ptr,
                                               netFrameInBytes);
    }
    
    // 本地渲染
    if (producer->producer_callback) {
        // 进行本地渲染的前处理
        int localWidth  = TMEDIA_PRODUCER(_self)->camera.width;
        int localHeight = TMEDIA_PRODUCER(_self)->camera.height;
        int localFrameInBytes = localWidth * localHeight * 3 / 2;
        self->buffer.ptr = tsk_realloc(self->buffer.ptr, localFrameInBytes);
        self->buffer.size = localFrameInBytes;
        if (!self->buffer.ptr) {
            TSK_DEBUG_ERROR("Failed to allocate buffer with size = %d", localFrameInBytes);
            self->buffer.size = 0;
            goto bail;
        }
        if (originWidth != (TMEDIA_PRODUCER(_self)->camera.width ||  originHeight != TMEDIA_PRODUCER(_self)->camera.height)) {
            //scaler and crop
            std::shared_ptr<FrameImage> local_frame = ICameraManager::getInstance()->scale(origin_frame, localWidth, localHeight,0);
            memcpy((((uint8_t *)self->buffer.ptr) + self->buffer.index), local_frame->data, localFrameInBytes);
            //TSK_DEBUG_INFO("-------------- local scale Origin_frame: %d,%d  out_frame:  %d,%d", origin_frame->width,origin_frame->height, local_frame->width,local_frame->height);
        } else {
            memcpy((((uint8_t *)self->buffer.ptr) + self->buffer.index), originFrameBuffer, nOriginFrameSize);
        }
        // 回调至本地渲染
        producer->producer_callback((int32_t)(TMEDIA_PRODUCER(_self)->session_id), localWidth, localHeight, rotationDegree, self->buffer.size, self->buffer.ptr, 0);
    }
    
bail:
    if (TSK_SAFEOBJ_MUTEX(self)) {
        tsk_safeobj_unlock(self);
    }
    return 0;
}
#endif
/* ============ Media Producer Interface ================= */
static int video_producer_android_set(tmedia_producer_t *_self, const tmedia_param_t *param)
{
    video_producer_android_t *self = (video_producer_android_t *)_self;
    int ret = tdav_producer_video_set (TDAV_PRODUCER_VIDEO (self), param);
    
    return ret;
}

static int video_producer_android_get(tmedia_producer_t *_self, tmedia_param_t *param)
{
    video_producer_android_t *self = (video_producer_android_t *)_self;
    
    return 0;
}

static int video_producer_android_prepare(tmedia_producer_t *_self, const tmedia_codec_t *codec)
{
    TSK_DEBUG_INFO("video_producer_android_prepare..");
    
    video_producer_android_t *self = (video_producer_android_t *)_self;
    
    if (!self || !codec)
    {
        TSK_DEBUG_ERROR("Invalid parameter");
        
        return -1;
    }
    
    // create video instance
    if (!(self->videoInstHandle = video_android_instance_create(TMEDIA_PRODUCER(self)->session_id, 1)))
    {
        TSK_DEBUG_ERROR("Failed to create video instance handle");
        
        return -2;
    }
    
    // init input parameters from the codec
    TMEDIA_PRODUCER(self)->video.fps = TMEDIA_CODEC_VIDEO(codec)->in.fps;
    TMEDIA_PRODUCER(self)->video.chroma = TMEDIA_CODEC_VIDEO(codec)->in.chroma;
    
    unsigned width, height;
    tmedia_defaults_get_video_size( &width, &height);
    TMEDIA_PRODUCER(self)->video.width = width;
    TMEDIA_PRODUCER(self)->video.height = height;
    
    unsigned width_child, height_child;
    tmedia_defaults_get_video_size_child( &width_child, &height_child);
    TMEDIA_PRODUCER(self)->video_child.width = width_child;
    TMEDIA_PRODUCER(self)->video_child.height = height_child;
    
    //    if(TMEDIA_PRODUCER(self)->camera.screenOrientation == 90 || TMEDIA_PRODUCER(self)->camera.screenOrientation == 270) {
    //        TMEDIA_PRODUCER(self)->video.width = width;
    //        TMEDIA_PRODUCER(self)->video.height = height;
    //    } else {
    //        TMEDIA_PRODUCER(self)->video.width = height;
    //        TMEDIA_PRODUCER(self)->video.height = width;
    //    }
    
    
    TMEDIA_PRODUCER(self)->camera.fps = TMEDIA_CODEC_VIDEO(codec)->in.fps;
    TMEDIA_PRODUCER(self)->camera.chroma = TMEDIA_CODEC_VIDEO(codec)->in.chroma;
    
    unsigned cameraWidth = 0 ;
    unsigned cameraHeight = 0 ;
    tmedia_defaults_get_camera_size( &cameraWidth, &cameraHeight );
    
    TMEDIA_PRODUCER(self)->camera.width = cameraWidth;
    TMEDIA_PRODUCER(self)->camera.height = cameraHeight;
    
    //    TMEDIA_PRODUCER(self)->camera.width = (TMEDIA_PRODUCER(self)->camera.screenOrientation == 90 || TMEDIA_PRODUCER(self)->camera.screenOrientation == 270)?320:240;
    //    TMEDIA_PRODUCER(self)->camera.height = (TMEDIA_PRODUCER(self)->camera.screenOrientation == 90 || TMEDIA_PRODUCER(self)->camera.screenOrientation == 270)?240:320;;
    TMEDIA_PRODUCER(self)->camera.rotation = tmedia_defaults_get_camera_rotation ();
    TMEDIA_PRODUCER(self)->camera.front = tsk_true; // TODO
    if (TMEDIA_PRODUCER(self)->camera.front) {
        TMEDIA_PRODUCER(self)->camera.mirror = tsk_true;
    } else {
        TMEDIA_PRODUCER(self)->camera.mirror = tsk_false;
    }
    
    TSK_DEBUG_INFO("video_producer_android_prepare(fps=%d, chroma=%d, width=%d, height=%d, screenOrientation=%d, cameraWidth=%d, cameraHeight=%d)",
                   TMEDIA_PRODUCER(self)->video.fps, TMEDIA_PRODUCER(self)->video.chroma,
                   (int)TMEDIA_PRODUCER(self)->video.width, (int)TMEDIA_PRODUCER(self)->video.height,
                   TMEDIA_PRODUCER(self)->camera.screenOrientation, TMEDIA_PRODUCER(self)->camera.width, TMEDIA_PRODUCER(self)->camera.height);
    
    // prepare playout device and update output parameters
    int ret = video_android_instance_prepare_producer(self->videoInstHandle, &_self);
    
    if (0 == ret)
    {
        // now that the producer is prepared we can initialize internal buffer using device caps
        // 这里调用video jni采集接口
        /*
         JNI_Init_Video_Capturer(TMEDIA_PRODUCER(self)->video.fps,
         TMEDIA_PRODUCER(self)->video.width,
         TMEDIA_PRODUCER(self)->video.height,
         _self);
         */
        video_android_instance_t *instance     = NULL;
        VideoAndroidDeviceCallbackImpl *cbFunc = NULL;
        instance = (video_android_instance_t *)(self->videoInstHandle);
        if (!instance)
        {
            TSK_DEBUG_WARN("Invalid parameter(instance == null)");
            return -4;
        }
        cbFunc = (VideoAndroidDeviceCallbackImpl *)(instance->callback);
        if (!cbFunc)
        {
            TSK_DEBUG_WARN("Invalid parameter(cbFunc == null)");
            return -5;
        }

        // no need set camera size and fps again!
        // // Camera sz is possibly different with the producer sz, TODO
        // ICameraManager::getInstance()->setCaptureProperty(TMEDIA_PRODUCER(self)->camera.fps,
        //                                                   TMEDIA_PRODUCER(self)->camera.width,
        //                                                   TMEDIA_PRODUCER(self)->camera.height);
        
        {
            //report video info
            ReportService * report = ReportService::getInstance();
            youmeRTC::ReportVideoInfo videoInfo;
            videoInfo.sessionid = TMEDIA_PRODUCER(self)->session_id;
            videoInfo.width = TMEDIA_PRODUCER(self)->video.width;
            videoInfo.height = TMEDIA_PRODUCER(self)->video.height;
            videoInfo.fps = (int)TMEDIA_PRODUCER(self)->video.fps;
            videoInfo.smooth = tsk_true;
            videoInfo.delay = 0;
            videoInfo.sdk_version = SDK_NUMBER;
            videoInfo.platform = NgnApplication::getInstance()->getPlatform();
            videoInfo.canal_id = NgnApplication::getInstance()->getCanalID();
            report->report(videoInfo);
        }
        
        ICameraManager::getInstance()->registerCameraPreviewCallback(cbFunc);
        YouMeVideoMixerAdapter::getInstance()->registerCodecCallback(cbFunc);
        // allocate buffer
        // YUV420p每帧大小计算公式:width * height * 3/2(宽 * 高 * 3/2), 此buffer用于保存网络传输的数据
        int nSize = TMEDIA_PRODUCER(self)->video.width * TMEDIA_PRODUCER(self)->video.height * 3 / 2;
        TSK_DEBUG_INFO("video producer buffer nSzie = %d", nSize);
        self->buffer.ptr = tsk_realloc(self->buffer.ptr, nSize);
        if (self->buffer.ptr)
        {
            self->buffer.size = nSize;
            self->buffer.index = 0;
        }
        else{
            self->buffer.size = 0;
            TSK_DEBUG_ERROR("Failed to allocate buffer with size = %d", nSize);
        }
        
        //第二路低分辨率
        self->buffer_child.size = 0;
        self->buffer_child.ptr = tsk_null;
        /*
        int nSize_child = TMEDIA_PRODUCER(self)->video_child.width * TMEDIA_PRODUCER(self)->video_child.height * 3 / 2;
        if(nSize_child){
            self->buffer_child.ptr = tsk_realloc(self->buffer_child.ptr, nSize_child);
            if (self->buffer_child.ptr)
            {
                self->buffer_child.size = nSize_child;
                self->buffer_child.index = 0;
            }
            else{
                self->buffer_child.size  = 0;
                TSK_DEBUG_ERROR("Failed to allocate buffer with size = %d", nSize_child);
            }
        }
         */
        //本地视频
        self->buffer_local.ptr = tsk_null;
        self->buffer_local.size = 0;
        tsk_safeobj_init(self);
    }
    
    
    
    return 0;
}

static int video_producer_android_start(tmedia_producer_t *_self)
{
    video_producer_android_t *self = (video_producer_android_t *)_self;
    
    if (!self)
    {
        TSK_DEBUG_ERROR("Invalid parameter");
        
        return -1;
    }
    
    TSK_DEBUG_INFO("video_producer_android_start");
    if(TMEDIA_PRODUCER(self)->camera.needOpen) {
        video_android_instance_t *instance     = NULL;
        VideoAndroidDeviceCallbackImpl *cbFunc = NULL;
        instance = (video_android_instance_t *)(self->videoInstHandle);
        if (!instance)
        {
            TSK_DEBUG_WARN("Invalid parameter(instance == null)");
            return -4;
        }
        cbFunc = (VideoAndroidDeviceCallbackImpl *)(instance->callback);
        if (!cbFunc)
        {
            TSK_DEBUG_WARN("Invalid parameter(cbFunc == null)");
            return -5;
        }
        YouMeVideoMixerAdapter::getInstance()->registerCodecCallback((VideoAndroidDeviceCallbackImpl *)(instance->callback));
        ICameraManager::getInstance()->registerCameraPreviewCallback((VideoAndroidDeviceCallbackImpl *)(instance->callback));
        ICameraManager::getInstance()->startCapture();
    }
    
    return 0;
}

static int video_producer_android_pause(tmedia_producer_t *_self)
{
    return 0;
}

static int video_producer_android_stop(tmedia_producer_t *_self)
{
    TSK_DEBUG_INFO("video_producer_android_stop..");
    
    video_producer_android_t *self = (video_producer_android_t *)_self;
    
    if (!self)
    {
        TSK_DEBUG_ERROR("Invalid parameter");
        
        return -1;
    }
    
    // TODO:这里调用jni video采集stop接口
    video_android_instance_stop_producer(self->videoInstHandle);
    
    ICameraManager::getInstance()->stopCapture();
    ICameraManager::getInstance()->unregisterCameraPreviewCallback();
    YouMeVideoMixerAdapter::getInstance()->unregisterCodecCallback();
    return 0;
}

// video producer object definition

/* constructor */
static tsk_object_t *video_producer_android_ctor(tsk_object_t *_self, va_list *app)
{
    video_producer_android_t *self = (video_producer_android_t *)_self;
    
    if (self)
    {
        /* init base */
        tdav_producer_video_init(TDAV_PRODUCER_VIDEO(self));
        
        TSK_DEBUG_INFO("Initial video_producer_android_ctor");
    }
    
    return self;
}

/* destructor */
static tsk_object_t *video_producer_android_dtor(tsk_object_t *_self)
{
    video_producer_android_t *self = (video_producer_android_t *)_self;
    
    if (self)
    {
        /* stop */
        video_producer_android_stop(TMEDIA_PRODUCER(self));
        
        /* deinit self */
        if (self->videoInstHandle)
        {
            // TODO:
        }
        
        tsk_safeobj_lock(self);
        if(self->buffer.ptr)
        {
            TSK_FREE(self->buffer.ptr);
            self->buffer.ptr = tsk_null;
            self->buffer.size = 0;
        }
        if(self->buffer_child.ptr)
        {
            TSK_FREE(self->buffer_child.ptr);
            self->buffer_child.ptr = tsk_null;
            self->buffer_child.size = 0;
        }
        if(self->buffer_local.ptr)
        {
            TSK_FREE(self->buffer_local.ptr);
            self->buffer_local.ptr = tsk_null;
            self->buffer_local.size = 0;
        }
        tsk_safeobj_unlock(self);
        
        tsk_safeobj_deinit(self);
        
        /* deinit base */
        tdav_producer_video_deinit(TDAV_PRODUCER_VIDEO(self));
    }
    
    return self;
}

/* object definition */
static const tsk_object_def_t video_producer_android_def_s = {
    sizeof(video_producer_android_t),
    video_producer_android_ctor,
    video_producer_android_dtor,
    tdav_producer_video_cmp
    
};

/* plugin definition */
static const tmedia_producer_plugin_def_t video_producer_android_plugin_def_s =
{
    &video_producer_android_def_s,
    tmedia_video,
    "ANDROID video producer",
    
    video_producer_android_set,
    video_producer_android_get,
    video_producer_android_prepare,
    video_producer_android_start,
    video_producer_android_pause,
    video_producer_android_stop
};
const tmedia_producer_plugin_def_t *video_producer_android_plugin_def_t = &video_producer_android_plugin_def_s;
