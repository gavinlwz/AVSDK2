//
//  PixelBuffer.h
//  VideoCodecTest
//
//  Created by bhb on 2017/9/01.
//  Copyright © 2017年 youme. All rights reserved.
//

#ifndef videocore_IPixelBuffer_hpp
#define videocore_IPixelBuffer_hpp

#include <stdint.h>

namespace videocore {


    
    typedef uint32_t PixelBufferFormatType;
    
    typedef enum {
        kVCPixelBufferStateAvailable,
        kVCPixelBufferStateDequeued,
        kVCPixelBufferStateEnqueued,
        kVCPixelBufferStateAcquired
    } PixelBufferState;
    
	class IPixelBuffer {
	public:
		virtual ~IPixelBuffer() {};

		virtual const int   width() const = 0;
		virtual const int   height() const = 0;

		virtual const PixelBufferFormatType pixelFormat() const = 0;

		virtual const void* baseAddress() const = 0;

		virtual void  lock(bool readOnly = false) = 0;
		virtual void  unlock(bool readOnly = false) = 0;
        
        virtual void setState(const PixelBufferState state) = 0;
        virtual const PixelBufferState state() const = 0;
        
        virtual const bool isTemporary() const = 0; /* mark if the pixel buffer needs to disappear soon */
        virtual void setTemporary(const bool temporary) = 0;
	};
}

 #endif
