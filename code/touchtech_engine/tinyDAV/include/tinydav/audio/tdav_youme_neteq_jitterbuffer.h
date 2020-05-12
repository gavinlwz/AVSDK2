//
//  tdav_webrtc_neteq_jitterbuffer.h
//  youme_voice_engine
//
//  Created by peter on 6/27/16.
//  Copyright © 2016 Youme. All rights reserved.
//

#ifndef TINYDAV_YOUME_NETEQ_JITTER_BUFFER_H
#define TINYDAV_YOUME_NETEQ_JITTER_BUFFER_H

#include "tinydav_config.h"

#include "tinymedia/tmedia_jitterbuffer.h"
#include "tinydav/audio/tdav_session_audio.h"

TDAV_BEGIN_DECLS

extern const tmedia_jitterbuffer_plugin_def_t *tdav_youme_neteq_jitterbuffer_plugin_def_t;

typedef void (*voiceLevelCb_t)(int32_t level, uint32_t sessionId);

TDAV_END_DECLS

#endif /* TINYDAV_YOUME_NETEQ_JITTER_BUFFER_H */
