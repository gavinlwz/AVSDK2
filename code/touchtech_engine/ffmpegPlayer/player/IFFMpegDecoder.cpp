//
//  IFFMpegAudioDecoder.cpp
//  youme_voice_engine
//
//  Created by peter on 8/13/16.
//  Copyright © 2016 Youme. All rights reserved.
//

#include "version.h"
#if FFMPEG_SUPPORT

#include "IFFMpegDecoder.h"
#include "FFMpegDecoder.h"

IFFMpegDecoder* IFFMpegDecoder::createInstance()
{
    return new CFFMpegDecoder();
}

void IFFMpegDecoder::destroyInstance(IFFMpegDecoder** ppInstance)
{
    if (ppInstance && *ppInstance) {
        delete *ppInstance;
        *ppInstance = NULL;
    }
}


#endif //FFMPEG_SUPPORT

