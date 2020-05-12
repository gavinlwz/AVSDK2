
#ifndef TINYWRAP_COMMON_H
#define TINYWRAP_COMMON_H

#include "tinyWRAP_config.h"

#if ANDROID
#define dyn_cast static_cast
#define __JNIENV JNIEnv
#else
#define dyn_cast dynamic_cast
#define __JNIENV void
#endif

typedef enum twrap_media_type_e
{
    // because of Java don't use OR
    twrap_media_none = 0x00,

    twrap_media_audio = 0x01,      // (0x01 << 0)
    twrap_media_video = 0x02,      // (0x01 << 1)
   // twrap_media_msrp = 0x04,       // (0x01 << 2)
    twrap_media_t140 = 0x08,       // (0x01 << 3)
    twrap_media_bfcp = 0x10,       // (0x01 << 4)
    twrap_media_bfcp_audio = 0x30, // (0x01 << 5) | twrap_media_bfcp;
    twrap_media_bfcp_video = 0x50, // (0x01 << 6) | twrap_media_bfcp;

    twrap_media_audiovideo = 0x03, /* @deprecated */
    twrap_media_audio_video = twrap_media_audiovideo,
} twrap_media_type_t;

#if !defined(SWIG)
#include "tinymedia/tmedia_common.h"

struct media_type_bind_s
{
    twrap_media_type_t twrap;
    tmedia_type_t tnative;
};
static const struct media_type_bind_s __media_type_binds[] = {
    { twrap_media_audio, tmedia_audio },
    { twrap_media_video, tmedia_video },
    { twrap_media_audio_video, (tmedia_type_t)(tmedia_audio | tmedia_video) },
    { twrap_media_t140, tmedia_t140 },
    { twrap_media_bfcp, tmedia_bfcp },
    { twrap_media_bfcp_audio, tmedia_bfcp_audio },
    { twrap_media_bfcp_video, tmedia_bfcp_video },
};
static const tsk_size_t __media_type_binds_count =
sizeof (__media_type_binds) / sizeof (__media_type_binds[0]);
static inline tmedia_type_t twrap_get_native_media_type (twrap_media_type_t type)
{
    tsk_size_t u;
    tmedia_type_t t = tmedia_none;
    for (u = 0; u < __media_type_binds_count; ++u)
    {
        if ((__media_type_binds[u].twrap & type) == __media_type_binds[u].twrap)
        {
            t = (tmedia_type_t)(t | __media_type_binds[u].tnative);
        }
    }
    return t;
}
static inline twrap_media_type_t twrap_get_wrapped_media_type (tmedia_type_t type)
{
    twrap_media_type_t t = twrap_media_none;
    tsk_size_t u;
    for (u = 0; u < __media_type_binds_count; ++u)
    {
        if ((__media_type_binds[u].tnative & type) == __media_type_binds[u].tnative)
        {
            t = (twrap_media_type_t)(t | __media_type_binds[u].twrap);
        }
    }
    return t;
}
#endif

#endif /* TINYWRAP_COMMON_H */
