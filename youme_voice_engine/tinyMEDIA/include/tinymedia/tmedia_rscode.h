//
//  tmedia_rscode.h
//  youme_voice_engine
//
//  Created by 游密 on 2017/5/8.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef tmedia_rscode_h
#define tmedia_rscode_h

#include "tinymedia_config.h"
#include "tinymedia/tmedia_codec.h"
#include "tinymedia/tmedia_params.h"
#include "tmedia_common.h"

TMEDIA_BEGIN_DECLS

/**Max number of plugins (consumer types) we can create */
#if !defined(TMED_RSCODE_MAX_PLUGINS)
#	define TMED_RSCODE_MAX_PLUGINS			0x0F
#endif

/** cast any pointer to @ref tmedia_consumer_t* object */
#define TMEDIA_RSCODE(self)		((tmedia_rscode_t*)(self))

/** Base object for all Consumers */
typedef struct tmedia_rscode_s
{
    TSK_DECLARE_OBJECT;

    tmedia_type_t type;
    tsk_bool_t is_started;
    tsk_bool_t is_prepared;
    const char* desc;

    struct {
        int type;
    } encode;
    
    struct {
        int type;
    } decode;
    
    const struct tmedia_rscode_plugin_def_s* plugin;
} tmedia_rscode_t;

/** Virtual table used to define a rscode plugin */
typedef struct tmedia_rscode_plugin_def_s
{
    //! object definition used to create an instance of the consumer
    const tsk_object_def_t* objdef;
    
    //! the type of the consumer
    tmedia_type_t type;
    //! full description (usefull for debugging)
    const char* desc;
    
    int (* set) (tmedia_rscode_t*, const tmedia_param_t*);
    int (* prepare) (tmedia_rscode_t*);
    int (* start) (tmedia_rscode_t*);
    int (* consume) (tmedia_rscode_t*);
    int (* pause) (tmedia_rscode_t*);
    int (* stop) (tmedia_rscode_t*);
}
tmedia_rscode_plugin_def_t;

#define TMEDIA_DECLARE_RSCODE tmedia_rscode_t __rscode__

TINYMEDIA_API tmedia_rscode_t* tmedia_rscode_create(tmedia_type_t type, uint64_t session_id);
TINYMEDIA_API int tmedia_rscode_init(tmedia_rscode_t* self);
TINYMEDIA_API int tmedia_rscode_set(tmedia_rscode_t *self, const tmedia_param_t* param);
TINYMEDIA_API int tmedia_rscode_prepare(tmedia_rscode_t *self, const tmedia_codec_t* codec);
TINYMEDIA_API int tmedia_rscode_start(tmedia_rscode_t *self);
TINYMEDIA_API int tmedia_rscode_consume(tmedia_rscode_t* self, const void* buffer, tsk_size_t size, tsk_object_t* proto_hdr);
TINYMEDIA_API int tmedia_rscode_pause(tmedia_rscode_t *self);
TINYMEDIA_API int tmedia_rscode_stop(tmedia_rscode_t *self);
TINYMEDIA_API int tmedia_rscode_deinit(tmedia_rscode_t* self);

TINYMEDIA_API int tmedia_rscode_plugin_register(const tmedia_rscode_plugin_def_t* plugin);
TINYMEDIA_API int tmedia_rscode_plugin_unregister(const tmedia_rscode_plugin_def_t* plugin);
TINYMEDIA_API int tmedia_rscode_plugin_unregister_by_type(tmedia_type_t type);

TMEDIA_END_DECLS

#endif /* tmedia_rscode_h */
