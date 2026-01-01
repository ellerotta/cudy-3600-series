/***********************************************************************
 *
 *
<:copyright-BRCM:2020:proprietary:standard

   Copyright (c) 2020 Broadcom
   All Rights Reserved

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
#ifdef DMP_DEVICE2_BASELINE_1

#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "cms_msg.h"
#include "cms_msg_pubsub.h"
#include "cms_mdm.h"
#include "mdm.h"
#include "mdm_private.h"
#include "rut_system.h"
#include "rut_lan.h"
#include "rut_wan.h"
#include "qdm_ipintf.h"


CmsRet rutTr69c_sendGetEventStatus(const char *ifName, char *ipAddress)
{
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   CmsRet r2;
   char msgBuf[sizeof(CmsMsgHeader)+sizeof(PubSubInterfaceMsgBody)]={0};
   CmsMsgHeader *msg = (CmsMsgHeader *) msgBuf;
   CmsMsgHeader *msgResp = NULL;
   PubSubInterfaceMsgBody *body = (PubSubInterfaceMsgBody *)(msg+1);
   void *msgHandle = cmsMdm_getThreadMsgHandle();

   msg->type = CMS_MSG_GET_EVENT_STATUS;
   msg->src = cmsMsg_getHandleEid(msgHandle);
   // ANY_WAN and LAN will be processed by tr69_md, real ifName will be 
   // forwarded to sys_directory in bcm_comp_md.c
   msg->dst = EID_TR69_MD;
   msg->flags_request = 1;
   msg->wordData = PUBSUB_EVENT_INTERFACE;
   msg->dataLength = sizeof(PubSubInterfaceMsgBody);

   // When we send out the request, we only need to fill in the intfName.
   strncpy(body->intfName, ifName, sizeof(body->intfName)-1);
   cmsLog_debug("Sending out GET_EVENT_STATUS for %s", ifName);

   r2 = cmsMsg_sendAndGetReplyBuf(msgHandle, msg, &msgResp);
   if (r2 == CMSRET_SUCCESS)
   {
      if ((msgResp->wordData == PUBSUB_EVENT_INTERFACE) &&
          (msgResp->dataLength >= sizeof(PubSubInterfaceMsgBody)))
      {
         body = (PubSubInterfaceMsgBody *)(msgResp+1);
         if (body->isIpv6Up)
         {
            strcpy(ipAddress, body->ipv6GlobalAddr);
            ret = CMSRET_SUCCESS;
         }
         else if (body->isIpv4Up)
         {
            strcpy(ipAddress, body->ipv4Addr);
            ret = CMSRET_SUCCESS;
         }
      }
      else
      {
         // We did not get a valid response, which probably means the specific
         // interface name is not present in sys_directory.
         ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
      }
      CMSMEM_FREE_BUF_AND_NULL_PTR(msgResp);
   }
   else
   {
      cmsLog_error("Send of GET_EVENT_STATUS msg failed, ret=%d", ret);
   }

   cmsLog_debug("Exit: ifname=%s ret=%d ipAddress=%s", ifName, ret, ipAddress);
   return ret;
}


// Return of CMSRET_SUCCESS means we found a LAN intf with IPv4 up, ipAddress will be filled in.
// Return of CMSRET_SUCCESS_OBJECT_UNCHANGED means no LAN intf found, or is not up,
// so ipAddress will not be filled in.
CmsRet rutTr69c_getDefaultLanIntfAddr(char *ipAddress)
{
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

   if (cmsMdm_isCmsClassic())
   {
      char ifName[CMS_IFNAME_LENGTH]={0};

      // In CMS TR181 mode, it is possible, but highly unlikely, that the
      // LAN interface has an IP address but is actually not up.
      if ((qdmIpIntf_getDefaultLanIntfNameLocked(ifName) == CMSRET_SUCCESS) &&
          (qdmIpIntf_getIpvxAddressByNameLocked(CMS_AF_SELECT_IPV4, ifName, ipAddress) == CMSRET_SUCCESS))
      {
         // LAN intf is up and we got the ipaddr
         if (cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, ipAddress))
         {
            // This should not happen.  Complain but keep going.
            cmsLog_error("%s marked as up, but has zero ipAddr %s",
                         ifName, ipAddress);
         }
         else if (!qdmIpIntf_isIpv4ServiceUpLocked_dev2(ifName, QDM_IPINTF_DIR_LAN))
         {
            cmsLog_debug("%s is not up", ifName);
         }
         else
         {
            // All good.  Found LAN intf, got ipv4 addr.
            ret = CMSRET_SUCCESS;
         }
      }
   }
   else
   {
      // BDK mode.
      // Do not run during mdm_init, basically, pretend LAN is down.
      // When tr69c starts, it will get the correct status.
      if (!mdmShmCtx->inMdmInit)
      {
         // Send getEvent status with ifname=MDMVS_LAN to tr69_md
         CmsRet r2 = rutTr69c_sendGetEventStatus(MDMVS_LAN, ipAddress);
         if (r2 == CMSRET_SUCCESS)
         {
            ret = r2;
         }
      }
   }

   cmsLog_debug("Exit: ret=%d ipAddress=%s", ret, ipAddress);
   return ret;
}


// ifName can be "ANY_WAN" or a specific wan intfName.
CmsRet rutTr69c_getRoutedAndConnectedWanIntfAddr(const char *bound_ifName,
                                                 char *ipAddress)
{
   char ifName[CMS_IFNAME_LENGTH]={0};
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

   cmsUtl_strcpy(ifName, bound_ifName);

   if (cmsMdm_isCmsClassic())
   {
      UBOOL8 found = TRUE;

      if (!cmsUtl_strcmp(ifName, MDMVS_ANY_WAN))
      {
         found = rutWan_findFirstIpvxRoutedAndConnected(CMS_AF_SELECT_IPVX, ifName);
      }

      if (found)
      {
         // check if up in IPv4 or IPv6 mode
         if (qdmIpIntf_isWanInterfaceUpLocked(ifName, TRUE) ||
             qdmIpIntf_isWanInterfaceUpLocked(ifName, FALSE))
         {
            // Interface is up.  Get ipaddr
            CmsRet r2 = qdmIpIntf_getIpvxAddressByNameLocked(CMS_AF_SELECT_IPVX,
                                                             ifName, ipAddress);
            if (r2 != CMSRET_SUCCESS)
            {
               // Weird, but keep the ret at OBJECT_UNCHANGED since we are not
               // able to get an ipAddress for this intf.
               cmsLog_error("Could not get IPAddr for %s even though it is UP!!", ifName);
            }
            else
            {
               // All good.  Found a routed and connected wan intf, and we got
               // the IPv4 or IPv6 addr
               ret = CMSRET_SUCCESS;
            }
         }
      }
   }
   else
   {
      // BDK mode.
      // Do not run during mdm_init, basically, pretend all WAN is down.
      // When tr69c starts, it will get the correct status.
      if (!mdmShmCtx->inMdmInit)
      {
         // Send getEvent status with ifname=ANY_WAN or real ifName to tr69_md
         CmsRet r2 = rutTr69c_sendGetEventStatus(ifName, ipAddress);
         if (r2 == CMSRET_SUCCESS)
         {
            ret = r2;
         }
      }
   }

   cmsLog_debug("Exit: ifName=%s ret=%d ipAddress=%s", ifName, ret, ipAddress);
   return ret;
}

#endif /* DMP_DEVICE2_BASELINE_1 */

