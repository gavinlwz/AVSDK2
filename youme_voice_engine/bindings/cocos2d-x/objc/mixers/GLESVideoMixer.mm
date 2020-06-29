//
//  GLESVideoMixer.mm
//  youme_voice_engine
//
//  Created by bhb on 2017/8/29.
//  Copyright © 2017年 youme. All rights reserved.
//

#include "GLESVideoMixer.h"
#include "GLESUtil.h"
#include "filters/FilterFactory.h"

#import <Foundation/Foundation.h>
#ifdef MAC_OS
#import <AppKit/AppKit.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#import <OpenGL/gl3.h>
#import <OpenGL/gl3ext.h>
#import <QuartzCore/CAOpenGLLayer.h>
#else
#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <OpenGLES/ES3/gl.h>
#endif
#include <CoreVideo/CoreVideo.h>
#include"YouMeVideoMixerAdapter.h"
#include "YouMeConstDefine.h"
#include <libkern/OSAtomic.h>

#if defined(__APPLE__)
#define SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(v)  ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedAscending)
#define IS_UNDER_APP_EXTENSION ([[[NSBundle mainBundle] executablePath] containsString:@".appex/"])
#endif


// Convenience macro to dispatch an OpenGL ES job to the created videocore::JobQueue
#define PERF_GL(x, dispatch) do {\
m_glJobQueue.dispatch([=](){\
YMOpenGLContext* cur = [YMOpenGLContext currentContext];\
if(m_glesCtx) {\
YM_SET_OPENGL_CURRENT_CONTEXT((YMOpenGLContext*)m_glesCtx)\
}\
x ;\
YM_SET_OPENGL_CURRENT_CONTEXT(cur)\
});\
} while(0)
// Dispatch and execute synchronously
#define PERF_GL_sync(x) PERF_GL((x), enqueue_sync);
// Dispatch and execute asynchronously
#define PERF_GL_async(x) PERF_GL((x), enqueue);

#define RENDER_INT(x) (int)(x)

//static dispatch_queue_t ym_av_encode_queue = dispatch_queue_create("im.youme.avEncodeQueue", DISPATCH_QUEUE_SERIAL);

std::atomic<bool> gMixPause(false);
@interface GLESObjCCallback : NSObject
{
    videocore::GLESVideoMixer* _mixer;
}
- (void) setMixer: (videocore::GLESVideoMixer*) mixer;
@end
@implementation GLESObjCCallback
- (instancetype) init {
    if((self = [super init])) {
#if TARGET_OS_IPHONE
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notification:) name:YMApplicationDidBecomeActiveNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notification:) name:YMApplicationWillResignActiveNotification object:nil];
#endif
        //dispatch_set_target_queue(ym_av_encode_queue, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0));
    }
    return self;
}

- (void) dealloc {
#if TARGET_OS_IPHONE
    [[NSNotificationCenter defaultCenter] removeObserver:self];
#endif
    [super dealloc];
}

- (void) notification: (NSNotification*) notification {
    
    if([notification.name isEqualToString:YMApplicationDidBecomeActiveNotification]) {
        
        _mixer->mixPaused(false);
        gMixPause = false;
        
    } else if([notification.name isEqualToString:YMApplicationWillResignActiveNotification]) {
        
        _mixer->mixPaused(true);
        gMixPause = true;
        
    }
}
- (void) setMixer: (videocore::GLESVideoMixer*) mixer
{
    _mixer = mixer;
}
@end

namespace videocore {
    
    
    void  SourceBuffer::inputBuffer(Apple::PixelBufferRef ref, YMCVOpenGLTextureCacheRef textureCache, JobQueue& m_glJobQueue, void* m_glesCtx, uint32_t rotation, uint32_t mirror, uint64_t timestamp)
    {
        bool flush = false;
        auto it = m_pixelBuffers.find(ref->cvBuffer());
        const auto now = std::chrono::steady_clock::now();
        m_bufferFormat = BufferFormatType_PixelBuffer;
        if(m_currentBuffer) {
            m_currentBuffer->setState(kVCPixelBufferStateAvailable);
        }
        ref->setState(kVCPixelBufferStateAcquired);
        if(it == m_pixelBuffers.end()) {
            //  PERF_GL_async({
            if(gMixPause)
                return;
            ref->lock(true);
            OSType format = (OSType)ref->pixelFormat();
            GLsizei width = (GLsizei)ref->width();
            GLsizei height = (GLsizei)ref->height();
            if(!width || !height)
                NSLog(@"%d: input CVPixelBuffer width and height == 0!!!  \n", __LINE__);
            bool is32bit = true;
            is32bit = (format != kCVPixelFormatType_16LE565);
            YMCVOpenGLTextureRef texture = nullptr;
            
#ifdef MAC_OS
            CVReturn ret = YMCVOpenGLTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                                        textureCache,
                                                                        ref->cvBuffer(),
                                                                        NULL,
                                                                        &texture);
#else
            CVReturn ret = YMCVOpenGLTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                                        textureCache,
                                                                        ref->cvBuffer(),
                                                                        NULL,
                                                                        GL_TEXTURE_2D,
                                                                        is32bit ? GL_RGBA : GL_RGB,
                                                                        width,
                                                                        height,
                                                                        is32bit ? GL_BGRA : GL_RGB,
                                                                        is32bit ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT_5_6_5,
                                                                        0,
                                                                        &texture);
#endif
            
            
            ref->unlock(true);
            if(ret == noErr && texture) {
                GLenum target = YM_CVOpenGLTextureGetTarget(texture);
                glBindTexture(target, YMCVOpenGLTextureGetName(texture));
                glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                
                auto iit = this->m_pixelBuffers.emplace(ref->cvBuffer(), ref).first;
                iit->second.texture = texture;
                iit->second.time = now;
                this->m_timestamp = timestamp;
                this->m_currentBuffer = ref;
                this->m_currentTexture = texture;
                this->m_width = width;
                this->m_height = height;
                this->m_mirror = mirror;
                this->m_rotation = rotation;
            } else {
                NSLog(@"%d: Error creating texture! (%ld)\n", __LINE__, (long)ret);
            }
            // });
            flush = true;
        } else {
            m_currentBuffer = ref;
            m_currentTexture = it->second.texture;
            m_width = ref->width();
            m_height = ref->height();
            m_timestamp = timestamp;
            m_mirror = mirror;
            m_rotation = rotation;
            it->second.time = now;
        }
        
        //PERF_GL_async({
        //const auto currentBuffer = this->m_currentBuffer->cvBuffer();
        for ( auto it = this->m_pixelBuffers.begin() ; it != m_pixelBuffers.end() ; ) {
            
            if ( (it->second.buffer->isTemporary()) && it->second.buffer->cvBuffer() != this->m_currentBuffer->cvBuffer() ) {
                // Buffer is temporary, release it.
                it = this->m_pixelBuffers.erase(it);
            } else {
                ++ it;
            }
            
        }
        if(flush) {
            YMCVOpenGLTextureCacheFlush(textureCache, 0);
        }
        
        //  });
        
    }
    
    
    void  SourceBuffer::inputBuffer(uint8_t* buffer, uint32_t bufferlen, uint32_t width, uint32_t height, uint32_t rotation, uint32_t mirror, uint64_t timestamp, BufferFormatType fmt)
    {
        m_width = width;
        m_height = height;
        m_bufferFormat = fmt;
        m_timestamp = timestamp;
        m_mirror = mirror;
        m_rotation = rotation;
        m_dataBuffer.get()->resetBuffer(buffer, bufferlen, width, height);
    }
    
    void SourceBuffer::inputBuffer(Apple::PixelBufferRef ref,  uint32_t rotation, uint32_t mirror, uint64_t timestamp){
        CVPixelBufferRef pixelbuffer = ref->cvBuffer();
        CVPixelBufferLockBaseAddress(pixelbuffer, 0);
        size_t bufferSize = CVPixelBufferGetDataSize(pixelbuffer);
        size_t videowidth = CVPixelBufferGetWidth(pixelbuffer);
        size_t videoheight = CVPixelBufferGetHeight(pixelbuffer);
        uint8_t* data = (uint8_t*)CVPixelBufferGetBaseAddress(pixelbuffer);
        inputBuffer(data, bufferSize, videowidth, videoheight, rotation, mirror, timestamp, BufferFormatType_BGRA);
        CVPixelBufferUnlockBaseAddress(ref->cvBuffer(), 0);
    }
    
    GLESVideoMixer* GLESVideoMixer::getInstance()
    {
        static GLESVideoMixer * instance = new GLESVideoMixer();
        return instance;
    }
    
    GLESVideoMixer::GLESVideoMixer()
    :m_glesCtx(nullptr),
    m_outBkFrameW(0),
    m_outBkFrameH(0),
    m_outNetFrameW(0),
    m_outNetFrameH(0),
    m_outNetFrameSecondW(0),
    m_outNetFrameSecondH(0),
    m_exiting(false),
    m_paused(false),
    m_setupGLES(false),
    m_pixelBufferPool(nullptr),
    m_glJobQueue("com.videocore.composite"),
    m_textureCache(nullptr),
    m_isOpenBeauty(false),
    m_beautyLevel(0.0f),
    m_cameraPreviewCallback(nullptr),
    m_netFirstFilter(nullptr),
    m_netSecondFilter(nullptr),
    m_uploadFilter(nullptr),
    m_extFilter(nullptr),
    m_filteExtEnabled(false),
    m_mixSizeOutSet(false),
    m_netMainHwEncode(true),
    m_netSecondHwEncode(true),
    m_videoData(nullptr),
    m_videoDataSize(0),
    m_numGLInputTasks(0)
    {
        
        for (int i = 0; i < 6; ++i) {
            m_fbo[i] = 0;
            m_pixelBuffer[i] = nullptr;
            m_texture[i] = nullptr;
        }
        
        m_callbackSession = [[GLESObjCCallback alloc] init];
        [(GLESObjCCallback*)m_callbackSession setMixer:this];
        
        m_mixThread = std::thread([this](){ this->mixThread(); });
        PERF_GL_async({
            initGLES();
        });
    }
    
    GLESVideoMixer::~GLESVideoMixer()
    {
        m_setupGLES = false;
        m_exiting = true;
        m_mixThreadCond.notify_all();
        if(m_mixThread.joinable()) {
            m_mixThread.join();
        }
        if(m_glesCtx)
        {
            
            m_glJobQueue.enqueue_sync([=](){
                YM_SET_OPENGL_CURRENT_CONTEXT((__bridge YMOpenGLContext*)m_glesCtx)
                for (int i = 0; i < 6; ++i) {
                    if(m_texture[i]){
                        GLuint textures = YMCVOpenGLTextureGetName(m_texture[i]);
                        glDeleteTextures(1, &textures);
                        CFRelease(m_texture[i]);
                    }
                    if(m_pixelBuffer[i]){
                        CVPixelBufferRelease(m_pixelBuffer[i]);
                    }
                    if (m_fbo[i]) {
                        glDeleteFramebuffers(1,&m_fbo[i]);
                    }
                }
                m_rawFbo.uninitFBO();
                m_filterExtFbo.uninitFBO();
                m_sourceFilters.clear();
                m_sourceBuffers.clear();
                m_filterFactory.clear();
                YMCVOpenGLTextureCacheFlush(m_textureCache, 0);
                CFRelease(m_textureCache);
#ifdef MAC_OS
                [YMOpenGLContext clearCurrentContext];
#else
                [(__bridge YMOpenGLContext*)m_glesCtx release];
#endif
                
                m_glesCtx = nil;
                TSK_DEBUG_INFO("gles mixer m_glJobQueue exit!!\n");
            });
        }
        m_glJobQueue.mark_exiting();
        m_glJobQueue.enqueue_sync([](){});
        [(id)m_callbackSession release];
        
    }
    
    bool GLESVideoMixer::initGLES(){
        if(!m_glesCtx){
#ifdef MAC_OS
            NSOpenGLPixelFormatAttribute attrs[] = {
                NSOpenGLPFADoubleBuffer,
                NSOpenGLPFAAccelerated,
                NSOpenGLPFAAllowOfflineRenderers,
                0,
                0
            };
            NSOpenGLPixelFormat *pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
            if (!pf)
            {
                NSLog(@"No OpenGL pixel format");
            }
            m_glesCtx = [[NSOpenGLContext alloc] initWithFormat:pf shareContext:nil];
            
#else
//            if(SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"7.0")) {
//                m_glesCtx = [[YMOpenGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
//            }
            if(!m_glesCtx) {
                m_glesCtx = [[YMOpenGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
            }
#endif
            
            if(!m_glesCtx) {
                std::cerr << "Error! Unable to create an OpenGL ES 2.0 or 3.0 Context!" << std::endl;
                return false;
            }
            YM_SET_OPENGL_CURRENT_CONTEXT((YMOpenGLContext*)m_glesCtx)
        }
        
        if(!m_textureCache)
        {
#ifdef MAC_OS
            CVOpenGLTextureCacheCreate(kCFAllocatorDefault, NULL, ((YMOpenGLContext*)this->m_glesCtx).CGLContextObj, ((YMOpenGLContext*)this->m_glesCtx).pixelFormat.CGLPixelFormatObj, NULL, &this->m_textureCache);
#else
            CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, (YMOpenGLContext*)this->m_glesCtx, NULL, &this->m_textureCache);
#endif
        }
        m_setupGLES = true;
        return true;
    }
    
    bool GLESVideoMixer::setupGLES(int width, int height, GLESMixerType type)
    {
        if (!width || !height) {
            return false;
        }
        int index = RENDER_INT(type);
        
        @autoreleasepool {
            
            if(!m_pixelBufferPool) {
                NSDictionary* pixelBufferOptions = @{ (NSString*) kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA),
                                                      (NSString*) kCVPixelBufferWidthKey : @(width),
                                                      (NSString*) kCVPixelBufferHeightKey : @(height),
#ifdef MAC_OS
                                                      (NSString*) kCVPixelBufferOpenGLCompatibilityKey : @YES,
#else
                                                      (NSString*) kCVPixelBufferOpenGLESCompatibilityKey : @YES,
#endif
                                                      (NSString*) kCVPixelBufferIOSurfacePropertiesKey : @{}};
                

                if(!(IS_UNDER_APP_EXTENSION))
                {
                    CVPixelBufferCreate(kCFAllocatorDefault, width, height, kCVPixelFormatType_32BGRA, (CFDictionaryRef)pixelBufferOptions, &m_pixelBuffer[index]);
                    CVPixelBufferCreate(kCFAllocatorDefault, width, height, kCVPixelFormatType_32BGRA, (CFDictionaryRef)pixelBufferOptions, &m_pixelBuffer[index+1]);
                }
            }
            else {
                if(!(IS_UNDER_APP_EXTENSION))
                {
                    CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, m_pixelBufferPool, &m_pixelBuffer[index]);
                    CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, m_pixelBufferPool, &m_pixelBuffer[index+1]);
                }
            }
            
        }
        
        for(int i = index ; i < index+2 ; ++i) {
            
#ifdef MAC_OS
            YMCVOpenGLTextureCacheCreateTextureFromImage(kCFAllocatorDefault, this->m_textureCache, this->m_pixelBuffer[i], NULL, &m_texture[i]);
#else
            YMCVOpenGLTextureCacheCreateTextureFromImage(kCFAllocatorDefault, this->m_textureCache, this->m_pixelBuffer[i], NULL, GL_TEXTURE_2D, GL_RGBA, width, height, GL_BGRA, GL_UNSIGNED_BYTE, 0, &m_texture[i]);
#endif
            
            glBindTexture(GL_TEXTURE_2D, YMCVOpenGLTextureGetName(m_texture[i]));
            GLenum target = YM_CVOpenGLTextureGetTarget(m_texture[i]);
            glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glGenFramebuffers(1, &this->m_fbo[i]);
            glBindFramebuffer(GL_FRAMEBUFFER, m_fbo[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, YMCVOpenGLTextureGetName(m_texture[i]), 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            
        }
        
        GL_FRAMEBUFFER_STATUS(__LINE__);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);
        glViewport(0, 0, width ,height);
        glClearColor(0.0,0.0,0.0,1.0);
        YMCVOpenGLTextureCacheFlush(m_textureCache, 0);
        
        TSK_DEBUG_INFO("GLESVideoMixer setupGLES success!! w:%d,h:%d,type:%d\n", width, height, type);
        return true;
    }
    
    void GLESVideoMixer::unloadGLES(GLESMixerType type)
    {
        int index = RENDER_INT(type);
        if(m_glesCtx)
        {
            for (int i = index; i < index+2; ++i) {
                if(m_texture[i]){
                    //GLuint textures = YMCVOpenGLTextureGetName(m_texture[i]);
                    //glDeleteTextures(1, &textures);
                    CFRelease(m_texture[i]);
                    m_texture[i] = nullptr;
                }
                if(m_pixelBuffer[i]){
                    CVPixelBufferRelease(m_pixelBuffer[i]);
                    m_pixelBuffer[i] = nullptr;
                }
                if (m_fbo[i]) {
                    glDeleteFramebuffers(1,&m_fbo[i]);
                    m_fbo[i] = 0;
                }
            }
        }
        
    }
    
    void GLESVideoMixer::setMixVideoSize(int width, int height)
    {
        if (width < 0 || height < 0 ) {
            return;
        }
        m_outBkFrameW = width;
        m_outBkFrameH = height;
        m_mixSizeOutSet = true;
        PERF_GL_async({
            unloadGLES(GLES_MIXER_TYPE_MIX);
            setupGLES(m_outBkFrameW, m_outBkFrameH, GLES_MIXER_TYPE_MIX);
        });
    }
    
    void GLESVideoMixer::setVideoNetResolutionWidth(int width, int height)
    {
        if (width < 0 || height < 0) {
            return;
        }
        m_outNetFrameW = width;
        m_outNetFrameH = height;
        PERF_GL_async({
            unloadGLES(GLES_MIXER_TYPE_NET);
            setupGLES(m_outNetFrameW, m_outNetFrameH, GLES_MIXER_TYPE_NET);
        });
    }
    
    void GLESVideoMixer::setVideoNetResolutionWidthForSecond(int width, int height)
    {
        if (width < 0 || height < 0) {
            return;
        }
        m_outNetFrameSecondW = width;
        m_outNetFrameSecondH = height;
        PERF_GL_async({
            unloadGLES(GLES_MIXER_TYPE_NET_SECOND);
            setupGLES(m_outNetFrameSecondW, m_outNetFrameSecondH, GLES_MIXER_TYPE_NET_SECOND);
        });
    }
    
    void GLESVideoMixer::addMixOverlayVideoNew(std::string userId, int x, int y, int z, int width, int height)
    {
        TSK_DEBUG_INFO("addMixOverlayVideoNew userid:%s", userId.c_str());
        mixInfo info = {z, (GLRect){x,y,width,height}, userId};
        std::unique_lock<std::mutex> l(m_mutex);
        std::vector<mixInfo>::iterator it;
        it = m_layerList.begin();
        while(it != m_layerList.end()) {
            if((*it).userId == userId) {
                m_layerList.erase(it);
                break;
            }
            it++;
        }
        if(m_layerList.size() == 0) {
            m_layerList.push_back(info);
        } else {
            for(it = m_layerList.begin(); ; it++) {
                if(it == m_layerList.end()) {
                    m_layerList.push_back(info);
                    break;
                } else if(z < (*it).z) {
                    m_layerList.insert(it, info);
                    break;
                }
            }
        }
    }
    
    void GLESVideoMixer::addMixOverlayVideo(std::string userId, int x, int y, int z, int width, int height){
        PERF_GL_async({
            addMixOverlayVideoNew(userId, x, y, z, width, height);
        });
    }
    
    void GLESVideoMixer::releaseBuffer(std::string userId)
    {
        PERF_GL_async({
            std::unique_lock<std::mutex> l(m_mutex);
            auto itBuffer = m_sourceBuffers.find(userId);
            if(itBuffer != m_sourceBuffers.end()) {
                m_sourceBuffers.erase(itBuffer);
            }
            
            auto itFilter = m_sourceFilters.find(userId);
            if(itFilter != m_sourceFilters.end()) {
                m_sourceFilters.erase(itFilter);
                m_filterFactory.remove(itFilter->second);
            }
            
        });
    }
    void GLESVideoMixer::removeMixOverlayVideo(std::string userId)
    {
        PERF_GL_async({
            std::unique_lock<std::mutex> l(m_mutex);
            auto layerit  = m_layerList.begin();
            while(layerit != m_layerList.end()) {
                if((*layerit).userId == userId) {
                    m_layerList.erase(layerit);
                    break;
                }
                layerit++;
            }
            auto itBuffer = m_sourceBuffers.find(userId);
            if(itBuffer != m_sourceBuffers.end()) {
                m_sourceBuffers.erase(itBuffer);
            }
            
            auto itFilter = m_sourceFilters.find(userId);
            if(itFilter != m_sourceFilters.end()) {
                m_filterFactory.remove(itFilter->second);
                m_sourceFilters.erase(itFilter);
            }
            TSK_DEBUG_INFO("removeMixOverlayVideo userid:%s, size:%d\n", userId.c_str(), m_sourceBuffers.size());
        });
    }
    
    void GLESVideoMixer::removeAllOverlayVideo()
    {
        PERF_GL_async({
            std::unique_lock<std::mutex> l(m_mutex);
            m_sourceBuffers.clear();
            m_sourceFilters.clear();
            m_layerList.clear();
            m_filterFactory.clear();
            m_netFirstFilter = nullptr;
            m_netSecondFilter = nullptr;
            m_uploadFilter = nullptr;
            m_extFilter = nullptr;
            m_currentBeauty = false;
            m_localUserId.clear();
            m_rawFbo.uninitFBO();
            m_filterExtFbo.uninitFBO();
            m_mixSizeOutSet = false;
            m_outBkFrameW = 0;
            m_outBkFrameH = 0;
            TSK_DEBUG_INFO("removeAllOverlayVideo !!\n");
        });
    }
    
    void GLESVideoMixer::pushBufferForLocal(std::string userId, CVPixelBufferRef pixelBufferRef, int rotation, int mirror, uint64_t timestamp)
    {
        if(m_paused.load() || m_exiting) {
            return;
        }
        if(m_numGLInputTasks > 1)
        {
            TSK_DEBUG_INFO("mixer too slow, drop input");
            return;
        }
        
        OSAtomicIncrement64(&m_numGLInputTasks);
        Apple::PixelBufferRef inPixelBuffer = std::make_shared<Apple::PixelBuffer>(pixelBufferRef, true);
        PERF_GL_async({
            OSAtomicDecrement64(&m_numGLInputTasks);
            if(userId.compare(m_localUserId)){
                //removeMixOverlayVideo(m_localUserId);
                releaseBuffer(m_localUserId);
                m_localUserId = userId;
                TSK_DEBUG_INFO("mixer new userid:%s", m_localUserId.c_str());
            }
            std::unique_lock<std::mutex> l(m_mutex);
            
#if MAC_OS
            m_sourceBuffers[userId].inputBuffer(inPixelBuffer, rotation, mirror, timestamp);
            
#else
            m_sourceBuffers[userId].inputBuffer(inPixelBuffer, m_textureCache, m_glJobQueue, m_glesCtx, rotation, mirror, timestamp);
#endif
            m_mixThreadCond.notify_one();
        });
    }
    
    void GLESVideoMixer::pushBufferForLocal(std::string userId, const void * buffer, int bufferLen, int width, int height, int rotation, int mirror, uint64_t timestamp, BufferFormatType fmt)
    {
        if(!m_setupGLES.load() || m_paused.load() || m_exiting.load()) {
            return;
        }
        
        PERF_GL_sync({
            
            if(userId.compare(m_localUserId)){
                //removeMixOverlayVideo(m_localUserId);
                releaseBuffer(m_localUserId);
                m_localUserId = userId;
            }
            std::unique_lock<std::mutex> l(m_mutex);
            m_sourceBuffers[userId].inputBuffer((uint8_t*)buffer, bufferLen, width, height, rotation, mirror, timestamp, fmt);
            m_mixThreadCond.notify_one();
        });
    }
    
    void GLESVideoMixer::pushBufferForRemote(std::string userId, CVPixelBufferRef pixelBufferRef, uint64_t timestamp)
    {
        if(!m_setupGLES.load() || m_paused.load() || m_exiting.load()) {
            return;
        }
        if(!needMixing(userId))
            return;
        Apple::PixelBufferRef inPixelBuffer = std::make_shared<Apple::PixelBuffer>(pixelBufferRef, true);
        PERF_GL_async({
            std::unique_lock<std::mutex> l(m_mutex);
#if MAC_OS
            m_sourceBuffers[userId].inputBuffer(inPixelBuffer, 0, 0, timestamp);
            
#else
            m_sourceBuffers[userId].inputBuffer(inPixelBuffer, m_textureCache, m_glJobQueue, m_glesCtx, 0, 0, timestamp);
#endif
            //  m_sourceBuffers[userId].inputBuffer(inPixelBuffer, this->m_textureCache, m_glJobQueue, m_glesCtx, 0, 0, timestamp);
        });
    }
    
    void GLESVideoMixer::localLayerChange()
    {
        bool added = false;
        auto iTex = m_sourceBuffers.find(m_localUserId);
        if(iTex != m_sourceBuffers.end()){
            if ((!m_outBkFrameW || !m_outBkFrameH)||
                (!m_mixSizeOutSet &&
                (m_outBkFrameW != iTex->second.currentWidth() ||
                 m_outBkFrameH != iTex->second.currentHeight())))
            {
                m_outBkFrameW = iTex->second.currentWidth();
                m_outBkFrameH = iTex->second.currentHeight();
                unloadGLES(GLES_MIXER_TYPE_MIX);
                setupGLES(m_outBkFrameW, m_outBkFrameH, GLES_MIXER_TYPE_MIX);
                added = true;
                
            }
        }
        
        if(added || !needMixing(m_localUserId)){
            addMixOverlayVideoNew(m_localUserId,0,0,0,m_outBkFrameW,m_outBkFrameH);
        }
    }
    
    void  GLESVideoMixer::mixThread()
    {
        pthread_setname_np("com.videocore.mixers");
        int current_fb = 0;
        while(!m_exiting.load())
        {
            if(m_paused.load()) {
                usleep(10000);
                continue;
            }
            {
                std::unique_lock<std::mutex> l(m_mutex);
                m_mixThreadCond.wait(l);
            }
            if (m_setupGLES.load()) {
                
                PERF_GL_async({
                    if(m_paused.load())
                        return;
                    //int ret = 0;
                    glPushGroupMarkerEXT(0, "Videocore.Mix");
                    uint64_t timestamp = 0;
                    CVPixelBufferRef tmpBufferRef = nullptr;
                    CVPixelBufferRef tmpBufferRef2 = nullptr;
                    CVPixelBufferRef tmpBufferRefPreview = nullptr;
                    localLayerChange();
                    beautyChange();
                    rawUpload();
                    if(m_filteExtEnabled){
                        drawfilterExt();
                    }
                    
                    //net first
                    tmpBufferRef = netRender(GLES_MIXER_TYPE_NET, current_fb, timestamp);
                    if(tmpBufferRef && m_cameraPreviewCallback){
                        //dispatch_async(ym_av_encode_queue, ^{
                            if (m_netMainHwEncode) { // hardware encode
                                    int ret = YouMeVideoMixerAdapter::getInstance()->pushVideoPreviewFrameGLES(tmpBufferRef,
                                                                                                        CVPixelBufferGetWidth(tmpBufferRef),
                                                                                                        CVPixelBufferGetHeight(tmpBufferRef),
                                                                                                        0,
                                                                                                        timestamp);
                                    if (1 == ret) {
                                        m_netMainHwEncode = false;
                                    }
                            } else {
                                // software encode, main video stream
                                sendRawVideoToEncoder(tmpBufferRef, timestamp, 0);
                            }
                        //});
                    }
                    
                    //net second
                    tmpBufferRef2 = netRender(GLES_MIXER_TYPE_NET_SECOND, current_fb, timestamp);
                    if(tmpBufferRef2 && m_cameraPreviewCallback){
                        //dispatch_async(ym_av_encode_queue, ^{
                            if (m_netSecondHwEncode){
                                int ret = YouMeVideoMixerAdapter::getInstance()->pushVideoPreviewFrameGLES(tmpBufferRef2,
                                                                                                    CVPixelBufferGetWidth(tmpBufferRef2),
                                                                                                    CVPixelBufferGetHeight(tmpBufferRef2),
                                                                                                    1,
                                                                                                    timestamp);
                                if (1 == ret) {
                                    m_netSecondHwEncode = false;
                                }
                            } else {
                                // software encode, minor video stream
                                
                                sendRawVideoToEncoder(tmpBufferRef2, timestamp, 1);
                                
                            }
                        //});
                    }
                    
                    //mix
                    tmpBufferRefPreview = mixRender(GLES_MIXER_TYPE_MIX, current_fb, timestamp);
                    if(tmpBufferRefPreview){
                        YouMeVideoMixerAdapter::getInstance()->pushVideoFrameMixedCallbackkForIOS(tmpBufferRefPreview,
                                                                                                  CVPixelBufferGetWidth(tmpBufferRefPreview),
                                                                                                  CVPixelBufferGetHeight(tmpBufferRefPreview),
                                                                                                  VIDEO_FMT_CVPIXELBUFFER,
                                                                                                  timestamp);
                    }
                    
                    // if(ret == 1)//切换到软编码
                    // {
                    //     if (YouMeVideoMixerAdapter::getInstance()->isSupportGLES()) {
                    //         YouMeVideoMixerAdapter::getInstance()->resetMixer(false);
                    //     }
                    // }
                    
                    glPopGroupMarkerEXT();
                    //TSK_DEBUG_INFO("video output local timestamp:%lld\n", timestamp);
                });
                current_fb = !current_fb;
            }
        }
    }

    void GLESVideoMixer::sendRawVideoToEncoder(CVPixelBufferRef pixelbuffer, uint64_t& timestamp, int videoId) {

        CVPixelBufferRef videoFrame = pixelbuffer;
        YouMeErrorCode ret = YOUME_SUCCESS;
        
        do {
            CVPixelBufferLockBaseAddress(videoFrame, kCVPixelBufferLock_ReadOnly);
            uint8_t* baseAddress = (uint8_t*)CVPixelBufferGetBaseAddress(videoFrame);
            if(!baseAddress)
            {
                CVPixelBufferUnlockBaseAddress(videoFrame, kCVPixelBufferLock_ReadOnly);
                ret = YOUME_ERROR_MEMORY_OUT;
                break;
            }
            
            size_t videoWidth = CVPixelBufferGetWidth(videoFrame);
            size_t videoHeight = CVPixelBufferGetHeight(videoFrame);
            size_t bytesPerRow = CVPixelBufferGetBytesPerRow(videoFrame);
            int pixelFormat = CVPixelBufferGetPixelFormatType(videoFrame);
            if(pixelFormat != kCVPixelFormatType_32BGRA && pixelFormat != kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
                && pixelFormat != kCVPixelFormatType_420YpCbCr8BiPlanarFullRange){
                CVPixelBufferUnlockBaseAddress(videoFrame, kCVPixelBufferLock_ReadOnly);
                ret = YOUME_ERROR_INVALID_PARAM;
                break;
            }
            
            size_t bufferSize = 0;
            if (kCVPixelFormatType_32BGRA == pixelFormat) {
                bufferSize = CVPixelBufferGetDataSize(videoFrame);
            } else {
                bufferSize = videoWidth * videoHeight * 3 / 2;
            }
            
            if (m_videoDataSize < bufferSize) {
                if(m_videoData)
                    delete [] m_videoData;
                m_videoData = new uint8_t[bufferSize];
                m_videoDataSize = bufferSize;
            }
            if(!m_videoData){
                CVPixelBufferUnlockBaseAddress(videoFrame, kCVPixelBufferLock_ReadOnly);
                ret = YOUME_ERROR_MEMORY_OUT;
                break;
            }
            
            int fmt = VIDEO_FMT_BGRA32;
            if (kCVPixelFormatType_32BGRA == pixelFormat) {
                if(bytesPerRow != videoWidth*4 ||
                    bufferSize != bytesPerRow*videoHeight)
                {
                    bufferSize = videoHeight*videoWidth*4;
                    for (int i = 0; i < videoHeight; i++) {
                        memcpy(&m_videoData[i*videoWidth*4], &baseAddress[i*bytesPerRow], videoWidth*4);
                    }
                }
                else{
                    memcpy(m_videoData, baseAddress, bufferSize);
                }
                
                fmt = VIDEO_FMT_BGRA32;
            } else {
                size_t y_size = videoWidth * videoHeight;
                size_t uv_size = y_size / 2;
                
                //y plane
                uint8_t *y_Buffer = (uint8_t*) CVPixelBufferGetBaseAddressOfPlane( videoFrame , 0 );;
                int src_y_stride = CVPixelBufferGetBytesPerRowOfPlane(videoFrame, 0);
                
                if (videoWidth != src_y_stride) {
                    for (int i = 0; i < videoHeight; i++) {
                        memcpy(&m_videoData[i*videoWidth], &y_Buffer[i*src_y_stride], videoWidth);
                    }
                } else {
                    memcpy( m_videoData,   y_Buffer,  y_size );
                }
                
                //uv_plane
                uint8_t * uv_Buffer = (uint8_t*) CVPixelBufferGetBaseAddressOfPlane( videoFrame, 1 );
                int src_uv_stride = CVPixelBufferGetBytesPerRowOfPlane(videoFrame, 1);
                if (videoWidth != src_uv_stride) {
                    for (int i = 0; i < videoHeight/2; i++) {
                        memcpy(&m_videoData[i*videoWidth + y_size], &uv_Buffer[i*src_uv_stride], videoWidth);
                    }
                } else {
                    memcpy( m_videoData + y_size, uv_Buffer, uv_size );
                }
                fmt = VIDEO_FMT_NV12;
            }
            
//            __block uint64_t ts = timestamp;
            
            CVPixelBufferUnlockBaseAddress(videoFrame, kCVPixelBufferLock_ReadOnly);
            //dispatch_async(ym_av_encode_queue, ^{
                YouMeVideoMixerAdapter::getInstance()->pushVideoPreviewFrameNew(m_videoData, bufferSize, videoWidth, videoHeight, videoId, timestamp, fmt);
            //});
        } while(0);
    }

    int GLESVideoMixer::rawUpload(){
        
        //std::unique_lock<std::mutex> l(m_mutex);
        auto iTex = this->m_sourceBuffers.find(m_localUserId);
        if(iTex == this->m_sourceBuffers.end())
            return 0;
        
        SourceBuffer* source = &(iTex->second);
        if(source->currentFormatType() == BufferFormatType_PixelBuffer)
            return 0;
        
        if(m_uploadFilter &&(
                             (source->currentFormatType() == BufferFormatType_I420 &&
                              m_uploadFilter->name() != VIDEO_FILTER_NAME_YUV) ||
                             (source->currentFormatType() == BufferFormatType_BGRA &&
                              m_uploadFilter->name() != VIDEO_FILTER_NAME_TOBGRA) ||
                             (source->currentFormatType() == BufferFormatType_NV12 &&
                              m_uploadFilter->name() != VIDEO_FILTER_NAME_NV12)))
        {
            m_filterFactory.remove(m_uploadFilter);
            m_uploadFilter = nullptr;
            
        }
        
        if(!m_uploadFilter){
            if(source->currentFormatType() == BufferFormatType_I420){
                m_uploadFilter = dynamic_cast<IVideoFilter*>(m_filterFactory.filter(VIDEO_FILTER_NAME_YUV));
            }
            else if(source->currentFormatType() == BufferFormatType_BGRA){
                m_uploadFilter = dynamic_cast<IVideoFilter*>(m_filterFactory.filter(VIDEO_FILTER_NAME_TOBGRA));
            } else if (source->currentFormatType() == BufferFormatType_NV12) {
                m_uploadFilter = dynamic_cast<IVideoFilter*>(m_filterFactory.filter(VIDEO_FILTER_NAME_NV12));
            }
            else{
                return 0;
            }
        }
        if(!m_uploadFilter->initialized()) {
            m_uploadFilter->initialize();
        }
        if(!m_rawFbo.initFBO(source->currentWidth(), source->currentHeight()))
            return 0;
        
        glBindFramebuffer(GL_FRAMEBUFFER,  m_rawFbo.getFrameBuffer());
        SourceBuffer::DataBuffer_ * buff_ = source->currentDataBuffer();
        if(buff_){
            glViewport(0, 0, source->currentWidth(), source->currentHeight());
            glClearColor(0.0,0.0,0.0,1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            GLRect rc = {0,0,(int)source->currentWidth(),(int)source->currentHeight()};
            m_uploadFilter->makeMatrix(rc, source->currentWidth(), source->currentHeight(), source->currentWidth(), source->currentHeight());
            m_uploadFilter->draw(buff_->buffer, buff_->bufferLen, buff_->width, buff_->height);
            m_uploadFilter->bind();
            m_uploadFilter->unbind();
            source->setTextureId(m_rawFbo.getTexture());
            
            glFlush();
        }
        glBindFramebuffer(GL_FRAMEBUFFER,  0);
        return m_rawFbo.getTexture();
    }
    
    int GLESVideoMixer::drawfilterExt(){
        int textureId = 0;
        int mirror = 0;
        //std::unique_lock<std::mutex> l(m_mutex);
        auto iTex = this->m_sourceBuffers.find(m_localUserId);
        if(iTex == this->m_sourceBuffers.end())
            return 0;
        
        SourceBuffer* source = &(iTex->second);
        
        if(!m_extFilter){
            m_extFilter = dynamic_cast<IVideoFilter*>(m_filterFactory.filter(VIDEO_FILTER_NAME_BGRA));
        }
        if(!m_extFilter->initialized()) {
            m_extFilter->initialize();
        }
        if(!m_filterExtFbo.initFBO(source->currentWidth(), source->currentHeight()))
            return 0;
        
        glBindFramebuffer(GL_FRAMEBUFFER,  m_filterExtFbo.getFrameBuffer());
        glViewport(0, 0, source->currentWidth(), source->currentHeight());
        glClearColor(0.0,0.0,0.0,1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_2D, source->getTextureId());
        GLRect rc = {0,0,(int)source->currentWidth(),(int)source->currentHeight()};
        if (source->mirror() == YOUME_VIDEO_MIRROR_MODE_ENABLED ||
            source->mirror() == YOUME_VIDEO_MIRROR_MODE_NEAR) {
            mirror = 1;
        }
        m_extFilter->makeMatrix(rc, source->currentWidth(), source->currentHeight(), source->currentWidth(), source->currentHeight(), mirror, source->rotation());
        m_extFilter->bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        m_extFilter->unbind();
        textureId = YouMeVideoMixerAdapter::getInstance()->pushVideoRenderFilterCallback(m_filterExtFbo.getTexture(), (int)source->currentWidth(),(int)source->currentHeight(), 0, 0);
        if (textureId) {
            source->setTextureId(textureId);
            source->setMirror(YOUME_VIDEO_MIRROR_MODE_DISABLED);
            source->setRotation(0);
        }
        glBindFramebuffer(GL_FRAMEBUFFER,  0);
        return m_filterExtFbo.getTexture();
    }
    
    void GLESVideoMixer::glRender(int width, int height, SourceBuffer* source, IVideoFilter * filter, GLRect& rc, int mirror, int rotation ){
        
        if(source->currentWidth() == 0 ||  source->currentHeight() == 0){
            TSK_DEBUG_INFO("glesmixer render source error!!\n");
            return;
        }
        
        // TODO: Add blending.
        if(source->blends()) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        if(filter) {
            glBindTexture(GL_TEXTURE_2D, source->getTextureId());
            filter->makeMatrix(rc, source->currentWidth(), source->currentHeight(), width, height, mirror, rotation);
            filter->setBeautyLevel(m_beautyLevel);
            filter->imageDimensions(2.0/width, 2.0/height);
            filter->bind();
            filter->unbind();
        } else {
            TSK_DEBUG_INFO("Null texture!");
        }
        if(source->blends()) {
            glDisable(GL_BLEND);
        }
    }
    
    CVPixelBufferRef GLESVideoMixer::netRender(GLESMixerType type, int index, uint64_t& timestamp)
    {
        //std::unique_lock<std::mutex> l(m_mutex);
        int currentIndex = RENDER_INT(type) + index;
        CVPixelBufferRef tmpBufferRef = nullptr;
        IVideoFilter* currentFilter = nullptr;
        int width = 0, height = 0;
        if (GLES_MIXER_TYPE_NET == type) {
            width = m_outNetFrameW;
            height = m_outNetFrameH;
            currentFilter = m_netFirstFilter;
        }
        else if (GLES_MIXER_TYPE_NET_SECOND == type) {
            width = m_outNetFrameSecondW;
            height = m_outNetFrameSecondH;
            currentFilter = m_netSecondFilter;
        }
        if (m_fbo[currentIndex]) {
            glViewport(0, 0, width,height);
            glClearColor(0.0,0.0,0.0,1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glBindFramebuffer(GL_FRAMEBUFFER, this->m_fbo[currentIndex]);
            auto iTex = this->m_sourceBuffers.find(m_localUserId);
            if(iTex == this->m_sourceBuffers.end())
                return nullptr;
            
            if(!currentFilter)
            {
                if (GLES_MIXER_TYPE_NET == type) {
                    m_netFirstFilter = dynamic_cast<IVideoFilter*>(m_filterFactory.filter(VIDEO_FILTER_NAME_BGRA));
                    currentFilter = m_netFirstFilter;
                }
                else  if (GLES_MIXER_TYPE_NET_SECOND == type) {
                    m_netSecondFilter = dynamic_cast<IVideoFilter*>(m_filterFactory.filter(VIDEO_FILTER_NAME_BGRA));
                    currentFilter = m_netSecondFilter;
                }
            }
            if(currentFilter && !currentFilter->initialized()) {
                currentFilter->initialize();
            }
            GLRect rc = {0,0,width,height};
            int mirror = 0;
            if ((iTex->second).mirror() == YOUME_VIDEO_MIRROR_MODE_ENABLED ||
                (iTex->second).mirror() == YOUME_VIDEO_MIRROR_MODE_FAR) {
                mirror = 1;
            }
            glRender(width, height, &(iTex->second), currentFilter, rc, mirror, (iTex->second).rotation());
            glFlush();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            tmpBufferRef = m_pixelBuffer[currentIndex];
            timestamp = iTex->second.getTimestamp();
        }
        return tmpBufferRef;
    }
    
    
    CVPixelBufferRef GLESVideoMixer::mixRender(GLESMixerType type, int index, uint64_t& timestamp)
    {
        //std::unique_lock<std::mutex> l(m_mutex);
        CVPixelBufferRef tmpBufferRef = nullptr;
        std::vector<mixInfo>::iterator it;
        int currentIndex = RENDER_INT(type) + index;
    resmix:
        int width = m_outBkFrameW;
        int height = m_outBkFrameH;
        IVideoFilter* currentFilter = nullptr;
        if (m_fbo[currentIndex] && m_layerList.size() && width && height) {
            glViewport(0, 0, width,height);
            glClearColor(0.0,0.0,0.0,1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glBindFramebuffer(GL_FRAMEBUFFER, this->m_fbo[currentIndex]);
            for (it = m_layerList.begin(); it != m_layerList.end(); ++it) {
                std::string userid = (*it).userId;
                auto filter = m_sourceFilters.find(userid);
                if(filter == m_sourceFilters.end()) {
                    if(m_currentBeauty)
                        currentFilter = dynamic_cast<IVideoFilter*>(m_filterFactory.filter(VIDEO_FILTER_NAME_BEAUTY));
                    else
                        currentFilter = dynamic_cast<IVideoFilter*>(m_filterFactory.filter(VIDEO_FILTER_NAME_BGRA));
                    m_sourceFilters[userid] = currentFilter;
                }
                else{
                    currentFilter = filter->second;
                }
                if(currentFilter && !currentFilter->initialized()) {
                    currentFilter->initialize();
                }
                auto iTex = this->m_sourceBuffers.find(userid);
                if(iTex == this->m_sourceBuffers.end()) continue;
                int mirror = 0;
                if ((iTex->second).mirror() == YOUME_VIDEO_MIRROR_MODE_ENABLED ||
                    (iTex->second).mirror() == YOUME_VIDEO_MIRROR_MODE_NEAR) {
                    mirror = 1;
                }
                glRender(width, height, &(iTex->second), currentFilter, (*it).rect, mirror, (iTex->second).rotation());
                if (userid == m_localUserId) {
                    timestamp = iTex->second.getTimestamp();
                    iTex->second.setTextureId(0);
                }
                
            }
            glFlush();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            tmpBufferRef = m_pixelBuffer[currentIndex];
        }
        
        return tmpBufferRef;
    }
    
    void GLESVideoMixer::beautyChange()
    {
        //std::unique_lock<std::mutex> l(m_mutex);
        if (m_currentBeauty != m_isOpenBeauty) {
            auto it = m_sourceFilters.find(m_localUserId);
            if (it == m_sourceFilters.end()) {
                m_filterFactory.remove(m_sourceFilters[m_localUserId]);
            }
            if(m_netFirstFilter)
                m_filterFactory.remove(m_netFirstFilter);
            if(m_netSecondFilter)
                m_filterFactory.remove(m_netSecondFilter);
            if (m_isOpenBeauty) {
                IFilter* filter = m_filterFactory.filter(VIDEO_FILTER_NAME_BEAUTY);
                m_sourceFilters[m_localUserId] = dynamic_cast<IVideoFilter*>(filter);
                m_netFirstFilter = dynamic_cast<IVideoFilter*>(m_filterFactory.filter(VIDEO_FILTER_NAME_BEAUTY));
                m_netSecondFilter = dynamic_cast<IVideoFilter*>(m_filterFactory.filter(VIDEO_FILTER_NAME_BEAUTY));
            }
            else{
                IFilter* filter = m_filterFactory.filter(VIDEO_FILTER_NAME_BGRA);
                m_sourceFilters[m_localUserId] = dynamic_cast<IVideoFilter*>(filter);
                m_netFirstFilter = dynamic_cast<IVideoFilter*>(m_filterFactory.filter(VIDEO_FILTER_NAME_BGRA));
                m_netSecondFilter = dynamic_cast<IVideoFilter*>(m_filterFactory.filter(VIDEO_FILTER_NAME_BGRA));
            }
            
            m_currentBeauty = m_isOpenBeauty;
        }
    }
    
    void GLESVideoMixer::mixPaused(bool paused)
    {
        m_paused = paused;
    }
    
    bool GLESVideoMixer::needMixing(std::string userId)
    {
        //std::unique_lock<std::mutex> l(m_mutex);
        auto layerit  = m_layerList.begin();
        while(layerit != m_layerList.end()) {
            if((*layerit).userId == userId) {
                return true;
            }
            layerit++;
        }
        return false;
    }
    
    void GLESVideoMixer::setSourceFilter(std::string userId, IVideoFilter *filter) {
        m_sourceFilters[userId] = filter;
    }
    
    void GLESVideoMixer::openBeautify()
    {
        m_isOpenBeauty = true;
    }
    
    void GLESVideoMixer::closeBeautify()
    {
        m_isOpenBeauty = false;
    }
    
    void GLESVideoMixer::setBeautifyLevel(float level)
    {
        m_beautyLevel = level;
    }
    
    void GLESVideoMixer::registerCodecCallback(CameraPreviewCallback *cb)
    {
        std::unique_lock<std::mutex> l(m_mutex);
        m_cameraPreviewCallback = cb;
    }
    
    void GLESVideoMixer::unregisterCodecCallback()
    {
        std::unique_lock<std::mutex> l(m_mutex);
        m_cameraPreviewCallback = NULL;
    }
    
    bool GLESVideoMixer::setExternalFilterEnabled(bool enabled)
    {
        m_filteExtEnabled = enabled;
        return true;
    }
}

