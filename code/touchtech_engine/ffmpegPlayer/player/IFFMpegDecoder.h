//
//  IFFMpegAudioDecoder.h
//  youme_voice_engine
//
//  Created by peter on 8/13/16.
//  Copyright © 2016 Youme. All rights reserved.
//

#ifndef _IFFMPEG_DECODER_H_
#define _IFFMPEG_DECODER_H_

#include "version.h"
#if FFMPEG_SUPPORT


#include <stdint.h>

typedef struct AudioMeta_s
{
    uint8_t  channels;
    uint32_t sampleRate;
    uint8_t  isInterleaved;
} AudioMeta_t;

typedef struct VideoMeta_s
{
    int32_t  i4PicWidth;
    int32_t  i4PicHeight;
    int32_t  i4PixelFormat;
    int32_t  i4CodedPicOrder;
    int32_t  i4DisplayPicOrder;
    int64_t  i8Pts;
} VideoMeta_t;

class IFFMpegDecoder
{
public:
    // Class factory
    static IFFMpegDecoder* createInstance();
    static void destroyInstance(IFFMpegDecoder** ppInstance);
    
    // A virtual destructor to make sure the correct destructor of the derived classes will called.
    virtual ~IFFMpegDecoder() {}
    
    virtual bool open(const char* pFilePath) = 0;
    virtual void close() = 0;
    virtual int32_t getNextFrame(void** ppBuf, uint32_t *pMaxSize, AudioMeta_t *pAudioMeta, VideoMeta_s *pVideoMeta) = 0;
};

#endif //FFMPEG_SUPPORT

#endif /* _IFFMPEG_DECODER_H_ */
