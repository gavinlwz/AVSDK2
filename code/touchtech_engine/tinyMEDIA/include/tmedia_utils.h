//
//  tmedia_utils.h
//  youme_voice_engine
//
//  Created by mac on 2017/5/19.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef tmedia_utils_h
#define tmedia_utils_h

#include "tsk_common.h"
#include "tinymedia_config.h"
#include "tinyrtp.h"

TMEDIA_BEGIN_DECLS
#define TMEDIA_DUMP_FILE_MAX_SIZE    (1024*1024)

#if TMEDIA_DUMP_ENABLE
#define TMEDIA_DUMP_TO_FILE(file_name, data, size) \
{ \
    static tsk_bool_t create = tsk_true;\
    tmedia_utils_dump(file_name, data, size, &create);\
}

#define TMEDIA_DUMP_LOG_TO_FILE(file_name, FMT, ...) \
{ \
    static tsk_bool_t create = tsk_true;\
    tmedia_utils_dump_log(file_name, &create, FMT, ##__VA_ARGS__);\
}

#define TMEDIA_DUMP_RTP_TO_FILE(file_name, packet) \
{ \
    static tsk_bool_t create = tsk_true;\
    tmedia_utils_dump_rtp_packet(file_name, packet, &create);\
}
#else
#define TMEDIA_DUMP_TO_FILE(file_name, data, size)
#define TMEDIA_DUMP_LOG_TO_FILE(file_name, FMT, ...)
#define TMEDIA_DUMP_RTP_TO_FILE(file_name, packet)
#endif

#define TMEDIA_I_AM_ACTIVE(tick, FMT, ...) \
{ \
    static int count = 0; \
    if((count % tick) == 0) { \
        TSK_DEBUG_INFO("[iamactive] %d times, " FMT, tick, ##__VA_ARGS__); \
    } \
    count++;\
}

#define TMEDIA_I_AM_ACTIVE_ERROR(tick, FMT, ...) \
{ \
    static int count = 0; \
    if((count % tick) == 0) { \
        TSK_DEBUG_ERROR("[iamactive] error %d times, " FMT, tick, ##__VA_ARGS__); \
    } \
    count++;\
}

TINYMEDIA_API uint32_t tmedia_utils_dump(const char *fileName, const void *data, uint32_t size, tsk_bool_t *create);
TINYMEDIA_API uint32_t tmedia_utils_dump_log(const char *filename, tsk_bool_t *create, const char *pszFormat, ...);
TINYMEDIA_API uint32_t tmedia_utils_dump_rtp_packet(const char *filename, trtp_rtp_packet_t *packet, tsk_bool_t *create);

TMEDIA_END_DECLS

#endif /* tmedia_utils_h */
