/***********************************************************************
 *
 *  Copyright (c) 2020  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2020:proprietary:standard

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

#include <json-c/json_tokener.h>
#include <json-c/json_util.h>

#include "cms.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "cms_msg_pubsub.h"
#include "cms_obj.h"
#include "rut_util.h"
#include "mdm.h"


/*!\file rut_json.c
 * \brief JSON utility functions which can be used by various cms_core
 *        and mdm_cbk_xxx libs.  Maybe this should be moved to a utility lib
 *        outside of cms_core, but don't have a good place for it right now.
 *
 */

// Seems like this function could be generalized and used by various apps
// which want to publish to sys_directory.
CmsRet rutJson_publishKeyValueToSysDirectory(void *msgHandle,
                                             const char *key, const char *value)
{
   CmsMsgHeader *msg = NULL;
   PubSubKeyValueMsgBody *keyBody = NULL;
   UINT32 valueLen = 0;
   UINT32 len = 0;
   CmsRet ret = CMSRET_INTERNAL_ERROR;

   cmsLog_notice("Entered: key=%s value=%s", key, value);

   valueLen = (UINT32) cmsUtl_strlen(value);
   len = sizeof(CmsMsgHeader) + sizeof(PubSubKeyValueMsgBody)+ valueLen + 1;
   msg = (CmsMsgHeader *) cmsMem_alloc(len, ALLOC_ZEROIZE);
   if (msg == NULL)
   {
      cmsLog_error("Could not allocate %d bytes", len);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   msg->src = cmsMsg_getHandleEid(msgHandle);
   msg->dst = EID_SYSMGMT_MD;
   msg->type = CMS_MSG_PUBLISH_EVENT;
   msg->wordData = PUBSUB_EVENT_KEY_VALUE;
   msg->flags_event = 1;
   msg->dataLength = len - sizeof(CmsMsgHeader);

   keyBody = (PubSubKeyValueMsgBody *) (msg+1);
   snprintf(keyBody->key, sizeof(keyBody->key), "%s", key);
   keyBody->valueLen = valueLen + 1;

   strcpy(((char *)(keyBody+1)), value);

   if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to JSON cert msg, ret=%d", ret);
   }
   else
   {
      cmsLog_debug("JSON cert msg sent!");
   }

   cmsMem_free(msg);
            
   return ret;
}

CmsRet rutJson_encodeCertsAndPublish()
{
   json_object *topObj = NULL;
   json_object *arrayObj = NULL;
   json_object *certObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CertificateCfgObject *certCfgObj = NULL;
   const char *s = NULL;
   void *msgHandle = cmsMdm_getThreadMsgHandle();

   // create the top level json object and the array.
   topObj = json_object_new_object();
   arrayObj = json_object_new_array();

   while (cmsObj_getNextFlags(MDMOID_CERTIFICATE_CFG, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void**) &certCfgObj) == CMSRET_SUCCESS)
   {
      // create the individual json cert obj
      certObj = json_object_new_object();
      json_object_object_add(certObj, "name",
                             json_object_new_string(certCfgObj->name));
      json_object_object_add(certObj, "type",
                             json_object_new_string(certCfgObj->type));
      json_object_object_add(certObj, "content",
                             json_object_new_string(certCfgObj->content));

      // The documentation is not clear, but I think the arryObj now owns
      // the certObj (this is consistent with json_object_object_add).
      json_object_array_add(arrayObj, certObj);
      certObj = NULL;

      cmsObj_free((void **) &certCfgObj);
   }

   // ownership of arrayObj transfers to topObj
   json_object_object_add(topObj, "certs", arrayObj);
   s = json_object_to_json_string(topObj);

   // create a Cms Msg and send to sys_directory
   rutJson_publishKeyValueToSysDirectory(msgHandle,
                                         "X_BROADCOM_COM_CertificateCfg", s);

   // free the top level JSON obj, which owns all the internal objects.
   json_object_put(topObj);
   return CMSRET_SUCCESS;
}

