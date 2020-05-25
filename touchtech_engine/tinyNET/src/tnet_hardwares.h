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
/**@file tnet_hardwares.h
 * @brief List of Hardware types as assigned by the IANA.
 *
 *
 *

 */

#ifndef TNET_HARDWARES_H
#define TNET_HARDWARES_H

#include "tinynet_config.h"

TNET_BEGIN_DECLS

/**
*	List of Hardware types as assigned by the IANA.
*	See RFC 1340, 826 and... for more information.
*/
typedef enum tnet_hardware_type_e
{
	tnet_htype_Ethernet_10Mb = 1, /**<    Ethernet (10Mb) */
	tnet_htype_Ethernet_3Mb = 2, /**<     Experimental Ethernet (3Mb) */
	tnet_htype_AX_25 = 3, /**<     Amateur Radio AX.25 */
	tnet_htype_Token_Ring = 4, /**<     Proteon ProNET Token Ring */
	tnet_htype_Chaos = 5, /**<     Chaos */
	tnet_htype_IEEE_802_Networks = 6, /**<     IEEE 802 Networks */
	tnet_htype_ARCNET = 7, /**<     ARCNET */
	tnet_htype_Hyperchannel = 8, /**<     Hyperchannel */
	tnet_htype_Lanstar = 9, /**<     Lanstar	 */
	tnet_htype_Autonet_Short_Address  = 10, /**<     Autonet Short Address */
	tnet_htype_ALocalTalk = 11, /**<     LocalTalk	 */
	tnet_htype_LocalNet= 12, /**<     LocalNet (IBM PCNet or SYTEK LocalNET)	 */
	tnet_htype_Ultra_link =  13, /**<     Ultra link */
	tnet_htype_SMDS = 14, /**<     SMDS	 */
	tnet_htype_Frame_Relay = 15, /**<     Frame Relay	 */
	tnet_htype_ATM = 16, /**<     Asynchronous Transmission Mode (ATM) */
}
tnet_hardware_type_t;

TNET_END_DECLS

#endif /* TNET_HARDWARES_H */
