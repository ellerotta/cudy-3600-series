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


#ifdef DMP_DEVICE2_BULKDATACOLL_1

#include "cms.h"
#include "cms_mem.h"
#include "cms_util.h"
#include "cms_core.h"
#include "rut_util.h"
#include "mdm.h"
#include "cms_msg_usp.h"


static CmsRet sendBulkdataChanged(CmsUspMsgType type, UINT32 instance,
                                  UBOOL8 isDelete)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   BulkdataChangedMsgBody *msgPayload = NULL;

   reqMsg=cmsMem_alloc(sizeof(CmsMsgHeader)+sizeof(BulkdataChangedMsgBody),
                       ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   reqMsg->type = (CmsMsgType) type;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = EID_OBUSPA;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(BulkdataChangedMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (BulkdataChangedMsgBody*)body;

   msgPayload->instance = instance;
   msgPayload->isDelete = isDelete;

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("sendMsg fail: type=%d instance=%d isDelete=%d ret=%d",
                   type, instance, isDelete, ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}

CmsRet rcl_dev2BulkDataObject( _Dev2BulkDataObject *newObj,
                const _Dev2BulkDataObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * During system startup, MDM needs to be initialized default values
    */
   if (mdmShmCtx->inMdmInit)
   {
      CMSMEM_REPLACE_STRING_FLAGS(newObj->protocols, "HTTP,USPEventNotif",
                                  mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(newObj->encodingTypes, "JSON",
                                  mdmLibCtx.allocFlags);
      return CMSRET_SUCCESS;
   }

   if (newObj && currObj)
   {
      if (newObj->enable != currObj->enable)
      {
         sendBulkdataChanged(CMS_MSG_BULKDATA_CHANGE, newObj->enable,
                             currObj->enable);
      }
   }

   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2BulkDataProfileObject( _Dev2BulkDataProfileObject *newObj,
                const _Dev2BulkDataProfileObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   UINT32 instance;

   instance = INSTANCE_ID_AT_DEPTH(iidStack, DEPTH_OF_IIDSTACK(iidStack)-1);
   
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumBulkDataProfileEntry_dev2(iidStack, 1);
      sendBulkdataChanged(CMS_MSG_BULKDATA_PROFILE_CHANGE, instance, FALSE);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumBulkDataProfileEntry_dev2(iidStack, -1);
      sendBulkdataChanged(CMS_MSG_BULKDATA_PROFILE_CHANGE, instance, TRUE);
   }

   if (newObj && currObj)
   {
      if (cmsUtl_strcmp(newObj->encodingType, MDMVS_JSON))
      {
         cmsLog_error("encodingType<%s> not supported. only JSON is supported",
                      newObj->encodingType);
         return CMSRET_INVALID_PARAM_VALUE;
      }

      sendBulkdataChanged(CMS_MSG_BULKDATA_PROFILE_CHANGE, instance, FALSE);
   }

   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2BulkDataProfileParamObject( _Dev2BulkDataProfileParamObject *newObj __attribute__((unused)),
                const _Dev2BulkDataProfileParamObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumProfileParamEntry_dev2(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumProfileParamEntry_dev2(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}

#ifdef DMP_DEVICE2_BULKDATACSVENCODING_1

CmsRet rcl_dev2BulkDataProfileCsvEncodingObject( _Dev2BulkDataProfileCsvEncodingObject *newObj __attribute__((unused)),
                const _Dev2BulkDataProfileCsvEncodingObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

#endif /* DMP_DEVICE2_BULKDATACSVENCODING_1 */

#ifdef DMP_DEVICE2_BULKDATAJSONENCODING_1

CmsRet rcl_dev2BulkDataProfileJsonEncodingObject( _Dev2BulkDataProfileJsonEncodingObject *newObj __attribute__((unused)),
                const _Dev2BulkDataProfileJsonEncodingObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if (newObj && currObj)
   {
      if (cmsUtl_strcmp(newObj->reportFormat, MDMVS_NAMEVALUEPAIR))
      {
         cmsLog_error("reportFormat<%s> not supported.",
                      newObj->reportFormat);
         return CMSRET_INVALID_PARAM_VALUE;
      }
   }

   return CMSRET_SUCCESS;
}

#endif /* DMP_DEVICE2_BULKDATAJSONENCODING_1 */

#ifdef DMP_DEVICE2_BULKDATAHTTP_1

CmsRet rcl_dev2BulkDataProfileHttpObject( _Dev2BulkDataProfileHttpObject *newObj,
                const _Dev2BulkDataProfileHttpObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   UINT32 instance;

   instance = INSTANCE_ID_AT_DEPTH(iidStack, DEPTH_OF_IIDSTACK(iidStack)-1);
   
   if (newObj && currObj)
   {
      if (cmsUtl_strcmp(newObj->compression, MDMVS_NONE) &&
          cmsUtl_strcmp(newObj->compression, "GZIP"))
      {
         cmsLog_error("compression<%s> not supported.",
                      newObj->compression);
         return CMSRET_INVALID_PARAM_VALUE;
      }

      sendBulkdataChanged(CMS_MSG_BULKDATA_PROFILE_HTTP_CHANGE, instance, TRUE);
   }

   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2BulkDataHttpReqURIParamObject( _Dev2BulkDataHttpReqURIParamObject *newObj __attribute__((unused)),
                const _Dev2BulkDataHttpReqURIParamObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumReqURIParamEntry_dev2(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumReqURIParamEntry_dev2(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}

#endif /* DMP_DEVICE2_BULKDATAHTTP_1 */


#endif /* DMP_DEVICE2_BULKDATACOLL_1 */

