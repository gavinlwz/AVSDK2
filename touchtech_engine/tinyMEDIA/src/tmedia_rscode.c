//
//  tmedia_rscode.c
//  youme_voice_engine
//
//  Created by 游密 on 2017/5/8.
//  Copyright © 2017年 Youme. All rights reserved.
//
#include "tinymedia/tmedia_defaults.h"
#include "tsk_debug.h"
#include "tinymedia/tmedia_rscode.h"


/* pointer to all registered jitter_buffers */
static const tmedia_rscode_plugin_def_t *__tmedia_rscode_plugins[TMED_RSCODE_MAX_PLUGINS] = { 0 };


tmedia_rscode_t* tmedia_rscode_create(tmedia_type_t type, uint64_t session_id) {
    tmedia_rscode_t *rscode = tsk_null;

    const tmedia_rscode_plugin_def_t *plugin;
    tsk_size_t i = 0;

    while ((i < TMED_RSCODE_MAX_PLUGINS) && (plugin = __tmedia_rscode_plugins[i++]))
    {
        if (plugin->objdef && plugin->type == type)
        {
            if ((rscode = tsk_object_new (plugin->objdef)))
            {
                /* initialize the newly created consumer */
                //rscode->plugin = plugin;
                //rscode->session_id = session_id;
                break;
            }
        }
    }

    return rscode;
}

int tmedia_rscode_init(tmedia_rscode_t* self) {
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    //self->video.in.chroma = TMEDIA_CONSUMER_CHROMA_DEFAULT;
    self->type = 0;
    
    return 0;
}

int tmedia_rscode_set(tmedia_rscode_t *self, const tmedia_param_t* param) {
    if (!self || !self->plugin || !self->plugin->set || !param)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    return self->plugin->set (self, param);
}

int tmedia_rscode_prepare(tmedia_rscode_t *self, const tmedia_codec_t* codec) {
    int ret;
    if (!self || !self->plugin || !self->plugin->prepare || !codec)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if ((ret = self->plugin->prepare (self)) == 0)
    {
        self->is_prepared = tsk_true;
    }
    return ret;
}

int tmedia_rscode_start(tmedia_rscode_t *self) {
    int ret;
    if (!self || !self->plugin || !self->plugin->start)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if ((ret = self->plugin->start (self)) == 0)
    {
        self->is_started = tsk_true;
    }
    return ret;
}

int tmedia_rscode_consume(tmedia_rscode_t* self, const void* buffer, tsk_size_t size, tsk_object_t* proto_hdr) {
    return 0;
}

int tmedia_rscode_pause(tmedia_rscode_t *self) {
    return 0;
}

int tmedia_rscode_stop(tmedia_rscode_t *self) {
    return 0;
}

int tmedia_rscode_deinit(tmedia_rscode_t* self) {
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    return 0;
}

int tmedia_rscode_plugin_register(const tmedia_rscode_plugin_def_t* plugin) {
    tsk_size_t i;
    if (!plugin)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    /* add or replace the plugin */
    for (i = 0; i < TMED_RSCODE_MAX_PLUGINS; i++)
    {
        if (!__tmedia_rscode_plugins[i] || (__tmedia_rscode_plugins[i] == plugin))
        {
            __tmedia_rscode_plugins[i] = plugin;
            return 0;
        }
    }
    
    TSK_DEBUG_ERROR ("There are already %d plugins.", TMED_RSCODE_MAX_PLUGINS);
    return -2;
}

int tmedia_rscode_plugin_unregister(const tmedia_rscode_plugin_def_t* plugin) {
    tsk_size_t i;
    tsk_bool_t found = tsk_false;
    if (!plugin)
    {
        TSK_DEBUG_ERROR ("Invalid Parameter");
        return -1;
    }
    
    /* find the plugin to unregister */
    for (i = 0; i < TMED_RSCODE_MAX_PLUGINS && __tmedia_rscode_plugins[i]; i++)
    {
        if (__tmedia_rscode_plugins[i] == plugin)
        {
            __tmedia_rscode_plugins[i] = tsk_null;
            found = tsk_true;
            break;
        }
    }
    
    /* compact */
    if (found)
    {
        for (; i < (TMED_RSCODE_MAX_PLUGINS - 1); i++)
        {
            if (__tmedia_rscode_plugins[i + 1])
            {
                __tmedia_rscode_plugins[i] = __tmedia_rscode_plugins[i + 1];
            }
            else
            {
                break;
            }
        }
        __tmedia_rscode_plugins[i] = tsk_null;
    }
    return (found ? 0 : -2);
}

int tmedia_rscode_plugin_unregister_by_type(tmedia_type_t type) {
    return 0;
}
