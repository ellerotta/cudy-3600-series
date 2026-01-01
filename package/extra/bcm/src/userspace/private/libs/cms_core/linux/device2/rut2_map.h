/***********************************************************************
 *
 *  Copyright (c) 2014  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2014:proprietary:standard

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

#ifndef __RUT2_MAP_H__
#define __RUT2_MAP_H__

/*!\file rut2_map.h
 * \brief Helper functions for map-t and map-e.
 *
 * In most cases, the functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms_util.h"
#include "cms_qdm.h"

typedef struct
{
   UINT16 min, max;
} NaptBlock;

typedef struct
{
   NaptBlock *range;
   int numRanges;
} PortSet;

CmsRet rutMap_getDomainInfo(const InstanceIdStack *ruleiidStack, char *mechanism, char *brprefix, char *wanintf,
                            UBOOL8 *firewall, UINT32 *psidOffset, UINT32 *psidLen, UINT32 *psid);

CmsRet rutMap_parseDomainRuleInfo(const char *IPv6Prefix, const char *IPv4Prefix, UINT32 eaLen, UINT32 psidLen, UINT32 psid,
                                  char *ipv6AddrBuf, char *subnetCidr4, char *addr, UINT32 *ipv4MaskLen);

CmsRet rutMap_parseDhcp6cInfo(const char *sitePrefix, const char *IPv6Prefix, char *IPv4Prefix, UINT32 eaLen, UINT32 psidLen);

CmsRet rutMap_newPortSetRange(UINT32 psidOffset, UINT32 psidLen, UINT32 psid, PortSet **ps);

void rutMap_deletePortSetRange(PortSet *ps);

void rutMap_mapControl(const char *ipIntfFullPath);

UBOOL8 rutTunnel_isMapDynamic(const char *wanIpIntfPath, char *mechanism);

#endif  /* __RUT2_MAP_H__ */

