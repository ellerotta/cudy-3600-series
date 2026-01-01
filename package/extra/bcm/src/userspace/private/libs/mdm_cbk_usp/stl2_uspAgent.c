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

#ifdef DMP_DEVICE2_LOCALAGENT_1

#include "stl.h"
#include "cms_mem.h"
#include "cms_util.h"
#include "bcm_ulog.h"


CmsRet stl_dev2LocalagentObject(_Dev2LocalagentObject *obj,
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
   obj->upTime = cmsTms_getSeconds();
   return CMSRET_SUCCESS;
}


CmsRet stl_dev2LocalagentMtpObject(_Dev2LocalagentMtpObject *obj,
                                   const InstanceIdStack *iidStack)
{
   CmsRet ret = CMSRET_SUCCESS;
   MdmPathDescriptor pathDesc;

   /* If this mtp instance is disable, set Status to Down. */
   if (!obj->enable)
   {
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
      return ret;
   }

   if (cmsUtl_strcmp(obj->protocol, MDMVS_STOMP) == 0)
   {
      Dev2LocalagentMtpStompObject *mtpStompObj = NULL;
      Dev2StompConnectionObject *stompConnObj = NULL;

      ret = cmsObj_get(MDMOID_DEV2_LOCALAGENT_MTP_STOMP, iidStack,
                       OGF_NO_VALUE_UPDATE, (void **)&mtpStompObj);
      if (ret != CMSRET_SUCCESS)
      {
         bcmuLog_error("Failed to get agent mtp stomp object. ret=%d", ret);
         return ret;
      }

      if (IS_EMPTY_STRING(mtpStompObj->reference))
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
         cmsObj_free((void **)&mtpStompObj);
         return ret;
      }
 
      ret = cmsMdm_fullPathToPathDescriptor(mtpStompObj->reference, &pathDesc);
      if (ret != CMSRET_SUCCESS)
      {
         bcmuLog_error("invalid stomp path=%s", mtpStompObj->reference);
         cmsObj_free((void **)&mtpStompObj);
         return ret;
      }
      cmsObj_free((void **)&mtpStompObj);

      ret = cmsObj_get(MDMOID_DEV2_STOMP_CONNECTION, &(pathDesc.iidStack),
                       OGF_NORMAL_UPDATE, (void **)&stompConnObj);
      if (ret != CMSRET_SUCCESS)
      {
         bcmuLog_error("Failed to get agent mtp stomp object. ret=%d", ret);
         return ret;
      }

      if (cmsUtl_strcmp(stompConnObj->status, MDMVS_ENABLED) == 0)
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_UP, mdmLibCtx.allocFlags);
      }
      else
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
      }

      cmsObj_free((void **)&stompConnObj);
   }
   else if (cmsUtl_strcmp(obj->protocol, MDMVS_MQTT) == 0)
   {
      Dev2LocalagentMtpMqttObject *mtpMqttObj = NULL;
      Dev2MqttClientObject *mqttClientObj = NULL;

      ret = cmsObj_get(MDMOID_DEV2_LOCALAGENT_MTP_MQTT, iidStack,
                       OGF_NO_VALUE_UPDATE, (void **)&mtpMqttObj);
      if (ret != CMSRET_SUCCESS)
      {
         bcmuLog_error("Failed to get agent mtp mqtt object. ret=%d", ret);
         return ret;
      }

      if (IS_EMPTY_STRING(mtpMqttObj->reference))
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
         cmsObj_free((void **)&mtpMqttObj);
         return ret;
      }
 
      ret = cmsMdm_fullPathToPathDescriptor(mtpMqttObj->reference, &pathDesc);
      if (ret != CMSRET_SUCCESS)
      {
         bcmuLog_error("invalid mqtt path=%s", mtpMqttObj->reference);
         cmsObj_free((void **)&mtpMqttObj);
         return ret;
      }
      cmsObj_free((void **)&mtpMqttObj);

      ret = cmsObj_get(MDMOID_DEV2_MQTT_CLIENT, &(pathDesc.iidStack),
                       OGF_NORMAL_UPDATE, (void **)&mqttClientObj);
      if (ret != CMSRET_SUCCESS)
      {
         bcmuLog_error("Failed to get agent mtp mqtt object. ret=%d", ret);
         return ret;
      }

      if (cmsUtl_strcmp(mqttClientObj->status, MDMVS_CONNECTED) == 0)
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_UP, mdmLibCtx.allocFlags);
      }
      else
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
      }

      cmsObj_free((void **)&mqttClientObj);
   }
   else if (cmsUtl_strcmp(obj->protocol, MDMVS_WEBSOCKET) == 0)
   {
      /* Unlike STOMP/MQTT, as long as websocket MTP is enabled,
       * it's ready to establish communication. So set to UP directly
       */
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_UP, mdmLibCtx.allocFlags);
   }

   return ret;

}  /* End of stl_dev2LocalagentMtpObject() */


#ifdef DMP_DEVICE2_STOMPAGENT_1
CmsRet stl_dev2LocalagentMtpStompObject(_Dev2LocalagentMtpStompObject *obj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif /* DMP_DEVICE2_STOMPAGENT_1 */


#ifdef DMP_DEVICE2_WEBSOCKETAGENT_1
CmsRet stl_dev2LocalagentMtpWebsocketObject(_Dev2LocalagentMtpWebsocketObject *obj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif /* DMP_DEVICE2_WEBSOCKETAGENT_1 */


#ifdef DMP_DEVICE2_MQTTAGENT_1
CmsRet stl_dev2LocalagentMtpMqttObject(_Dev2LocalagentMtpMqttObject *obj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif /* DMP_DEVICE2_MQTTAGENT_1 */


#ifdef DMP_DEVICE2_SUBSCRIPTIONS_1
CmsRet stl_dev2LocalagentSubscriptionObject(_Dev2LocalagentSubscriptionObject *obj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif /* DMP_DEVICE2_SUBSCRIPTIONS_1 */


CmsRet stl_dev2LocalagentRequestObject(_Dev2LocalagentRequestObject *obj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2LocalagentRequestArgsObject(_Dev2LocalagentRequestArgsObject *obj __attribute__((unused)),
                                           const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2LocalagentCertificateObject(_Dev2LocalagentCertificateObject *obj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


#ifdef DMP_DEVICE2_CONTROLLERTRUST_1
CmsRet stl_dev2LocalagentControllertrustObject(_Dev2LocalagentControllertrustObject *obj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2LocalagentControllertrustRoleObject(_Dev2LocalagentControllertrustRoleObject *obj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2LocalagentControllertrustRolePermissionObject(_Dev2LocalagentControllertrustRolePermissionObject *obj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2LocalagentControllertrustCredentialObject(_Dev2LocalagentControllertrustCredentialObject *obj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


#ifdef DMP_DEVICE2_CHALLENGE_1
CmsRet stl_dev2LocalagentControllertrustChallengeObject(_Dev2LocalagentControllertrustChallengeObject *obj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif /* DMP_DEVICE2_CHALLENGE_1 */

#endif /* DMP_DEVICE2_CONTROLLERTRUST_1 */


#endif /* DMP_DEVICE2_LOCALAGENT_1 */


