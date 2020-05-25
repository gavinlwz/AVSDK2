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

/**@file tmedia_imageattr.h
 * @brief 'image-attr' parser as per RFC 6236
 *
 *
 *
 */
#ifndef TINYMEDIA_imageattr_H
#define TINYMEDIA_imageattr_H

#include "tinymedia_config.h"

#include "tmedia_common.h"

TMEDIA_BEGIN_DECLS

#define TMEDIA_imageattr_ARRAY_MAX_SIZE	16

typedef int32_t xyvalue_t;
typedef double qvalue_t;
typedef double spvalue_t;

typedef struct tmedia_imageattr_srange_xs
{
	unsigned is_range:1;
	union{
		struct{
			spvalue_t start;
			spvalue_t end;
		}range;
		struct{
			spvalue_t values[TMEDIA_imageattr_ARRAY_MAX_SIZE + 1];
			tsk_size_t count;
		}array;
	};
}
tmedia_imageattr_srange_xt;

typedef struct tmedia_imageattr_xyrange_xs
{
	unsigned is_range:1;
	union{
		struct{
			xyvalue_t start;
			xyvalue_t step;
			xyvalue_t end;
		}range;
		struct{
			xyvalue_t values[TMEDIA_imageattr_ARRAY_MAX_SIZE + 1];
			tsk_size_t count;
		}array;
	};
}
tmedia_imageattr_xyrange_xt;

typedef struct tmedia_imageattr_set_xs
{
	tmedia_imageattr_xyrange_xt xrange;
	tmedia_imageattr_xyrange_xt yrange;
	tmedia_imageattr_srange_xt srange;
	struct{
		unsigned is_present:1;
		spvalue_t start;
		spvalue_t end;
	}prange;
	qvalue_t qvalue;
}
tmedia_imageattr_set_xt;

typedef struct tmedia_imageattr_xs
{
	struct{
		tmedia_imageattr_set_xt sets[TMEDIA_imageattr_ARRAY_MAX_SIZE + 1];
		tsk_size_t count;
	}send;
	struct{
		tmedia_imageattr_set_xt sets[TMEDIA_imageattr_ARRAY_MAX_SIZE + 1];
		tsk_size_t count;
	}recv;
}
tmedia_imageattr_xt;

TINYMEDIA_API int tmedia_imageattr_parse(tmedia_imageattr_xt* self, const void* in_data, tsk_size_t in_size);

TMEDIA_END_DECLS

#endif /* TINYMEDIA_imageattr_H */
