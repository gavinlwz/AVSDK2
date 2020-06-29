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

/**@file tsdp_header_O.h
 * @brief SDP "o=" header (Origin).
 *
 *
 *
 * @date Created: Oat Nov 8 16:54:58 2009 mdiop
 */
#ifndef _TSDP_HEADER_O_H_
#define _TSDP_HEADER_O_H_

#include "tinysdp_config.h"
#include "tinysdp/headers/tsdp_header.h"

TSDP_BEGIN_DECLS


#define TSDP_HEADER_O_VA_ARGS(nettype, addrtype, addr)			tsdp_header_O_def_t, (const char*)nettype, (const char*)addrtype, (const char*)addr

#define TSDP_HEADER_O_SESS_ID_DEFAULT		123456
#define TSDP_HEADER_O_SESS_VERSION_DEFAULT	678901

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @struct	
///
/// @brief	SDP "o=" header (Origin).
///  The "o=" field gives the originator of the session (her username and
///     the address of the user's host) plus a session identifier and version number.
///
/// @par ABNF : u=username SP
/// sess-id SP sess-version SP nettype SP addrtype SP unicast-address
///
/// username	=  	non-ws-string 
/// sess-id	=  	1*DIGIT
/// sess-version	=  	1*DIGIT
/// nettype	=  	token 
/// addrtype	=  	token
/// unicast-address = FQDN
/// 	
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct tsdp_header_O_s
{	
	TSDP_DECLARE_HEADER;
	/** <nettype> is a text string giving the type of network.  Initially
	"IN" is defined to have the meaning "Internet", but other values
	MAY be registered in the future (see Section 8 of RFC 4566)*/
	char* nettype;
	/**<addrtype> is a text string giving the type of the address that
	follows.  Initially "IP4" and "IP6" are defined, but other values
	MAY be registered in the future (see Section 8 of RFC 4566)*/
	char* addrtype;
	/** <unicast-address> is the address of the machine from which the
	session was created.  For an address type of IP4, this is either
	the fully qualified domain name of the machine or the dotted-
	decimal representation of the IP version 4 address of the machine.
	For an address type of IP6, this is either the fully qualified
	domain name of the machine or the compressed textual
	representation of the IP version 6 address of the machine.  For
	both IP4 and IP6, the fully qualified domain name is the form that
	SHOULD be given unless this is unavailable, in which case the
	globally unique address MAY be substituted.  A local IP address
	MUST NOT be used in any context where the SDP description might
	leave the scope in which the address is meaningful (for example, a
	local address MUST NOT be included in an application-level
	referral that might leave the scope)*/
	char* addr;
}
tsdp_header_O_t;

TINYSDP_API tsdp_header_O_t* tsdp_header_O_create(const char* username, uint32_t sess_id, uint32_t sess_version, const char* nettype, const char* addrtype, const char* addr);


TINYSDP_API tsdp_header_O_t *tsdp_header_O_parse(const char *data);

TINYSDP_GEXTERN const tsk_object_def_t *tsdp_header_O_def_t;

TSDP_END_DECLS

#endif /* _TSDP_HEADER_O_H_ */

