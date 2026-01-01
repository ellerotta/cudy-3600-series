/***********************************************************************
 *
 *  Copyright (c) 2021  Broadcom Corporation
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
#ifdef SUPPORT_VXLAN_TUNNEL_TR181

#include "cms_msg.h"
#include "cms_util.h"

#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut_system.h"

CmsRet stl_dev2VxlanObject(_Dev2VxlanObject *obj,
                            const InstanceIdStack *iidStack)
{
   cmsLog_debug("Enter");
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2VxlanTunnelObject(_Dev2VxlanTunnelObject *obj,
                                 const InstanceIdStack *iidStack)
{
   cmsLog_debug("Enter");
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2VxlanTunnelStatsObject(_Dev2VxlanTunnelStatsObject *obj,
                                       const InstanceIdStack *iidStack)
{
   CmsRet ret = CMSRET_SUCCESS;
   cmsLog_debug("Enter");
   InstanceIdStack iidStackChild = EMPTY_INSTANCE_ID_STACK;
   Dev2VxlanTunnelInterfaceStatsObject *tunnelIfStatsObj = NULL;

   if (obj == NULL)
   {
      return ret;
   }
   
   obj->keepAliveSent = 0;
   obj->keepAliveReceived = 0;
   obj->bytesSent = 0;
   obj->bytesReceived = 0;
   obj->packetsSent = 0;
   obj->packetsReceived = 0;
   obj->errorsSent = 0;
   obj->errorsReceived = 0;

   while (cmsObj_getNextInSubTree(MDMOID_DEV2_VXLAN_TUNNEL_INTERFACE_STATS, iidStack, &iidStackChild,
                 (void **)&tunnelIfStatsObj) == CMSRET_SUCCESS)
   {
      obj->bytesSent += tunnelIfStatsObj->bytesSent;
      obj->bytesReceived += tunnelIfStatsObj->bytesReceived;
      obj->packetsSent += tunnelIfStatsObj->packetsSent;
      obj->packetsReceived += tunnelIfStatsObj->packetsReceived;
      obj->errorsSent += tunnelIfStatsObj->errorsSent;
      obj->errorsReceived += tunnelIfStatsObj->errorsReceived;

      cmsObj_free((void **) &tunnelIfStatsObj);
   }

   return ret;
}

CmsRet stl_dev2VxlanTunnelInterfaceObject(_Dev2VxlanTunnelInterfaceObject *obj,
                                            const InstanceIdStack *iidStack)
{
   cmsLog_debug("Enter");
   IF_OBJ_NOT_NULL_GET_LASTCHANGE(obj);
   return CMSRET_SUCCESS;
}

CmsRet stl_dev2VxlanTunnelInterfaceStatsObject(_Dev2VxlanTunnelInterfaceStatsObject *obj,
                                            const InstanceIdStack *iidStack)
{
   CmsRet ret = CMSRET_SUCCESS;
   Dev2VxlanTunnelInterfaceObject *vxlanTunnelIntf = NULL;

   cmsLog_debug("Enter");

   if (obj == NULL)
      return ret;

   if ((ret = cmsObj_get(MDMOID_DEV2_VXLAN_TUNNEL_INTERFACE,
                                 iidStack, OGF_NO_VALUE_UPDATE,
                                 (void **) &vxlanTunnelIntf)) == CMSRET_SUCCESS)
   {
      UINT64 multicastBytesReceived = 0;
      UINT64 multicastPacketsReceived = 0;
      UINT64 unicastPacketsReceived = 0;
      UINT64 broadcastPacketsReceived = 0;
      UINT64 multicastBytesSent = 0;
      UINT64 multicastPacketsSent = 0;
      UINT64 unicastPacketsSent = 0;
      UINT64 broadcastPacketsSent = 0;
      UINT64 bytesReceived = 0;
      UINT64 packetsReceived = 0;
      UINT64 errorsReceived = 0;
      UINT64 discardPacketsReceived = 0;
      UINT64 bytesSent = 0;
      UINT64 packetsSent = 0;
      UINT64 errorsSent = 0;
      UINT64 discardPacketsSent = 0;

      rut_getIntfStats_uint64(vxlanTunnelIntf->name,
                          &bytesReceived, &packetsReceived,
                          &multicastBytesReceived, &multicastPacketsReceived,
                          &unicastPacketsReceived, &broadcastPacketsReceived,
                          &errorsReceived, &discardPacketsReceived,
                          &bytesSent, &packetsSent,
                          &multicastBytesSent, &multicastPacketsSent,
                          &unicastPacketsSent, &broadcastPacketsSent,
                          &errorsSent, &discardPacketsSent);

      obj->bytesSent = bytesSent;
      obj->bytesReceived = bytesReceived;
      obj->packetsSent = packetsSent;
      obj->packetsReceived = packetsReceived;
      obj->errorsSent = (UINT32)errorsSent;
      obj->errorsReceived = (UINT32)errorsReceived;
      cmsObj_free((void **) &vxlanTunnelIntf);
   }

   return ret;
}

#endif /* SUPPORT_VXLAN_TUNNEL_TR181 */





