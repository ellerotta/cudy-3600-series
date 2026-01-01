/***********************************************************************
 *
 *
<:copyright-BRCM:2019:proprietary:standard

   Copyright (c) 2019 Broadcom
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


/*!\file zbus_misc.c
 * \brief Miscellaneous zbus functions
 */

// TODO: most of the functions in this file are outbound event related
// functions.  Maybe rename this file to zbus_events.c?


#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "bcm_retcodes.h"
#include "bcm_ulog.h"
#include "bcm_zbus_intf.h"
#include "cms_msg.h"
#include "cms_msg_pubsub.h"
#include "cms_util.h"
#include "bcm_generic_hal_utils.h"


// Only used by sysdir_notify (part of system directory server).
BcmRet zbus_out_notifyEvent(const char *compName, UINT32 eventType, UINT32 eid,
                            const char *strDataOut)
{
   const ZbusAddr *zbusAddr=NULL;
   BcmRet ret;

   bcmuLog_notice("Entered: compName=%s eventType=%d eid=%d strData=%s",
                 compName, eventType, eid, strDataOut);

   zbusAddr = zbusIntf_componentNameToZbusAddr(compName);
   if (zbusAddr == NULL)
   {
      bcmuLog_error("Could not get zbus addr for %s", compName);
      return BCMRET_INTERNAL_ERROR;
   }

   ret = bus_out_notifyEvent(zbusAddr, eventType, eid, strDataOut);
   return ret;
}

BcmRet zbus_out_subscribeEvent(const char *compName,
                               const CmsMsgHeader *msgReq,
                               CmsMsgHeader **msgResp)
{
   BcmRet ret;
   char *strReq = NULL;
   char *strResp = NULL;
   UINT32 eventType = 0;
   UINT32 flags = 0;

   if (msgReq == NULL)
   {
       bcmuLog_error("msgReq is NULL");
       return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_notice("Entered: compName=%s msg->type=0x%x wordData=%d",
                  compName, msgReq->type, msgReq->wordData);
   eventType = msgReq->wordData;
   if (msgReq->type == CMS_MSG_SUBSCRIBE_EVENT)
   {
      // For subscribe, msgResp must be provided
      if (msgResp == NULL)
      {
          bcmuLog_error("msgResp is NULL");
          return BCMRET_INVALID_ARGUMENTS;
      }
   }
   else
   {
      flags = ZBUS_FLAG_UNSUBSCRIBE;
   }

   strReq = zbusUtil_createOutboundEventDataStr(msgReq);
   if (strReq == NULL)
   {
      return BCMRET_INTERNAL_ERROR;
   }

   ret = bus_out_subscribeEvent(compName, msgReq->src, eventType, flags,
                                strReq, &strResp);
   if (ret == BCMRET_SUCCESS)
   {
      if (msgReq->type == CMS_MSG_SUBSCRIBE_EVENT)
      {
         // convert strResp back to binary msgResp and fill in header
         *msgResp = zbusUtil_createEventRespMsg(msgReq, strResp);
         ret = (*msgResp == NULL) ? BCMRET_INTERNAL_ERROR : BCMRET_SUCCESS;
      }
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(strReq);
   CMSMEM_FREE_BUF_AND_NULL_PTR(strResp);
   return ret;
}

BcmRet zbus_out_subscribeNamespaces(const char *compName, CmsEntityId eid,
                                    UINT32 flags, char **strResult)
{
   BcmRet ret;
   char *result=NULL;

   // flags 0: subscribe, or ZBUS_FLAG_UNSUBSCRIBE,
   bcmuLog_notice("Entered: compName=%s flags=0x%x", compName, flags);

   ret = bus_out_subscribeEvent(compName, eid, PUBSUB_EVENT_NAMESPACE, flags,
                                "", &result);
   if ((flags & ZBUS_FLAG_UNSUBSCRIBE) == 0)
   {
      // Subscribe gets result string.  Caller will free.
      *strResult = result;
   }
   else
   {
      // Unsubscribe should not return any data, not even empty string.
      CMSMEM_FREE_BUF_AND_NULL_PTR(result);
   }
   return ret;
}


BcmRet zbus_out_getNamespaces(const char *strReq, char **strResult)
{
   BcmRet ret=BCMRET_INVALID_ARGUMENTS;

   bcmuLog_notice("Entered: strReq=%s", strReq);

   ret = bus_out_getEventStatus(PUBSUB_EVENT_NAMESPACE, strReq, strResult);
   return ret;
}

BcmRet zbus_out_getEventStatus(const CmsMsgHeader *msgReq, CmsMsgHeader **msgResp)
{
   UINT32 eventType = msgReq->wordData;
   BcmRet ret;
   char *strReq = NULL;
   char *strResp = NULL;

/* Null-checking "msgReq" suggests that it may be null,
   but it has already been dereferenced on all paths leading to the check.*/
#if 0
   if ( NULL == msgReq || NULL == msgResp)
   {
       bcmuLog_error("one or more NULL input args");
       return BCMRET_INVALID_ARGUMENTS;
   }
#endif

   strReq = zbusUtil_createOutboundEventDataStr(msgReq);
   if (strReq == NULL)
   {
       return BCMRET_INTERNAL_ERROR;
   }

   ret = zbus_out_getEventStatusGeneric(eventType, strReq, &strResp);
   if (ret == BCMRET_SUCCESS)
   {
      // convert strResp back to binary msgResp and fill in header
      *msgResp = zbusUtil_createEventRespMsg(msgReq, strResp);
      ret = (*msgResp == NULL) ? BCMRET_INTERNAL_ERROR : BCMRET_SUCCESS;
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(strReq);
   CMSMEM_FREE_BUF_AND_NULL_PTR(strResp);
   return ret;
}

BcmRet zbus_out_getEventStatusGeneric(UINT32 eventType,
                                      const char *strReq, char **strResp)
{
   BcmRet ret;
   ret = bus_out_getEventStatus(eventType, strReq, strResp);
   return ret;
}

BcmRet zbus_out_queryEventStatus(UINT32 flags, const CmsMsgHeader *msgReq,
                                 CmsMsgHeader **msgResp)
{
   char *strResp = NULL;
   BcmRet ret;

   if (msgReq == NULL || msgResp == NULL)
   {
       bcmuLog_error("one or more NULL input args");
       return BCMRET_INVALID_ARGUMENTS;
   }

   // The query string is right after the request msg header
   ret = zbus_out_queryEventStatusGeneric(msgReq->wordData, flags,
                                          (char *)(msgReq+1), &strResp);
   if (ret == BCMRET_SUCCESS)
   {
      // repackage strResp into a response CMS msg.
      *msgResp = zbusUtil_createEventRespMsg(msgReq, strResp);
      ret = (*msgResp == NULL) ? BCMRET_INTERNAL_ERROR : BCMRET_SUCCESS;
   }

   if (strResp)
      CMSMEM_FREE_BUF_AND_NULL_PTR(strResp);

   return ret;
}

BcmRet zbus_out_queryEventStatusGeneric(UINT32 eventType, UINT32 flags,
                                 const char *strReq, char **strResp)
{
   BcmRet ret;
   ret = bus_out_queryEventStatus(eventType, flags, strReq, strResp);
   return ret;
}

BcmRet zbus_out_publishEvent(const char *compName, const CmsMsgHeader *msg)
{
   char *strDataOut = NULL;
   UINT32 src = 0;
   UINT32 eventType = 0;
   UINT32 flags = 0;
   BcmRet ret=BCMRET_INVALID_ARGUMENTS;

   if (compName == NULL || msg == NULL)
   {
      bcmuLog_error("NULL input args");
      return BCMRET_INVALID_ARGUMENTS;
   }

   // msg->type specifies whether it is PUBLISH or UNPUBLISH
   bcmuLog_notice("Entered: compName=%s msg->type=0x%x", compName, msg->type);

   // convert CMS msg to generic params
   strDataOut = zbusUtil_createOutboundEventDataStr(msg);
   if (strDataOut == NULL)
   {
      return BCMRET_INTERNAL_ERROR;
   }

   src = msg->src;
   eventType = msg->wordData;
   if (msg->type == CMS_MSG_UNPUBLISH_EVENT)
      flags |= ZBUS_FLAG_UNPUBLISH;

   // Call the lower layer bus function, which only accepts generic args
   ret = bus_out_publishEvent(compName, src, eventType, flags, strDataOut);

   CMSMEM_FREE_BUF_AND_NULL_PTR(strDataOut);

   bcmuLog_debug("returning %d", ret);
   return ret;
}


BcmRet zbus_out_publishEventGeneric(const char *compName, UINT32 src,
                                    UINT32 eventType, UINT32 flags,
                                    const char *strDataOut)
{
   return (bus_out_publishEvent(compName, src,
                                eventType, flags,
                                strDataOut));
}

BcmRet zbus_out_proxyGetParameterValues(const CmsMsgHeader *msgReq, CmsMsgHeader **msgResp)
{
   BcmRet ret;
   const char *compName;
   const char *fullpath;
   const char *fullpathArray[1];
   UINT32 flags = 0;
   const ZbusAddr *destAddr;
   BcmGenericParamInfo *paramInfoArray = NULL;
   UINT32 numParamInfos = 0;
   CmsMsgMdProxyGetParamRequest *req;

   req = (CmsMsgMdProxyGetParamRequest *)(msgReq + 1);
   compName = req->destCompName;
   fullpath = req->fullPath;
   flags = req->flags;

   bcmuLog_notice("Entered: compName=%s fullpath=%s", compName, fullpath);
   fullpathArray[0] = fullpath;

   destAddr = zbusIntf_componentNameToZbusAddr(compName);
   if (destAddr == NULL)
   {
      bcmuLog_error("Unrecognized compName %s\n", compName);
      return BCMRET_INVALID_ARGUMENTS;
   }

   ret = zbus_out_getParameterValues(destAddr, fullpathArray, 1,
                                     TRUE, flags,
                                     &paramInfoArray, &numParamInfos);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("zbus_out_getParameterValues failed, ret=%d", ret);
      return ret;
   }

   if (numParamInfos != 1)
   {
      bcmuLog_notice("expect one parameter returned, got %d\n", numParamInfos);
   }

   *msgResp = zbusUtil_createProxyGetParamRespMsg(msgReq, paramInfoArray[0].fullpath,
                                                  paramInfoArray[0].type,
                                                  paramInfoArray[0].value);

   ret = (*msgResp == NULL) ? BCMRET_INTERNAL_ERROR : BCMRET_SUCCESS;

   cmsUtl_freeParamInfoArray(&paramInfoArray, numParamInfos); 
   return ret;
}

// compose firewall string based on fwctlBlk
// See:
// rutIpt_ipFilterInRunIptables_dev2
// parseFirewallExceptionObject_dev2
// rut2_iptables.c rcl2_firwall.c dal2_sec.c
BcmRet composeFirewallStr(const FirewallCtlMsgBody *fwCtl,
                          char *str, UINT32 len)
{
   char src[BUFLEN_256]={0};
   char dst[BUFLEN_256]={0};
   char sport[BUFLEN_40]={0};
   char dport[BUFLEN_40]={0};
   char action;  // Insert or Delete or Flush (there is also Append)
   const char *ipt = (fwCtl->isIpv6) ? "ip6tables" : "iptables";
   const char *protocol;

   /* Action */
   if (fwCtl->enable)
   {
      /* Insert */
      action = 'I';
   }
   else
   {
      if (strcmp(fwCtl->name, "VoiceFilter"))
      {
         /* Delete */
         action = 'D';
      }
      else
      {
         /* DELETE ALL requested - Flush */
         action = 'F';
      }
   }

   if (strcmp(fwCtl->protocol, "TCP") == 0)
   {
      protocol = " -p tcp";
   }
   else if (strcmp(fwCtl->protocol, "UDP") == 0)
   {
      protocol = " -p udp";
   }
   else if (strcmp(fwCtl->protocol, "TCP or UDP") == 0)
   {
      /* TODO: This option was supported in the classic CMS API.  Maybe we
       *       should support here as well.  Hardcoding for now. */
      protocol = " -p udp";
   }
   else
   {
      bcmuLog_error("unsupported protocol %s", fwCtl->protocol);
      return BCMRET_INVALID_ARGUMENTS;
   }

   if (fwCtl->sourceIPAddress[0] != '\0')
   {
      if (fwCtl->sourceNetMask[0] != '\0')
      {
         if (fwCtl->isIpv6)
         {
             bcmuLog_error("src netmask not allowed in IPv6");
             return BCMRET_INVALID_ARGUMENTS;
         }
         snprintf(src, sizeof(src), " -s %s/%d", fwCtl->sourceIPAddress,
                  cmsNet_getLeftMostOneBitsInMask(fwCtl->sourceNetMask));
      }
      else
      {
          snprintf(src, sizeof(src), " -s %s", fwCtl->sourceIPAddress);
      }
   }

   if (fwCtl->destinationIPAddress[0] != '\0')
   {
      if (fwCtl->destinationNetMask[0] != '\0')
      {
         if (fwCtl->isIpv6)
         {
             bcmuLog_error("dst netmask not allowed in IPv6");
             return BCMRET_INVALID_ARGUMENTS;
         }
         snprintf(dst, sizeof(dst), " -d %s/%d", fwCtl->destinationIPAddress,
                  cmsNet_getLeftMostOneBitsInMask(fwCtl->destinationNetMask));
      }
      else
      {
          snprintf(dst, sizeof(dst), " -d %s", fwCtl->destinationIPAddress);
      }
   }

   if (fwCtl->sourcePort != 0)
   {
      snprintf(sport, sizeof(sport), " --sport %d", fwCtl->sourcePort);
   }

   if (fwCtl->destinationPort != 0)
   {
      snprintf(dport, sizeof(dport), " --dport %d", fwCtl->destinationPort);
   }

   /* fwCtl->name is not needed because firewalld will identify our rule based on
    * our component name. */
   snprintf(str, len,
            "%s -%c INPUT -i %s%s%s%s%s%s -j ACCEPT",
            ipt, action, fwCtl->intfName,  protocol, src, sport, dst, dport);

   bcmuLog_notice("iptablesStr=%s", str);

   return BCMRET_SUCCESS;
}


BcmRet zbus_out_firewallCtl(const char *compName, const CmsMsgHeader *msgReq)
{
   const ZbusAddr *zbusAddr=NULL;
   char ruleStr[1024]={0};
   FirewallCtlMsgBody *fwCtl = (FirewallCtlMsgBody *)(msgReq+1);
   BcmRet ret=BCMRET_INVALID_ARGUMENTS;

   if (msgReq == NULL)
   {
      bcmuLog_error("NULL input args");
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_notice("Entered:");

   zbusAddr = zbusIntf_componentNameToZbusAddr(BDK_APP_FIREWALLD);
   if (zbusAddr == NULL)
   {
      bcmuLog_error("Could not get zbus addr for firewalld");
      return ret;
   }

   // Do we need to add to FORWARD chain as well?
   // the function below only creates string for ACCEPT
   ret = composeFirewallStr(fwCtl, ruleStr, sizeof(ruleStr));
   BCM_RETURN_IF_NOT_SUCCESS(ret);

   ret = bus_out_firewallCtl(zbusAddr, compName, ruleStr);
   return ret;
}


BcmRet zbus_out_mdmOp(const char *destCompName,
                      const char *op, const char *arg, char **retData)
{
   const ZbusAddr *zbusAddr=NULL;
   BcmRet ret=BCMRET_INVALID_ARGUMENTS;

   bcmuLog_notice("Entered: destCompName=%s op=%s", destCompName, op);

   zbusAddr = zbusIntf_componentNameToZbusAddr(destCompName);
   if (zbusAddr == NULL)
   {
      bcmuLog_error("Could not get zbus addr for %s", destCompName);
      return ret;
   }

   ret = bus_out_mdmOp(zbusAddr, op, arg, retData);
   return ret;
}

