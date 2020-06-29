/*******************************************************************
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

/**@file tmedia_producer.c
 * @brief Base producer object.
 */
#include "tinymedia/tmedia_producer.h"
#include "tinymedia/tmedia_defaults.h"

#include "tsk_debug.h"

/**@defgroup tmedia_producer_group Producers
*/

/* pointer to all registered producers */
const tmedia_producer_plugin_def_t *__tmedia_producer_plugins[TMED_PRODUCER_MAX_PLUGINS] = { 0 };

/**@ingroup tmedia_producer_group
* Initialize the producer.
* @param self The producer to initialize
* @retval Zero if succeed and non-zero error code otherwise.
*
* @sa @ref tmedia_producer_deinit
*/
int tmedia_producer_init (tmedia_producer_t *self)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    self->video.chroma = TMEDIA_PRODUCER_CHROMA_DEFAULT;
    self->audio.bits_per_sample = TMEDIA_PRODUCER_BITS_PER_SAMPLE_DEFAULT;
    self->audio.channels = TMEDIA_PRODUCER_CHANNELS_DEFAULT;
    self->audio.rate = TMEDIA_PRODUCER_RATE_DEFAULT;
    self->audio.volume = tmedia_defaults_get_volume ();
    self->audio.useHalMode = tsk_false;

    return 0;
}

/**@ingroup tmedia_producer_group
* callback to encode and send() data
*/
int tmedia_producer_set_enc_callback (tmedia_producer_t *self, tmedia_producer_enc_cb_f callback, const void *callback_data)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    self->enc_cb.callback = callback;
    self->enc_cb.callback_data = callback_data;

    return 0;
}

int tmedia_producer_set_enc_callback_new (tmedia_producer_t *self, tmedia_producer_enc_cb_f_new callback_new, const void *callback_data)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    self->enc_cb.callback_new = callback_new;
    self->enc_cb.callback_data = callback_data;
    
    return 0;
}

/**@ingroup tmedia_producer_group
* callback to send() data "as is"
*/
int tmedia_producer_set_raw_callback (tmedia_producer_t *self, tmedia_producer_raw_cb_f callback, const void* callback_data)
{
	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	self->raw_cb.callback = callback;
	self->raw_cb.chunck_curr.usr_data = callback_data;

	return 0;
}


/**@ingroup tmedia_producer_group
* @retval Zero if succeed and non-zero error code otherwise
*/
int tmedia_producer_set (tmedia_producer_t *self, const tmedia_param_t *param)
{
    if (!self || !self->plugin || !self->plugin->set || !param)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    return self->plugin->set (self, param);
}

/**@ingroup tmedia_producer_group
 * @retval Zero if succeed and non-zero error code otherwise
 */
int tmedia_producer_get (tmedia_producer_t *self, tmedia_param_t *param)
{
    if (!self || !self->plugin || !self->plugin->get || !param)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    return self->plugin->get (self, param);
}

/**@ingroup tmedia_producer_group
* Alert the producer to be prepared to start.
* @param self the producer to prepare
* @param codec The codec to use to prepare the producer
* @retval Zero if succeed and non-zero error code otherwise
*/
int tmedia_producer_prepare (tmedia_producer_t *self, const tmedia_codec_t *codec)
{
    int ret;
    if (!self || !self->plugin || !self->plugin->prepare || !codec)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if ((ret = self->plugin->prepare (self, codec)) == 0)
    {
        self->is_prepared = tsk_true;
    }
    return ret;
}

/**@ingroup tmedia_producer_group
* Starts the producer
* @param self The producer to start
* @retval Zero if succeed and non-zero error code otherwise
*/
int tmedia_producer_start (tmedia_producer_t *self)
{
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


/**@ingroup tmedia_producer_group
* Pauses the producer
* @param self The producer to pause
* @retval Zero if succeed and non-zero error code otherwise
*/
int tmedia_producer_pause (tmedia_producer_t *self)
{
    if (!self || !self->plugin || !self->plugin->pause)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    return self->plugin->pause (self);
}


/**@ingroup tmedia_producer_group
* Stops the producer
* @param self The producer to stop
* @retval Zero if succeed and non-zero error code otherwise
*/
int tmedia_producer_stop (tmedia_producer_t *self)
{
    int ret;
    if (!self || !self->plugin || !self->plugin->stop)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if ((ret = self->plugin->stop (self)) == 0)
    {
        self->is_started = tsk_false;
    }
    return ret;
}


/**@ingroup tmedia_producer_group
* DeInitialize the producer.
* @param self The producer to deinitialize
* @retval Zero if succeed and non-zero error code otherwise.
*
* @sa @ref tmedia_producer_deinit
*/
int tmedia_producer_deinit (tmedia_producer_t *self)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    return 0;
}

/**@ingroup tmedia_producer_group
* Creates a new producer using an already registered plugin.
* @param type The type of the producer to create
* @param session_id
* @sa @ref tmedia_producer_plugin_register()
*/
tmedia_producer_t *tmedia_producer_create (tmedia_type_t type, uint64_t session_id)
{
    tmedia_producer_t *producer = tsk_null;
    const tmedia_producer_plugin_def_t *plugin;
    tsk_size_t i = 0;

    while ((i < TMED_PRODUCER_MAX_PLUGINS) && (plugin = __tmedia_producer_plugins[i++]))
    {
        TSK_DEBUG_INFO ("tmedia_producer_create index:%d type:%d, ", i, plugin->type);
        if (plugin->objdef && plugin->type == type)
        {
            if ((producer = tsk_object_new (plugin->objdef)))
            {
                /* initialize the newly created producer */
                producer->plugin = plugin;
                producer->session_id = session_id;
                break;
            }
        }
    }

    return producer;
}

/**@ingroup tmedia_producer_group
* Registers a producer plugin.
* @param plugin the definition of the plugin.
* @retval Zero if succeed and non-zero error code otherwise.
* @sa @ref tmedia_producer_create()
*/
int tmedia_producer_plugin_register (const tmedia_producer_plugin_def_t *plugin)
{
    tsk_size_t i;
    if (!plugin)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    /* add or replace the plugin */
    for (i = 0; i < TMED_PRODUCER_MAX_PLUGINS; i++)
    {
        if (!__tmedia_producer_plugins[i] || (__tmedia_producer_plugins[i] == plugin))
        {
            __tmedia_producer_plugins[i] = plugin;
            return 0;
        }
    }

    TSK_DEBUG_ERROR ("There are already %d plugins.", TMED_PRODUCER_MAX_PLUGINS);
    return -2;
}

/**@ingroup tmedia_producer_group
* UnRegisters a producer plugin.
* @param plugin the definition of the plugin.
* @retval Zero if succeed and non-zero error code otherwise.
*/
int tmedia_producer_plugin_unregister (const tmedia_producer_plugin_def_t *plugin)
{
    tsk_size_t i;
    tsk_bool_t found = tsk_false;
    if (!plugin)
    {
        TSK_DEBUG_ERROR ("Invalid Parameter");
        return -1;
    }

    /* find the plugin to unregister */
    for (i = 0; i < TMED_PRODUCER_MAX_PLUGINS && __tmedia_producer_plugins[i]; i++)
    {
        if (__tmedia_producer_plugins[i] == plugin)
        {
            __tmedia_producer_plugins[i] = tsk_null;
            found = tsk_true;
            break;
        }
    }

    /* compact */
    if (found)
    {
        for (; i < (TMED_PRODUCER_MAX_PLUGINS - 1); i++)
        {
            if (__tmedia_producer_plugins[i + 1])
            {
                __tmedia_producer_plugins[i] = __tmedia_producer_plugins[i + 1];
            }
            else
            {
                break;
            }
        }
        __tmedia_producer_plugins[i] = tsk_null;
    }
    return (found ? 0 : -2);
}

/**@ingroup tmedia_producer_group
* UnRegisters all producers matching the given type.
* @param type the type of the plugins to unregister.
* @retval Zero if succeed and non-zero error code otherwise.
*/
int tmedia_producer_plugin_unregister_by_type (tmedia_type_t type)
{
    int i, j;

    /* find the plugin to unregister */
    for (i = 0; i < TMED_PRODUCER_MAX_PLUGINS && __tmedia_producer_plugins[i];)
    {
        if ((__tmedia_producer_plugins[i]->type & type) == __tmedia_producer_plugins[i]->type)
        {
            __tmedia_producer_plugins[i] = tsk_null;
            /* compact */
            for (j = i; j < (TMED_PRODUCER_MAX_PLUGINS - 1) && __tmedia_producer_plugins[j + 1]; ++j)
            {
                __tmedia_producer_plugins[j] = __tmedia_producer_plugins[j + 1];
            }
            __tmedia_producer_plugins[j] = tsk_null;
        }
        else
        {
            ++i;
        }
    }
    return 0;
}
