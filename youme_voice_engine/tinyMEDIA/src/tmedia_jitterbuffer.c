﻿/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音频通话引擎
 *
 *  当前版本:1.0
 *  作者:brucewang
 *  日期:2015.9.30
 *  说明:对外发布接口
 *
 *  取代版本:0.9
 *  作者:brucewang
 *  日期:2015.9.15
 *  说明:内部测试接口
 ******************************************************************/
#include "tinymedia/tmedia_jitterbuffer.h"

#include "tsk_debug.h"

/* pointer to all registered jitter_buffers */
static const tmedia_jitterbuffer_plugin_def_t *__tmedia_jitterbuffer_plugins[TMED_JITTER_BUFFER_MAX_PLUGINS] = { 0 };

int tmedia_jitterbuffer_init (tmedia_jitterbuffer_t *self)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    return 0;
}

int tmedia_jitterbuffer_set_param (tmedia_jitterbuffer_t *self, const tmedia_param_t *param)
{
    if (!self || !self->plugin || !param)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }
    return self->plugin->set_param ? self->plugin->set_param (self, param) : 0;
}

int tmedia_jitterbuffer_get_param (tmedia_jitterbuffer_t *self, tmedia_param_t *param)
{
    if (!self || !self->plugin || !param)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }
    return self->plugin->get_param ? self->plugin->get_param (self, param) : 0;
}

int tmedia_jitterbuffer_open (tmedia_jitterbuffer_t *self, uint32_t frame_duration, uint32_t in_rate, uint32_t out_rate, uint32_t channels)
{
    int ret;

    if (!self || !self->plugin || !self->plugin->open)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (self->opened)
    {
        TSK_DEBUG_WARN ("JitterBuffer already opened");
        return 0;
    }

    if ((ret = self->plugin->open (self, frame_duration, in_rate, out_rate, channels)))
    {
        TSK_DEBUG_ERROR ("Failed to open [%s] jitterbufferr", self->plugin->desc);
        return ret;
    }
    else
    {
        self->opened = tsk_true;
        return 0;
    }
}

int tmedia_jitterbuffer_tick (tmedia_jitterbuffer_t *self)
{
    if (!self || !self->plugin || !self->plugin->tick)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if (!self->opened)
    {
        TSK_DEBUG_ERROR ("JitterBuffer not opened");
        return -1;
    }

    return self->plugin->tick (self);
}

int tmedia_jitterbuffer_put_bkaud (tmedia_jitterbuffer_t *self, void *data, tsk_object_t *proto_hdr)
{
    if (!self || !self->plugin || !self->plugin->put_bkaud || !proto_hdr)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    if (!self->opened)
    {
        TSK_DEBUG_ERROR ("JitterBuffer not opened");
        return -1;
    }
    
    return self->plugin->put_bkaud (self, data, proto_hdr);
}

int tmedia_jitterbuffer_put (tmedia_jitterbuffer_t *self, void *data, tsk_size_t data_size, tsk_object_t *proto_hdr)
{
    if (!self || !self->plugin || !self->plugin->put || !proto_hdr)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if (!self->opened)
    {
        TSK_DEBUG_ERROR ("JitterBuffer not opened");
        return -1;
    }

    return self->plugin->put (self, data, data_size, proto_hdr);
}

tsk_size_t tmedia_jitterbuffer_get (tmedia_jitterbuffer_t *self, void *out_data, void *voice_data, tsk_size_t out_size)
{
    if (!self || !self->plugin || !self->plugin->get)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }

    if (!self->opened)
    {
        TSK_DEBUG_ERROR ("JitterBuffer not opened");
        return 0;
    }

    return self->plugin->get (self, out_data, voice_data, out_size);
}

int tmedia_jitterbuffer_reset (tmedia_jitterbuffer_t *self)
{
    if (!self || !self->plugin)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (self->opened && self->plugin->reset)
    {
        return self->plugin->reset (self);
    }

    return 0;
}

int tmedia_jitterbuffer_close (tmedia_jitterbuffer_t *self)
{
    if (!self || !self->plugin)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (!self->opened)
    {
        TSK_DEBUG_WARN ("JitterBuffer not opened");
        return 0;
    }

    if (self->plugin->close)
    {
        int ret;

        if ((ret = self->plugin->close (self)))
        {
            TSK_DEBUG_ERROR ("Failed to close [%s] jitterbufferr", self->plugin->desc);
            return ret;
        }
        else
        {
            self->opened = tsk_false;
            return 0;
        }
    }
    else
    {
        self->opened = tsk_false;
        return 0;
    }
}

int tmedia_jitterbuffer_deinit (tmedia_jitterbuffer_t *self)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if (self->opened)
    {
        tmedia_jitterbuffer_close (self);
    }

    return 0;
}

tmedia_jitterbuffer_t *tmedia_jitterbuffer_create (tmedia_type_t type)
{
    tmedia_jitterbuffer_t *jitter_buffer = tsk_null;
    const tmedia_jitterbuffer_plugin_def_t *plugin;
    tsk_size_t i = 0;

    while ((i < TMED_JITTER_BUFFER_MAX_PLUGINS) && (plugin = __tmedia_jitterbuffer_plugins[i++]))
    {
        if (plugin->objdef && plugin->type == type)
        {
            if ((jitter_buffer = tsk_object_new (plugin->objdef)))
            {
                /* initialize the newly created jitter_buffer */
                jitter_buffer->plugin = plugin;
                break;
            }
        }
    }

    return jitter_buffer;
}

int tmedia_jitterbuffer_plugin_register (const tmedia_jitterbuffer_plugin_def_t *plugin)
{
    tsk_size_t i;
    if (!plugin)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    /* add or replace the plugin */
    for (i = 0; i < TMED_JITTER_BUFFER_MAX_PLUGINS; i++)
    {
        if (!__tmedia_jitterbuffer_plugins[i] || (__tmedia_jitterbuffer_plugins[i] == plugin))
        {
            __tmedia_jitterbuffer_plugins[i] = plugin;
            return 0;
        }
    }

    TSK_DEBUG_ERROR ("There are already %d plugins.", TMED_JITTER_BUFFER_MAX_PLUGINS);
    return -2;
}

/**@ingroup tmedia_jitterbuffer_group
* UnRegisters a jitter_buffer plugin.
* @param plugin the definition of the plugin.
* @retval Zero if succeed and non-zero error code otherwise.
*/
int tmedia_jitterbuffer_plugin_unregister (const tmedia_jitterbuffer_plugin_def_t *plugin)
{
    tsk_size_t i;
    tsk_bool_t found = tsk_false;
    if (!plugin)
    {
        TSK_DEBUG_ERROR ("Invalid Parameter");
        return -1;
    }

    /* find the plugin to unregister */
    for (i = 0; i < TMED_JITTER_BUFFER_MAX_PLUGINS && __tmedia_jitterbuffer_plugins[i]; i++)
    {
        if (__tmedia_jitterbuffer_plugins[i] == plugin)
        {
            __tmedia_jitterbuffer_plugins[i] = tsk_null;
            found = tsk_true;
            break;
        }
    }

    /* compact */
    if (found)
    {
        for (; i < (TMED_JITTER_BUFFER_MAX_PLUGINS - 1); i++)
        {
            if (__tmedia_jitterbuffer_plugins[i + 1])
            {
                __tmedia_jitterbuffer_plugins[i] = __tmedia_jitterbuffer_plugins[i + 1];
            }
            else
            {
                break;
            }
        }
        __tmedia_jitterbuffer_plugins[i] = tsk_null;
    }
    return (found ? 0 : -2);
}

int tmedia_jitterbuffer_plugin_unregister_by_type (tmedia_type_t type)
{
    tsk_size_t i;
    tsk_bool_t found = tsk_false;

    /* find the plugin to unregister */
    for (i = 0; i < TMED_JITTER_BUFFER_MAX_PLUGINS && __tmedia_jitterbuffer_plugins[i]; i++)
    {
        if ((__tmedia_jitterbuffer_plugins[i]->type & type) == __tmedia_jitterbuffer_plugins[i]->type)
        {
            __tmedia_jitterbuffer_plugins[i] = tsk_null;
            found = tsk_true;
            break;
        }
    }

    /* compact */
    if (found)
    {
        for (; i < (TMED_JITTER_BUFFER_MAX_PLUGINS - 1); i++)
        {
            if (__tmedia_jitterbuffer_plugins[i + 1])
            {
                __tmedia_jitterbuffer_plugins[i] = __tmedia_jitterbuffer_plugins[i + 1];
            }
            else
            {
                break;
            }
        }
        __tmedia_jitterbuffer_plugins[i] = tsk_null;
    }
    return (found ? 0 : -2);
}
