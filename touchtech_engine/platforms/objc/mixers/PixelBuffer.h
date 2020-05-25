//
//  PixelBuffer.h
//  VideoCodecTest
//
//  Created by bhb on 2017/9/01.
//  Copyright © 2017年 youme. All rights reserved.
//
#ifndef videocore_PixelBuffer_hpp
#define videocore_PixelBuffer_hpp

#include <CoreVideo/CoreVideo.h>
#include "IPixelBuffer.h"
#include <memory>

namespace videocore { namespace Apple {
 
    
    class PixelBuffer : public IPixelBuffer
    {
        
    public:
        
        PixelBuffer(CVPixelBufferRef pb, bool temporary = false);
        ~PixelBuffer();
        
        static const size_t hash(std::shared_ptr<PixelBuffer> buf) { return std::hash<std::shared_ptr<PixelBuffer>>()(buf); };
        
    public:
        const int   width() const  { return (int)CVPixelBufferGetWidth(m_pixelBuffer); };
        const int   height() const { return (int)CVPixelBufferGetHeight(m_pixelBuffer); };
        const void* baseAddress() const { return CVPixelBufferGetBaseAddress(m_pixelBuffer); };
        
        const PixelBufferFormatType pixelFormat() const { return m_pixelFormat; };
        
        void  lock(bool readOnly = false);
        void  unlock(bool readOnly = false);
        
        void  setState(const PixelBufferState state) { m_state = state; };
        const PixelBufferState state() const { return m_state; };
        
        const bool isTemporary() const { return m_temporary; };
        void setTemporary(const bool temporary) { m_temporary = temporary; };
        
    public:
        const CVPixelBufferRef cvBuffer() const { return m_pixelBuffer; };
        
    private:
        
        CVPixelBufferRef m_pixelBuffer;
        PixelBufferFormatType m_pixelFormat;
        PixelBufferState m_state;
        
        bool m_locked;
        bool m_temporary;
    };
    
    typedef std::shared_ptr<PixelBuffer> PixelBufferRef;
    
}
}

#endif
