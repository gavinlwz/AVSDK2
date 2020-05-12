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
#ifndef TINYMEDIA_CONSUMER_H
#define TINYMEDIA_CONSUMER_H

#include "tinymedia_config.h"

#include "tinymedia/tmedia_codec.h"
#include "tinymedia/tmedia_params.h"
#include "tmedia_common.h"

TMEDIA_BEGIN_DECLS

#define TMEDIA_CONSUMER_BITS_PER_SAMPLE_DEFAULT		16
#define TMEDIA_CONSUMER_CHANNELS_DEFAULT			2
#define TMEDIA_CONSUMER_RATE_DEFAULT				8000


/**Max number of plugins (consumer types) we can create */
#if !defined(TMED_CONSUMER_MAX_PLUGINS)
#	define TMED_CONSUMER_MAX_PLUGINS			0x0F
#endif

/** cast any pointer to @ref tmedia_consumer_t* object */
#define TMEDIA_CONSUMER(self)		((tmedia_consumer_t*)(self))

/**  Default Video chroma */
#if !defined(TMEDIA_CONSUMER_CHROMA_DEFAULT)
#	define TMEDIA_CONSUMER_CHROMA_DEFAULT tmedia_chroma_yuv420p
#endif

/** Base object for all Consumers */
typedef struct tmedia_consumer_s
{
	TSK_DECLARE_OBJECT;
	
	tmedia_type_t type;
	const char* desc;

	struct{
		int fps;
		struct {
			tmedia_chroma_t chroma;
			tsk_size_t width;
			tsk_size_t height;
		} in;
		struct {
			tmedia_chroma_t chroma;
			tsk_size_t width;
			tsk_size_t height;
			tsk_bool_t auto_resize; // auto_resize to "in.width, in.height"
		} display;
	} video;

	struct{
		uint8_t bits_per_sample;
		uint8_t ptime;
		uint8_t gain;
		struct{
			uint8_t channels;
			uint32_t rate;
		} in;
		struct{
			uint8_t channels;
			uint32_t rate;
		} out;
		float volume;
        
        uint32_t fadeInVolume;
        uint32_t fadeInStep;
        uint32_t fadeInTime;
        
        tsk_bool_t useHalMode;
	} audio;

	tsk_bool_t is_started;
	tsk_bool_t is_prepared;
	uint64_t session_id;

	struct{
		enum tmedia_codec_id_e codec_id;
		// other options to be added
	} decoder;

	const struct tmedia_consumer_plugin_def_s* plugin;
    tsk_bool_t isSpeakerMute;
} tmedia_consumer_t;

/** Virtual table used to define a consumer plugin */
typedef struct tmedia_consumer_plugin_def_s
{
	//! object definition used to create an instance of the consumer
	const tsk_object_def_t* objdef;
	
	//! the type of the consumer
	tmedia_type_t type;
	//! full description (usefull for debugging)
	const char* desc;

	int (*set_param) (tmedia_consumer_t* , const tmedia_param_t*);
    int (*get_param) (tmedia_consumer_t* , tmedia_param_t*);
	int (* prepare) (tmedia_consumer_t*, const tmedia_codec_t* );
	int (* start) (tmedia_consumer_t* );
	int (* consume) (tmedia_consumer_t*, const void* buffer, tsk_size_t size, tsk_object_t* proto_hdr);
	int (* pause) (tmedia_consumer_t* );
	int (* stop) (tmedia_consumer_t* );
}
tmedia_consumer_plugin_def_t;

#define TMEDIA_DECLARE_CONSUMER tmedia_consumer_t __consumer__

TINYMEDIA_API tmedia_consumer_t* tmedia_consumer_create(tmedia_type_t type, uint64_t session_id);
TINYMEDIA_API int tmedia_consumer_init(tmedia_consumer_t* self);
TINYMEDIA_API int tmedia_consumer_set_param(tmedia_consumer_t *self, const tmedia_param_t* param);
TINYMEDIA_API int tmedia_consumer_get_param(tmedia_consumer_t *self, tmedia_param_t* param);
TINYMEDIA_API int tmedia_consumer_prepare(tmedia_consumer_t *self, const tmedia_codec_t* codec);
TINYMEDIA_API int tmedia_consumer_start(tmedia_consumer_t *self);
TINYMEDIA_API int tmedia_consumer_consume(tmedia_consumer_t* self, const void* buffer, tsk_size_t size, tsk_object_t* proto_hdr);
TINYMEDIA_API int tmedia_consumer_pause(tmedia_consumer_t *self);
TINYMEDIA_API int tmedia_consumer_stop(tmedia_consumer_t *self);
TINYMEDIA_API int tmedia_consumer_deinit(tmedia_consumer_t* self);

TINYMEDIA_API int tmedia_consumer_plugin_register(const tmedia_consumer_plugin_def_t* plugin);
TINYMEDIA_API int tmedia_consumer_plugin_unregister(const tmedia_consumer_plugin_def_t* plugin);
TINYMEDIA_API int tmedia_consumer_plugin_unregister_by_type(tmedia_type_t type);

TMEDIA_END_DECLS

#endif /* TINYMEDIA_CONSUMER_H */
