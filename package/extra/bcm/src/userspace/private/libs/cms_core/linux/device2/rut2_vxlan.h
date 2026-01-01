/***********************************************************************
 *
 *  Copyright (c) 2006-2021  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2021:proprietary:standard

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:>
 *
 ************************************************************************/

#ifndef __RUT2_VXLAN_H__
#define __RUT2_VXLAN_H__

#ifdef SUPPORT_VXLAN_TUNNEL_TR181

/*!\file rut2_vxlan.h
 * \brief System level interface functions for VxLAN functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms.h"

typedef enum
{
   VXLAN_COMMAND_NONE     = 0,
   VXLAN_COMMAND_CONFIG   = 1,
   VXLAN_COMMAND_UNCONFIG = 2
} VXLANCommandType;
#define VXLAN_CRITERION_UNUSED        -1
#define VXLAN_RESULT_NO_CHANGE        -1
#define VXLAN_RESULT_AUTO_MARK        -2

/* only support 4 tunnel interface */
#define MAX_VXLAN_TUNNELS_INTF   4

/** Create a VxLan tunnel interface.
 *
 * @param tunnel interface config.
 *
 * @return CmsRet enum.
 */
CmsRet rutVxlan_create_tunnel_intf_dev2(char *name, UBOOL8 isIpv6, SINT32 vni, UINT32 rport,
          UINT32 sport, char *remoteEP, char *dev, UINT32 timeout, UINT32 thrsd,
          SINT32 zeroCsumTx, SINT32 zeroCsumRx);

/** Delete a VxLan tunnel interface.
 *
 * @param tunnel interface config.
 *
 * @return CmsRet enum.
 */
CmsRet rutVxlan_delete_tunnel_intf_dev2(char *name, UBOOL8 isIpv6, UINT32 rport, char *dev);


/** activate/deactive a vxlan tunnel interface.
 *
 * @param ifname: VxLan tunnel interface name.
 * @param active: active/deactive.
 *
 * @return CmsRet enum.
 */
CmsRet rutVxlan_activateTunnelIf_dev2(const char *ifname, UBOOL8 active);

/** get vxlan tunnel interface instance number.
 *
 * @param num: tunnel intf number.
 *
 * @return intf number.
 */
CmsRet rutVxlan_getTunnelIfNum_dev2(UINT32 *num);

/** update vxlan tunnel interface instance number.
 *
 * @param delta: 1 or -1.
 *
 * @return none.
 */
void rutVxlan_updateTunnelIfNum_dev2(SINT32 delta);

#endif /* SUPPORT_VXLAN_TUNNEL_TR181 */

#endif // __RUT2_VXLAN_H__
