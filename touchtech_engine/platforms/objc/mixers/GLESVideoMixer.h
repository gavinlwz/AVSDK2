//
//  GLESVideoMixer.h
//  youme_voice_engine
//
//  Created by bhb on 2017/8/29.
//  Copyright © 2017年 youme. All rights reserved.
//

#ifndef __videocore__GLESVideoMixer__
#define __videocore__GLESVideoMixer__

#include "config.h"
#include <iostream>
#include <map>
#include <thread>
#include <mutex>
#include <vector>
#include <map>
#include <list>
#include <unordered_map>
//#include <CoreVideo/CoreVideo.h>
//#include <GLKit/GLKit.h>
#include "PixelBuffer.h"
#include "IYouMeVideoMixer.h"
#include "JobQueue.h"
#include "filters/IVideoFilter.h"
#include "filters/FilterFactory.h"
#include "ICameraManager.hpp"
#include "GLESFbo.h"


namespace videocore  {
    
    //视频输入类型
    typedef enum _buffer_format_type_
    {
        BufferFormatType_I420 = 2,
        BufferFormatType_BGRA = 3,
        BufferFormatType_PixelBuffer = 5,
        BufferFormatType_NV12 = 9,

    }BufferFormatType;
    
    
    typedef enum _gles_mixer_type_
    {
        GLES_MIXER_TYPE_MIX = 0,
        GLES_MIXER_TYPE_NET = 2,
        GLES_MIXER_TYPE_NET_SECOND = 4,
    }GLESMixerType;

   
    struct SourceBuffer
    {
    public:
        typedef struct __PixelBuffer_ {
            __PixelBuffer_(Apple::PixelBufferRef buf) : texture(nullptr), buffer(buf) {};
            ~__PixelBuffer_() {
                if(texture)
                {
                    CFRelease(texture);
                }
            };
            
            Apple::PixelBufferRef buffer;
            YMCVOpenGLTextureRef texture;
            std::chrono::steady_clock::time_point time;
        }PixelBuffer_;
        
        typedef struct __DataBuffer_ {
            __DataBuffer_()
            :bufferLen(0),
            width(0),
            height(0),
            buffer(nullptr)
            {
            }
            ~__DataBuffer_(){ if(buffer) free(buffer);};
            
            void resetBuffer(uint8_t* buff, uint32_t buffLen, uint32_t w, uint32_t h)
            {
                if(bufferLen != buffLen)
                {
                    if(buffer) free(buffer);
                    buffer = (uint8_t*) malloc(buffLen);
                    if(buffer == NULL)
                        return;
                    bufferLen = buffLen;
                }
                memcpy(buffer, buff, buffLen);
                width = w;
                height = h;
            }
            
        public:
            uint8_t*                buffer;       //yuv or brga
            uint32_t                bufferLen;
            uint32_t                width;
            uint32_t                height;
        }DataBuffer_;
    public:
        SourceBuffer() : m_currentTexture(nullptr),
        m_pixelBuffers(),
        m_width(0),
        m_height(0),
        m_blends(false),
        m_textureId(0),
        m_mirror(0),
        m_timestamp(0){
            m_dataBuffer = std::unique_ptr<DataBuffer_>(new DataBuffer_());
        };
        ~SourceBuffer() {};
 
        void inputBuffer(Apple::PixelBufferRef ref, YMCVOpenGLTextureCacheRef textureCache, JobQueue& jobQueue, void* glContext, uint32_t rotation, uint32_t mirror, uint64_t timestamp);
        
        void inputBuffer(uint8_t* buffer, uint32_t bufferlen, uint32_t width, uint32_t height, uint32_t rotation, uint32_t mirror, uint64_t timestamp, BufferFormatType fmt);

        void inputBuffer(Apple::PixelBufferRef ref,  uint32_t rotation, uint32_t mirror, uint64_t timestamp);

        YMCVOpenGLTextureRef currentTexture() const { return m_currentTexture; };
        Apple::PixelBufferRef currentBuffer() const { return m_currentBuffer; };
        DataBuffer_* currentDataBuffer() const { return m_dataBuffer.get(); };
        BufferFormatType currentFormatType() const {return m_bufferFormat;};
        uint32_t   currentWidth(){return m_rotation==90||m_rotation==270? m_height : m_width;};
        uint32_t   currentHeight(){return m_rotation==90||m_rotation==270? m_width : m_height;};
        uint64_t   getTimestamp(){return m_timestamp;};
        bool blends() const { return m_blends; };
        void setBlends(bool blends) { m_blends = blends; };
        void setTextureId(uint32_t textureId){m_textureId = textureId;}
        uint32_t getTextureId(){return m_textureId ? m_textureId: YMCVOpenGLTextureGetName(currentTexture());}
        uint32_t  mirror(){return m_mirror;}
        uint32_t  setMirror(int mirror){return m_mirror = mirror;}
        uint32_t  rotation(){return m_rotation;}
        uint32_t  setRotation(int rotation){return m_rotation = rotation;}
    private:
        std::map< CVPixelBufferRef, PixelBuffer_ >   m_pixelBuffers;
        Apple::PixelBufferRef                        m_currentBuffer;
        YMCVOpenGLTextureRef                         m_currentTexture;
     
        std::unique_ptr<__DataBuffer_>               m_dataBuffer;
        bool                                         m_blends;
        BufferFormatType                             m_bufferFormat;
        uint32_t                                     m_width;
        uint32_t                                     m_height;
        uint64_t                                     m_timestamp;    //ms
        
        uint32_t                                     m_textureId;
        uint32_t                                     m_mirror;
        uint32_t                                     m_rotation;
    };

 
    class GLESVideoMixer : public IYouMeVideoMixer
    {
        
    public:
        GLESVideoMixer();
        ~GLESVideoMixer();
        static GLESVideoMixer* getInstance();
        
        virtual void setMixVideoSize(int width, int height);
        
        virtual void setVideoNetResolutionWidth(int width, int height);
        
        virtual void setVideoNetResolutionWidthForSecond(int width, int height);
        
        virtual void addMixOverlayVideo(std::string userId, int x, int y, int z, int width, int height);

        virtual void removeMixOverlayVideo(std::string userId);
        
        virtual void removeAllOverlayVideo();
        
        virtual bool needMixing(std::string userId);
        
        virtual void openBeautify();
        
        virtual void closeBeautify();
        
        virtual void setBeautifyLevel(float level);
        
        //local
        void pushBufferForLocal(std::string userId, CVPixelBufferRef pixelBufferRef, int rotation, int mirror,  uint64_t timestamp);
        
        void pushBufferForLocal(std::string userId, const void * buffer, int bufferLen, int width, int height, int rotation, int mirror, uint64_t timestamp, BufferFormatType fmt = BufferFormatType_BGRA);
        
        //remote
        void pushBufferForRemote(std::string userId, CVPixelBufferRef pixelBufferRef, uint64_t timestamp);

        void setSourceFilter(std::string userId, IVideoFilter *filter);
    
        FilterFactory& filterFactory() { return m_filterFactory; };
        
        void mixPaused(bool paused);

        void registerCodecCallback(CameraPreviewCallback *cb);
        void unregisterCodecCallback();
        
        bool setExternalFilterEnabled(bool enabled);
    private:
        void releaseBuffer(std::string userId);
        
        void mixThread();
        
        void sendRawVideoToEncoder(CVPixelBufferRef pixelbuffer, uint64_t& timestamp, int videoId);

        CVPixelBufferRef mixRender(GLESMixerType type, int index, uint64_t& timestamp);
        CVPixelBufferRef netRender(GLESMixerType type, int index, uint64_t& timestamp);
        
        int rawUpload();
        
        int drawfilterExt();
        
        void glRender(int width, int height, SourceBuffer* source, IVideoFilter * filter, GLRect& rc, int mirror = 0, int rotation = 0 );
        
        void beautyChange();
        
        bool initGLES();
        
        bool setupGLES(int width, int height, GLESMixerType type);

        void unloadGLES(GLESMixerType type);
        
        void localLayerChange();
        
        void addMixOverlayVideoNew(std::string userId, int x, int y, int z, int width, int height);

    private:
        FilterFactory m_filterFactory;
        JobQueue m_glJobQueue;
        
        std::thread m_mixThread;
        std::mutex  m_mutex;
        std::condition_variable m_mixThreadCond;
        
        CVPixelBufferPoolRef m_pixelBufferPool;
        YMCVOpenGLTextureCacheRef m_textureCache;
        CVPixelBufferRef   m_pixelBuffer[6];
        YMCVOpenGLTextureRef      m_texture[6];
        
        void*       m_callbackSession;
        void*       m_glesCtx;
        GLuint m_fbo[6];
        
        int m_outBkFrameW;     //back ground w
        int m_outBkFrameH;     //back ground h
        
        int m_outNetFrameW;
        int m_outNetFrameH;
        
        int m_outNetFrameSecondW;
        int m_outNetFrameSecondH;
        struct mixInfo
        {
            int z;
            GLRect rect;
            std::string userId;
        };
        std::vector<mixInfo> m_layerList;

        std::unordered_map<std::string, IVideoFilter*>   m_sourceFilters;
        std::unordered_map<std::string, SourceBuffer>    m_sourceBuffers;
        IVideoFilter*     m_netFirstFilter;
        IVideoFilter*     m_netSecondFilter;
        IVideoFilter*     m_uploadFilter;
        IVideoFilter*     m_extFilter;
        
        std::atomic<bool> m_exiting;
        std::atomic<bool> m_paused;
        
        std::atomic<bool> m_setupGLES;
        std::string m_localUserId;
        
        bool m_isOpenBeauty;
        bool m_currentBeauty;
        float m_beautyLevel;
        
        CameraPreviewCallback * m_cameraPreviewCallback;
        
        GLESFbo m_rawFbo;
        GLESFbo m_filterExtFbo;
        
        bool m_filteExtEnabled;
        bool m_mixSizeOutSet;

        bool m_netMainHwEncode;
        bool m_netSecondHwEncode;

        uint8_t* m_videoData;
        int m_videoDataSize;
    };
    
}
#endif

