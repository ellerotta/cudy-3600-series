/***********************************************************************
 *
 *
<:copyright-BRCM:2022:proprietary:standard

   Copyright (c) 2022 Broadcom
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


#ifdef DMP_USPAGENT_1


#include "cms.h"
#include "bcm_ulog.h"
#include "cms_util.h"
#include "cms_core.h"
#include "mdm.h"


/* This file contains STLs for USPAgent Controller which is used by ACS
 * (CWMP) to get USP (Device.LocalAgent.Controller)'s configuration.
 */


extern CmsRet replaceLocalAgentWithUSPAgent
   (const char *input, char *output, int outputLen);


static CmsRet getInstance
   (const MdmObjectId oid, const InstanceIdStack *uspIidStack, void *uspObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 found = FALSE;
   InstanceIdStack iidStack;

   if (uspIidStack == NULL || uspObj == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   INIT_INSTANCE_ID_STACK(&iidStack);

   switch (oid)
   {
      case MDMOID_USP_AGENT_CONTROLLER:
      {
         Dev2USPAgentControllerObject *obj = (Dev2USPAgentControllerObject *) uspObj;
         Dev2LocalagentControllerObject *localObj = NULL;
         char buf[BUFLEN_1024] = {0};

         /* look for Dev2LocalagentControllerObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLER,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&localObj) == CMSRET_SUCCESS)
         {
            if (strstr(obj->alias, localObj->alias) != NULL)
            {
               found = TRUE;
               break;
            }

            /* free local agent controller that is not matched */
            cmsObj_free((void **) &localObj);
         }

         if (found == TRUE && localObj != NULL)
         {
            obj->enable = localObj->enable;
            obj->periodicNotifInterval = localObj->periodicNotifInterval;
            obj->USPNotifRetryMinimumWaitInterval = localObj->USPNotifRetryMinimumWaitInterval;
            obj->USPNotifRetryIntervalMultiplier = localObj->USPNotifRetryIntervalMultiplier;

            if (localObj->endpointID != NULL)
            {
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                  obj->endpointID, localObj->endpointID,
                  mdmLibCtx.allocFlags);
            }
            if (localObj->controllerCode != NULL)
            {
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                  obj->controllerCode, localObj->controllerCode,
                  mdmLibCtx.allocFlags);
            }
            if (localObj->provisioningCode != NULL)
            {
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                  obj->provisioningCode, localObj->provisioningCode,
                  mdmLibCtx.allocFlags);
            }
            if (localObj->assignedRole != NULL)
            {
               replaceLocalAgentWithUSPAgent(localObj->assignedRole, buf, sizeof(buf)-1);
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                  obj->assignedRole, buf,
                  mdmLibCtx.allocFlags);
            }
            if (localObj->credential != NULL)
            {
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                  obj->credential, localObj->credential,
                  mdmLibCtx.allocFlags);
            }
            if (localObj->periodicNotifTime != NULL)
            {
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                  obj->periodicNotifTime, localObj->periodicNotifTime,
                  mdmLibCtx.allocFlags);
            }

            /* free local agent controller */
            cmsObj_free((void **) &localObj);
         }
      }
         break;

      case MDMOID_USP_AGENT_CONTROLLER_MTP:
      {
         Dev2USPAgentControllerMtpObject *mtpObj = (Dev2USPAgentControllerMtpObject *) uspObj;
         Dev2USPAgentControllerObject *cntrlObj = NULL;
         Dev2LocalagentControllerObject *localCntrlObj = NULL;
         Dev2LocalagentControllerMtpObject *localMtpObj = NULL;
         InstanceIdStack mtpIidStack = *uspIidStack;

         /* get Dev2USPAgentControllerObject parent of Dev2USPAgentControllerMtpObject */
         ret = cmsObj_getAncestor(MDMOID_USP_AGENT_CONTROLLER,
                                  MDMOID_USP_AGENT_CONTROLLER_MTP,
                                  &mtpIidStack,
                                  (void **)&cntrlObj);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get usp agent controller, ret=%d", ret);
            return ret;
         }

         /* look for Dev2LocalagentControllerObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLER,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&localCntrlObj) == CMSRET_SUCCESS)
         {
            if (strstr(cntrlObj->alias, localCntrlObj->alias) != NULL)
            {
               found = TRUE;
            }

            /* free local agent controller */
            cmsObj_free((void **) &localCntrlObj);
         }

         /* free usp agent controller */
         cmsObj_free((void **) &cntrlObj);

         if (found == FALSE)
         {
            cmsLog_error("Could not find parent of local agent controller mtp, ret=%d", ret);
            return CMSRET_OBJECT_NOT_FOUND;
         }

         found = FALSE;
         INIT_INSTANCE_ID_STACK(&mtpIidStack);

         /* look for Dev2LocalagentControllerMtpObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextInSubTree(MDMOID_DEV2_LOCALAGENT_CONTROLLER_MTP,
                                        &iidStack, &mtpIidStack,
                                        (void **)&localMtpObj) == CMSRET_SUCCESS)
         {
            if (strstr(mtpObj->alias, localMtpObj->alias) != NULL)
            {
               found = TRUE;
               break;
            }

            /* free local agent controller MTP that is not matched */
            cmsObj_free((void **) &localMtpObj);
         }

         if (found == FALSE)
         {
            cmsLog_error("Could not find local agent controller mtp, ret=%d", ret);
            return CMSRET_OBJECT_NOT_FOUND;
         }

         mtpObj->enable = localMtpObj->enable;

         if (localMtpObj->protocol != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               mtpObj->protocol, localMtpObj->protocol,
               mdmLibCtx.allocFlags);
         }

         /* free local agent controller MTP */
         cmsObj_free((void **) &localMtpObj);
      }
         break;

      case MDMOID_USP_AGENT_CONTROLLER_MTP_STOMP:
      case MDMOID_USP_AGENT_CONTROLLER_MTP_MQTT:
      case MDMOID_USP_AGENT_CONTROLLER_MTP_WEBSOCKET:
      {
         Dev2USPAgentControllerMtpObject *mtpObj = NULL;
         Dev2USPAgentControllerObject *cntrlObj = NULL;
         Dev2LocalagentControllerObject *localCntrlObj = NULL;
         Dev2LocalagentControllerMtpObject *localMtpObj = NULL;
         InstanceIdStack mtpIidStack = *uspIidStack;

         /* Dev2USPAgentControllerMtpStompObject has the same iidStack
          * with Dev2USPAgentControllerMtpObject */

         /* get Dev2USPAgentControllerObject parent of Dev2USPAgentControllerMtpObject */
         ret = cmsObj_getAncestor(MDMOID_USP_AGENT_CONTROLLER,
                                  MDMOID_USP_AGENT_CONTROLLER_MTP,
                                  &mtpIidStack,
                                  (void **)&cntrlObj);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get usp agent controller, ret=%d", ret);
            return ret;
         }

         /* look for Dev2LocalagentControllerObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLER,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&localCntrlObj) == CMSRET_SUCCESS)
         {
            if (strstr(cntrlObj->alias, localCntrlObj->alias) != NULL)
            {
               found = TRUE;
            }

            /* free local agent controller */
            cmsObj_free((void **) &localCntrlObj);
         }

         /* free usp agent controller */
         cmsObj_free((void **) &cntrlObj);

         if (found == FALSE)
         {
            cmsLog_error("Could not find parent of local agent controller mtp, ret=%d", ret);
            return CMSRET_OBJECT_NOT_FOUND;
         }

         mtpIidStack = *uspIidStack;
         /* get usp agent controller MTP */
         ret = cmsObj_get(MDMOID_USP_AGENT_CONTROLLER_MTP,
                          &mtpIidStack,
                          OGF_NO_VALUE_UPDATE,
                          (void **)&mtpObj);

         if (ret != CMSRET_SUCCESS || mtpObj == NULL)
         {
            cmsLog_error("Could not get usp agent controller mtp, ret=%d", ret);
            return ret;
         }

         found = FALSE;
         INIT_INSTANCE_ID_STACK(&mtpIidStack);

         /* look for Dev2LocalagentControllerMtpObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextInSubTree(MDMOID_DEV2_LOCALAGENT_CONTROLLER_MTP,
                                        &iidStack, &mtpIidStack,
                                        (void **)&localMtpObj) == CMSRET_SUCCESS)
         {
            if (strstr(mtpObj->alias, localMtpObj->alias) != NULL)
            {
               found = TRUE;
            }

            /* free local agent controller MTP that is not matched */
            cmsObj_free((void **) &localMtpObj);
         }

         /* free usp agent controller mtp */
         cmsObj_free((void **) &mtpObj);

         if (found == FALSE)
         {
            cmsLog_error("Could not find local agent controller mtp, ret=%d", ret);
            return CMSRET_OBJECT_NOT_FOUND;
         }

         if (oid == MDMOID_USP_AGENT_CONTROLLER_MTP_STOMP)
         {
            Dev2USPAgentControllerMtpStompObject *stompObj =
               (Dev2USPAgentControllerMtpStompObject *) uspObj;
            Dev2LocalagentControllerMtpStompObject *localStompObj = NULL;

            /* Dev2LocalagentControllerMtpStompObject has the same iidStack
             * with Dev2LocalagentControllerMtpObject */

            /* get local agent controller MTP STOMP */
            ret = cmsObj_get(MDMOID_DEV2_LOCALAGENT_CONTROLLER_MTP_STOMP,
                             &mtpIidStack,
                             OGF_NO_VALUE_UPDATE,
                             (void **)&localStompObj);

            if (ret == CMSRET_SUCCESS && localStompObj != NULL)
            {
               if (localStompObj->reference != NULL)
               {
                  REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                     stompObj->reference, localStompObj->reference,
                     mdmLibCtx.allocFlags);
               }
               if (localStompObj->destination != NULL)
               {
                  REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                     stompObj->destination, localStompObj->destination,
                     mdmLibCtx.allocFlags);
               }

               /* free local agent controller MTP STOMP */
               cmsObj_free((void **) &localStompObj);
            }
            else
            {
               cmsLog_error("Failed to get Dev2LocalagentControllerMtpStompObject, ret=%d", ret);
            }
         }
         else if (oid == MDMOID_USP_AGENT_CONTROLLER_MTP_MQTT)
         {
            Dev2USPAgentControllerMtpMqttObject *mqttObj =
               (Dev2USPAgentControllerMtpMqttObject *) uspObj;
            Dev2LocalagentControllerMtpMqttObject *localMqttObj = NULL;

            /* Dev2LocalagentControllerMtpMqttObject has the same iidStack
             * with Dev2LocalagentControllerMtpObject */

            /* get local agent controller MTP MQTT */
            ret = cmsObj_get(MDMOID_DEV2_LOCALAGENT_CONTROLLER_MTP_MQTT,
                             &mtpIidStack,
                             OGF_NO_VALUE_UPDATE,
                             (void **)&localMqttObj);

            if (ret == CMSRET_SUCCESS && localMqttObj != NULL)
            {
               if (localMqttObj->reference != NULL)
               {
                  REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                     mqttObj->reference, localMqttObj->reference,
                     mdmLibCtx.allocFlags);
               }
               if (localMqttObj->topic != NULL)
               {
                  REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                     mqttObj->topic, localMqttObj->topic,
                     mdmLibCtx.allocFlags);
               }

               mqttObj->publishRetainResponse = localMqttObj->publishRetainResponse;
               mqttObj->publishRetainNotify = localMqttObj->publishRetainNotify;

               /* free local agent controller MTP MQTT */
               cmsObj_free((void **) &localMqttObj);
            }
            else
            {
               cmsLog_error("Failed to get Dev2LocalagentControllerMtpMqttObject, ret=%d", ret);
            }
         }
         else if (oid == MDMOID_USP_AGENT_CONTROLLER_MTP_WEBSOCKET)
         {
            Dev2USPAgentControllerMtpWebsocketObject *wsObj =
               (Dev2USPAgentControllerMtpWebsocketObject *) uspObj;
            Dev2LocalagentControllerMtpWebsocketObject *localWsObj = NULL;

            /* Dev2LocalagentControllerMtpWebsocketObject has the same iidStack
             * with Dev2LocalagentControllerMtpObject */

            /* get local agent controller MTP WEBSOCKET */
            ret = cmsObj_get(MDMOID_DEV2_LOCALAGENT_CONTROLLER_MTP_WEBSOCKET,
                             &mtpIidStack,
                             OGF_NO_VALUE_UPDATE,
                             (void **)&localWsObj);

            if (ret == CMSRET_SUCCESS && localWsObj != NULL)
            {
               if (localWsObj->host != NULL)
               {
                  REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                     wsObj->host, localWsObj->host,
                     mdmLibCtx.allocFlags);
               }
               if (localWsObj->path != NULL)
               {
                  REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                     wsObj->path, localWsObj->path,
                     mdmLibCtx.allocFlags);
               }

               wsObj->port = localWsObj->port;
               wsObj->enableEncryption = localWsObj->enableEncryption;
               wsObj->keepAliveInterval = localWsObj->keepAliveInterval;
               wsObj->currentRetryCount = localWsObj->currentRetryCount;
               wsObj->sessionRetryMinimumWaitInterval = localWsObj->sessionRetryMinimumWaitInterval;
               wsObj->sessionRetryIntervalMultiplier = localWsObj->sessionRetryIntervalMultiplier;

               /* free local agent controller MTP WEBSOCKET */
               cmsObj_free((void **) &localWsObj);
            }
            else
            {
               cmsLog_error("Failed to get Dev2LocalagentControllerMtpWebsocketObject, ret=%d", ret);
            }
         }
      }
         break;

      default:
         break;
   }

   if (found == FALSE)
   {
      ret = CMSRET_OBJECT_NOT_FOUND;
   }

   return ret;
}


CmsRet stl_dev2USPAgentControllerObject(_Dev2USPAgentControllerObject *obj,
                                        const InstanceIdStack *iidStack)
{
   return getInstance(MDMOID_USP_AGENT_CONTROLLER, iidStack, obj);
}


CmsRet stl_dev2USPAgentControllerMtpObject(_Dev2USPAgentControllerMtpObject *obj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
   return getInstance(MDMOID_USP_AGENT_CONTROLLER_MTP, iidStack, obj);
}


CmsRet stl_dev2USPAgentControllerMtpStompObject(_Dev2USPAgentControllerMtpStompObject *obj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
   return getInstance(MDMOID_USP_AGENT_CONTROLLER_MTP_STOMP, iidStack, obj);
}


CmsRet stl_dev2USPAgentControllerMtpWebsocketObject(_Dev2USPAgentControllerMtpWebsocketObject *obj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
   return getInstance(MDMOID_USP_AGENT_CONTROLLER_MTP_WEBSOCKET, iidStack, obj);
}


CmsRet stl_dev2USPAgentControllerMtpMqttObject(_Dev2USPAgentControllerMtpMqttObject *obj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
   return getInstance(MDMOID_USP_AGENT_CONTROLLER_MTP_MQTT, iidStack, obj);
}


#endif /* DMP_USPAGENT_1 */
