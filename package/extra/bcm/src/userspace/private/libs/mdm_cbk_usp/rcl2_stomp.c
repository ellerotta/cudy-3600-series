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


#ifdef DMP_DEVICE2_STOMPCONN_1

#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "rut_util.h"
#include "mdm.h"
#include "cms_msg_usp.h"


CmsRet rcl_dev2StompObject(_Dev2StompObject *newObj __attribute__((unused)),
                           const _Dev2StompObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

static CmsRet sendMsgChanged(const char *fullpath, int enable, int isDelete)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   StompConnectionChangedMsgBody *msgPayload = NULL;

   reqMsg = cmsMem_alloc(sizeof(CmsMsgHeader) +
                         sizeof(StompConnectionChangedMsgBody), ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = (CmsMsgType) CMS_MSG_STOMP_CONN_CONFIG_CHANGE;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = EID_OBUSPA;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(StompConnectionChangedMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (StompConnectionChangedMsgBody*)body;

   msgPayload->enable = enable;
   msgPayload->isDelete = isDelete;
   cmsUtl_strncpy(msgPayload->fullPath, fullpath, sizeof(msgPayload->fullPath));

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to send message to obuspa (ret=%d)", ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}

CmsRet rcl_dev2StompConnectionObject(_Dev2StompConnectionObject *newObj,
                                     const _Dev2StompConnectionObject *currObj,
                                     const InstanceIdStack *iidStack,
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
   MdmPathDescriptor pathDesc;
   char *fullpath = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumStompConnEntry_dev2(iidStack, 1);
      return ret;
   }

   memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
   pathDesc.oid = MDMOID_DEV2_STOMP_CONNECTION;
   pathDesc.iidStack = *iidStack;

   cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullpath);

   if (newObj && currObj)
   {
      /* notify obuspa only if any config changes */
      if (newObj->enable != currObj->enable || newObj->port != currObj->port ||
          cmsUtl_strcmp(newObj->virtualHost, currObj->virtualHost) ||
          cmsUtl_strcmp(newObj->host, currObj->host) ||
          cmsUtl_strcmp(newObj->username, currObj->username) ||
          cmsUtl_strcmp(newObj->password, currObj->password) ||
          newObj->enableEncryption != currObj->enableEncryption ||
#ifdef DMP_DEVICE2_STOMPHEARTBEAT_1
          newObj->enableHeartbeats != currObj->enableHeartbeats ||
          newObj->outgoingHeartbeat != currObj->outgoingHeartbeat ||
          newObj->incomingHeartbeat != currObj->incomingHeartbeat ||
#endif
          newObj->serverRetryInitialInterval != currObj->serverRetryInitialInterval ||
          newObj->serverRetryIntervalMultiplier != currObj->serverRetryIntervalMultiplier ||
          newObj->serverRetryMaxInterval != currObj->serverRetryMaxInterval)
      {
         ret = sendMsgChanged(fullpath, currObj->enable, FALSE);
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      Dev2LocalagentMtpStompObject *aMtpObj = NULL;
      Dev2LocalagentControllerMtpStompObject *cMtpObj = NULL;
      InstanceIdStack mtpIidStack = EMPTY_INSTANCE_ID_STACK;
      UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;

      rutUtil_modifyNumStompConnEntry_dev2(iidStack, -1);

      ret = sendMsgChanged(fullpath, currObj->enable, TRUE);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("fail to send msg to obuspa");
      }

      mdmLibCtx.hideObjectsPendingDelete = FALSE;

      while (cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_MTP_STOMP, &mtpIidStack,
             OGF_NO_VALUE_UPDATE, (void **)&aMtpObj) == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strcmp(aMtpObj->reference, fullpath))
         {
            CMSMEM_REPLACE_STRING_FLAGS(aMtpObj->reference, "",
                                        mdmLibCtx.allocFlags);

            if ((ret = cmsObj_setFlags(aMtpObj, &mtpIidStack,
                                       OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
            {
               cmsLog_error("fail to set agent mtp stomp obj, ret=%d", ret);
            }
         }

         cmsObj_free((void **)&aMtpObj);
      }

      INIT_INSTANCE_ID_STACK(&mtpIidStack);

      while (cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLER_MTP_STOMP,
             &mtpIidStack, OGF_NO_VALUE_UPDATE,
             (void **)&cMtpObj) == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strcmp(cMtpObj->reference, fullpath))
         {
            CMSMEM_REPLACE_STRING_FLAGS(cMtpObj->reference, "",
                                        mdmLibCtx.allocFlags);

            if ((ret = cmsObj_setFlags(cMtpObj, &mtpIidStack,
                                       OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
            {
               cmsLog_error("fail to set controller mtp stomp obj, ret<%d>",
                            ret);
            }
         }

         cmsObj_free((void **)&cMtpObj);
      }

      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);

   return ret;
}


#endif /* DMP_DEVICE2_STOMPCONN_1 */

