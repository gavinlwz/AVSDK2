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

/**@file tmedia_content.h
 * @brief Base content object.
 *
 *
 *
 */
#ifndef TINYMEDIA_CONTENT_H
#define TINYMEDIA_CONTENT_H

#include "tinymedia_config.h"

#include "tsk_buffer.h"
#include "tsk_list.h"
#include "tsk_params.h"

TMEDIA_BEGIN_DECLS

/**Max number of plugins (content types) we can create */
#define TMEDIA_CONTENT_MAX_PLUGINS			0x0F

/** cast any pointer to @ref tmedia_content_t* object */
#define TMEDIA_CONTENT(self)		((tmedia_content_t*)(self))

/** Base object for all contents */
typedef struct tmedia_content_s
{
	TSK_DECLARE_OBJECT;

	const char* type;
	//! plugin used to create the codec
	const struct tmedia_content_plugin_def_s* plugin;
}
tmedia_content_t;

/** Virtual table used to define a content plugin */
typedef struct tmedia_content_plugin_def_s
{
	//! object definition used to create an instance of the codec
	const tsk_object_def_t* objdef;

	//! e.g. 'message/CPIM'
	const char* type;

	int (*parse) (tmedia_content_t*, const void* in_data, tsk_size_t in_size);
	tsk_buffer_t* (*get_data) (tmedia_content_t*);
}
tmedia_content_plugin_def_t;

/** List of @ref tmedia_codec_t elements */
typedef tsk_list_t tmedia_contents_L_t;

/**< Declare base class as content */
#define TMEDIA_DECLARE_CONTENT tmedia_content_t __content__

TINYMEDIA_API int tmedia_content_plugin_register(const char* type, const tmedia_content_plugin_def_t* plugin);
TINYMEDIA_API int tmedia_content_plugin_unregister(const char* type, const tmedia_content_plugin_def_t* plugin);
TINYMEDIA_API int tmedia_content_plugin_unregister_all();

TINYMEDIA_API tmedia_content_t* tmedia_content_create(const char* type);
TINYMEDIA_API tmedia_content_t* tmedia_content_parse(const void* data, tsk_size_t size, const char* type);

TINYMEDIA_API int tmedia_content_init(tmedia_content_t* self);
TINYMEDIA_API int tmedia_content_deinit(tmedia_content_t* self);
TINYMEDIA_API tsk_buffer_t* tmedia_content_get_data(tmedia_content_t* self);

/** dummy content */
typedef struct tmedia_content_dummy_s
{
	TMEDIA_DECLARE_CONTENT;
	
	tsk_buffer_t* data;
}
tmedia_content_dummy_t;

#define TMEDIA_CONTENT_DUMMY(self)	((tmedia_content_dummy_t*)(self))
#define TMEDIA_CONTENT_IS_DUMMY(self) ( (self) && (TMEDIA_CONTENT((self))->plugin==tmedia_content_dummy_plugin_def_t) )

TINYMEDIA_GEXTERN const tmedia_content_plugin_def_t *tmedia_content_dummy_plugin_def_t;


/** content header */
typedef struct tmedia_content_header_s
{
	TSK_DECLARE_OBJECT;

	char* name;
	char* value;
	tsk_params_L_t* params;
}
tmedia_content_header_t;

tmedia_content_header_t* tmedia_content_header_create(const char* name, const char* value);
int tmedia_content_header_deinit(tmedia_content_header_t* self);
char* tmedia_content_header_tostring(const tmedia_content_header_t* self);

#define TMEDIA_CONTENT_HEADER(self)	((tmedia_content_header_t*)(self))
#define TMEDIA_DECLARE_CONTENT_HEADER tmedia_content_header_t __content_header__
typedef tsk_list_t tmedia_content_headers_L_t;

TINYMEDIA_GEXTERN const tsk_object_def_t *tmedia_content_header_def_t;

TMEDIA_END_DECLS

#endif /* TINYMEDIA_CONTENT_H */
