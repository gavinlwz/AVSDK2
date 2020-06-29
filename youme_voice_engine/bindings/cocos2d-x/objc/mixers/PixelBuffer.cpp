//
//  PixelBuffer.cpp
//  VideoCodecTest
//
//  Created by bhb on 2017/9/01.
//  Copyright © 2017年 youme. All rights reserved.
//
#include "PixelBuffer.h"

namespace videocore { namespace Apple {
 
    PixelBuffer::PixelBuffer(CVPixelBufferRef pb, bool temporary)
    : m_state(kVCPixelBufferStateAvailable),
    m_locked(false),
    m_pixelBuffer(CVPixelBufferRetain(pb)),
    m_temporary(temporary)
    {
        m_pixelFormat = (PixelBufferFormatType)CVPixelBufferGetPixelFormatType(pb);
    }
    PixelBuffer::~PixelBuffer()
    {
        CVPixelBufferRelease(m_pixelBuffer);
    }
    
    void  PixelBuffer::lock(bool readonly)
    {
        m_locked = true;
        CVPixelBufferLockBaseAddress( (CVPixelBufferRef)cvBuffer(), readonly ? kCVPixelBufferLock_ReadOnly : 0 );
    }
    void    PixelBuffer::unlock(bool readonly)
    {
        m_locked = false;
        CVPixelBufferUnlockBaseAddress( (CVPixelBufferRef)cvBuffer(), readonly ? kCVPixelBufferLock_ReadOnly : 0 );
    }
    
}
}
