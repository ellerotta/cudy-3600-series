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


#include "stl.h"
#include "cms_mem.h"
#include "cms_util.h"
#include "bcm_ulog.h"


/* This file contains STLs for USPAgent which is used by ACS (CWMP)
 * to get USP (Device.LocalAgent)'s configuration.
 */


CmsRet replaceLocalAgentWithUSPAgent
   (const char *input, char *output, int outputLen)
{
   CmsRet ret = CMSRET_SUCCESS;
   int len = 0;
   char item[BUFLEN_256] = {0};
   char data[BUFLEN_1024] = {0};
   char *token = NULL;
   char *saveptr = NULL;
   char *p = NULL;

   if (input == NULL || output == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   strncpy(data, input, sizeof(data)-1);

   memset(output, 0, outputLen);

   for (token = strtok_r(data, ",", &saveptr);
        token != NULL;
        token = strtok_r(saveptr, ",", &saveptr))
   {
	  p = strstr(token, "LocalAgent");
      if (p != NULL)
      {
         if ((p - token) < (int)(sizeof(item) - 1))
         {
            strncpy(item, token, p - token);
            len = strlen(item) + strlen("USPAgent") +
                  strlen(p+strlen("LocalAgent"));
            if (len < (int) sizeof(item)-1)
            {
               strcat(item, "USPAgent");
               strcat(item, p + strlen("LocalAgent"));
            }
         }
      }
      else
      {
         strncpy(item, token, sizeof(item)-1);
      }

      len = strlen(output) + strlen(item) + 1;
      if (len < outputLen)
      {
         strcat(output, item);
         strcat(output, ",");
      }
      else
      {
         ret = CMSRET_RESOURCE_EXCEEDED;
         break;
      }
   }

   /* remove the last ',' separator */
   len = strlen(output);
   if (len > 0)
      output[len-1] = '\0';

   return ret;
}


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
      case MDMOID_USP_AGENT:
      {
         Dev2USPAgentObject *obj = (Dev2USPAgentObject *) uspObj;
         Dev2LocalagentObject *localObj = NULL;

         /* get local agent */
         ret = cmsObj_get(MDMOID_DEV2_LOCALAGENT,
                          &iidStack,
                          OGF_NO_VALUE_UPDATE,
                          (void **)&localObj);

         if (ret != CMSRET_SUCCESS || localObj == NULL)
         {
            cmsLog_error("Could not get local agent, ret=%d", ret);
            return ret;
         }

         obj->upTime = localObj->upTime;
         obj->maxSubscriptionChangeAdoptionTime = localObj->maxSubscriptionChangeAdoptionTime;

         if (localObj->endpointID != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->endpointID,
               localObj->endpointID,
               mdmLibCtx.allocFlags);
         }
         if (localObj->softwareVersion != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->softwareVersion,
               localObj->softwareVersion,
               mdmLibCtx.allocFlags);
         }
         if (localObj->supportedProtocols != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->supportedProtocols,
               localObj->supportedProtocols,
               mdmLibCtx.allocFlags);
         }
         if (localObj->supportedFingerprintAlgorithms != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->supportedFingerprintAlgorithms,
               localObj->supportedFingerprintAlgorithms,
               mdmLibCtx.allocFlags);
         }
         if (localObj->advertisedDeviceSubtypes != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->advertisedDeviceSubtypes,
               localObj->advertisedDeviceSubtypes,
               mdmLibCtx.allocFlags);
         }
         if (localObj->X_BROADCOM_COM_DualStackPreference != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->X_BROADCOM_COM_DualStackPreference,
               localObj->X_BROADCOM_COM_DualStackPreference,
               mdmLibCtx.allocFlags);
         }

         /* free local agent */
         cmsObj_free((void **) &localObj);
      }
         break;

      case MDMOID_USP_AGENT_MTP:
      {
         Dev2USPAgentMtpObject *obj = (Dev2USPAgentMtpObject *) uspObj;
         Dev2LocalagentMtpObject *localObj = NULL;

         /* look for Dev2LocalagentMtpObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_MTP,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&localObj) == CMSRET_SUCCESS)
         {
            if (strstr(obj->alias, localObj->alias) != NULL)
            {
               found = TRUE;
               break;
            }

            /* free local agent MTP that is not matched */
            cmsObj_free((void **) &localObj);
         }

         if (found == FALSE || localObj == NULL)
         {
            cmsLog_error("Could not find local agent MTP, ret=%d", ret);
            return CMSRET_OBJECT_NOT_FOUND;
         }

         obj->enable = localObj->enable;
         obj->enableMDNS = localObj->enableMDNS;

         if (localObj->protocol != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->protocol, localObj->protocol,
               mdmLibCtx.allocFlags);
         }
         if (localObj->status != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->status, localObj->status,
               mdmLibCtx.allocFlags);
         }

         /* free local agent MTP */
         cmsObj_free((void **) &localObj);
      }
         break;

      case MDMOID_USP_AGENT_MTP_STOMP:
      case MDMOID_USP_AGENT_MTP_MQTT:
      case MDMOID_USP_AGENT_MTP_WEBSOCKET:
      {
         Dev2USPAgentMtpObject *mtpObj = NULL;
         Dev2LocalagentMtpObject *localMtpObj = NULL;
         InstanceIdStack mtpIidStack = *uspIidStack;

         /* Dev2USPAgentMtpStompObject has the same iidStack
          * with Dev2USPAgentMtpObject */

         /* get usp agent MTP */
         ret = cmsObj_get(MDMOID_USP_AGENT_MTP,
                          &mtpIidStack,
                          OGF_NO_VALUE_UPDATE,
                          (void **)&mtpObj);

         if (ret != CMSRET_SUCCESS || mtpObj == NULL)
         {
            cmsLog_error("Could not get usp agent mtp, ret=%d", ret);
            return ret;
         }

         /* look for Dev2LocalagentMtpObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_MTP,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&localMtpObj) == CMSRET_SUCCESS)
         {
            if (strstr(mtpObj->alias, localMtpObj->alias) != NULL)
            {
               found = TRUE;
            }

            /* free local agent MTP */
            cmsObj_free((void **) &localMtpObj);
         }

         /* free usp agent MTP */
         cmsObj_free((void **) &mtpObj);

         if (found == FALSE)
         {
            cmsLog_error("Could not find local agent mtp, ret=%d", ret);
            return CMSRET_OBJECT_NOT_FOUND;
         }

         if (oid == MDMOID_USP_AGENT_MTP_STOMP)
         {
            Dev2USPAgentMtpStompObject *stompObj =
               (Dev2USPAgentMtpStompObject *) uspObj;
            Dev2LocalagentMtpStompObject *localStompObj = NULL;

            /* Dev2LocalagentMtpStompObject has the same iidStack
             * with Dev2LocalagentMtpObject */

            /* get local agent MTP STOMP */
            ret = cmsObj_get(MDMOID_DEV2_LOCALAGENT_MTP_STOMP,
                             &iidStack,
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
               if (localStompObj->destinationFromServer != NULL)
               {
                  REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                     stompObj->destinationFromServer,
                     localStompObj->destinationFromServer,
                     mdmLibCtx.allocFlags);
               }

               /* free local agent MTP STOMP */
               cmsObj_free((void **) &localStompObj);
            }
            else
            {
               cmsLog_error("Failed to get Dev2LocalagentMtpStompObject, ret=%d", ret);
            }
         }
         else if (oid == MDMOID_USP_AGENT_MTP_MQTT)
         {
            Dev2USPAgentMtpMqttObject *mqttObj =
               (Dev2USPAgentMtpMqttObject *) uspObj;
            Dev2LocalagentMtpMqttObject *localMqttObj = NULL;

            /* Dev2LocalagentMtpMqttObject has the same iidStack
             * with Dev2LocalagentMtpObject */

            /* get local agent MTP MQTT */
            ret = cmsObj_get(MDMOID_DEV2_LOCALAGENT_MTP_MQTT,
                             &iidStack,
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
               if (localMqttObj->responseTopicConfigured != NULL)
               {
                  REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                     mqttObj->responseTopicConfigured,
                     localMqttObj->responseTopicConfigured,
                     mdmLibCtx.allocFlags);
               }
               if (localMqttObj->responseTopicDiscovered != NULL)
               {
                  REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                     mqttObj->responseTopicDiscovered,
                     localMqttObj->responseTopicDiscovered,
                     mdmLibCtx.allocFlags);
               }

               mqttObj->publishQoS = localMqttObj->publishQoS;

               /* free local agent MTP MQTT */
               cmsObj_free((void **) &localMqttObj);
            }
            else
            {
               cmsLog_error("Failed to get Dev2LocalagentMtpMqttObject, ret=%d", ret);
            }
         }
         else if (oid == MDMOID_USP_AGENT_MTP_WEBSOCKET)
         {
            Dev2USPAgentMtpWebsocketObject *wsObj =
               (Dev2USPAgentMtpWebsocketObject *) uspObj;
            Dev2LocalagentMtpWebsocketObject *localWsObj = NULL;

            /* Dev2LocalagentMtpWebsocketObject has the same iidStack
             * with Dev2LocalagentMtpObject */

            /* get local agent MTP WEBSOCKET */
            ret = cmsObj_get(MDMOID_DEV2_LOCALAGENT_MTP_WEBSOCKET,
                             &iidStack,
                             OGF_NO_VALUE_UPDATE,
                             (void **)&localWsObj);

            if (ret == CMSRET_SUCCESS && localWsObj != NULL)
            {
               if (localWsObj->interfaces != NULL)
               {
                  REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                     wsObj->interfaces, localWsObj->interfaces,
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

               /* free local agent controller MTP WEBSOCKET */
               cmsObj_free((void **) &localWsObj);
            }
            else
            {
               cmsLog_error("Failed to get Dev2LocalagentMtpWebsocketObject, ret=%d", ret);
            }
         }
      }
         break;

      case MDMOID_USP_AGENT_CERTIFICATE:
      {
         Dev2USPAgentCertificateObject *obj = (Dev2USPAgentCertificateObject *) uspObj;
         Dev2LocalagentCertificateObject *localObj = NULL;

         /* look for Dev2LocalagentCertificateObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CERTIFICATE,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&localObj) == CMSRET_SUCCESS)
         {
            if (strstr(obj->alias, localObj->alias) != NULL)
            {
               found = TRUE;
               break;
            }

            /* free local agent certificate that is not matched */
            cmsObj_free((void **) &localObj);
         }

         if (found == FALSE)
         {
            cmsLog_error("Could not find local agent certificate, ret=%d", ret);
            return CMSRET_OBJECT_NOT_FOUND;
         }

         obj->enable = localObj->enable;
         if (localObj->serialNumber != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->serialNumber, localObj->serialNumber,
               mdmLibCtx.allocFlags);
         }
         if (localObj->issuer != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->issuer, localObj->issuer,
               mdmLibCtx.allocFlags);
         }
         if (localObj->X_BROADCOM_COM_AltName != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->X_BROADCOM_COM_AltName, localObj->X_BROADCOM_COM_AltName,
               mdmLibCtx.allocFlags);
         }

         /* free local agent certificate */
         cmsObj_free((void **) &localObj);
      }
         break;

      case MDMOID_USP_AGENT_CONTROLLERTRUST:
      {
         Dev2USPAgentControllertrustObject *obj = (Dev2USPAgentControllertrustObject *) uspObj;
         Dev2LocalagentControllertrustObject *localObj = NULL;
         char buf[BUFLEN_1024] = {0};

         /* get local agent controller trust */
         ret = cmsObj_get(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST,
                          &iidStack,
                          OGF_NO_VALUE_UPDATE,
                          (void **)&localObj);

         if (ret != CMSRET_SUCCESS || localObj == NULL)
         {
            cmsLog_error("Could not get local agent controller trust, ret=%d", ret);
            return ret;
         }

         if (localObj->untrustedRole != NULL)
         {
            replaceLocalAgentWithUSPAgent(localObj->untrustedRole, buf, sizeof(buf)-1);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->untrustedRole, buf,
               mdmLibCtx.allocFlags);
         }
         if (localObj->bannedRole != NULL)
         {
            replaceLocalAgentWithUSPAgent(localObj->bannedRole, buf, sizeof(buf)-1);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->bannedRole, buf,
               mdmLibCtx.allocFlags);
         }

         obj->TOFUAllowed = localObj->TOFUAllowed;
         obj->TOFUInactivityTimer = localObj->TOFUInactivityTimer;

         /* free local agent controller trust */
         cmsObj_free((void **) &localObj);
      }
         break;

      case MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE:
      {
         Dev2USPAgentControllertrustRoleObject *obj = (Dev2USPAgentControllertrustRoleObject *) uspObj;
         Dev2LocalagentControllertrustRoleObject *localObj = NULL;

         /* look for Dev2LocalagentControllertrustRoleObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&localObj) == CMSRET_SUCCESS)
         {
            if (strstr(obj->alias, localObj->alias) != NULL)
            {
               found = TRUE;
               break;
            }

            /* free local agent controller trust role that is not matched */
            cmsObj_free((void **) &localObj);
         }

         if (found == FALSE)
         {
            cmsLog_error("Could not find local agent controller trust role, ret=%d", ret);
            return CMSRET_OBJECT_NOT_FOUND;
         }

         if (localObj->name != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS
               (obj->name, localObj->name, mdmLibCtx.allocFlags);
         }

         obj->enable = localObj->enable;

         /* free local agent controller trust role */
         cmsObj_free((void **) &localObj);
      }
         break;

      case MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE_PERMISSION:
      {
         Dev2USPAgentControllertrustRolePermissionObject *permObj =
            (Dev2USPAgentControllertrustRolePermissionObject *) uspObj;
         Dev2USPAgentControllertrustRoleObject *roleObj = NULL;
         Dev2LocalagentControllertrustRoleObject *localRoleObj = NULL;
         Dev2LocalagentControllertrustRolePermissionObject *localPermObj = NULL;
         InstanceIdStack mtpIidStack = *uspIidStack;

         /* get Dev2USPAgentControllertrustRoleObject parent of Dev2USPAgentControllertrustRolePermissionObject */
         ret = cmsObj_getAncestor(MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE,
                                  MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE_PERMISSION,
                                  &mtpIidStack,
                                  (void **)&roleObj);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get usp agent controller trust role, ret=%d", ret);
            return ret;
         }

         /* look for Dev2LocalagentControllertrustRoleObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&localRoleObj) == CMSRET_SUCCESS)
         {
            if (strstr(roleObj->alias, localRoleObj->alias) != NULL)
            {
               found = TRUE;
            }

            /* free local agent controller trust role */
            cmsObj_free((void **) &localRoleObj);
         }

         /* free usp agent controller trust role */
         cmsObj_free((void **) &roleObj);

         if (found == FALSE)
         {
            cmsLog_error("Could not find parent of local agent controller trust role permission, ret=%d", ret);
            return CMSRET_OBJECT_NOT_FOUND;
         }

         found = FALSE;
         INIT_INSTANCE_ID_STACK(&mtpIidStack);

         /* look for Dev2LocalagentControllertrustRolePermissionObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextInSubTree(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE_PERMISSION,
                                        &iidStack, &mtpIidStack,
                                        (void **)&localPermObj) == CMSRET_SUCCESS)
         {
            if (strstr(permObj->alias, localPermObj->alias) != NULL)
            {
               found = TRUE;
               break;
            }

            /* free local agent controller trust role permission that is not matched */
            cmsObj_free((void **) &localPermObj);
         }

         if (found == FALSE)
         {
            cmsLog_error("Could not find local agent controller trust role permission, ret=%d", ret);
            return CMSRET_OBJECT_NOT_FOUND;
         }

         if (localPermObj->targets != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS
               (permObj->targets, localPermObj->targets, mdmLibCtx.allocFlags);
         }
         if (localPermObj->param != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS
               (permObj->param, localPermObj->param, mdmLibCtx.allocFlags);
         }
         if (localPermObj->obj != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS
               (permObj->obj, localPermObj->obj, mdmLibCtx.allocFlags);
         }
         if (localPermObj->instantiatedObj != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS
               (permObj->instantiatedObj, localPermObj->instantiatedObj, mdmLibCtx.allocFlags);
         }
         if (localPermObj->commandEvent != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS
               (permObj->commandEvent, localPermObj->commandEvent, mdmLibCtx.allocFlags);
         }

         permObj->enable = localPermObj->enable;
         permObj->order = localPermObj->order;

         /* free local agent controller trust role permission */
         cmsObj_free((void **) &localPermObj);
      }
         break;

      case MDMOID_USP_AGENT_CONTROLLERTRUST_CREDENTIAL:
      {
         Dev2USPAgentControllertrustCredentialObject *obj =
            (Dev2USPAgentControllertrustCredentialObject *) uspObj;
         Dev2LocalagentControllertrustCredentialObject *localObj = NULL;

         /* look for Dev2LocalagentControllertrustCredentialObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_CREDENTIAL,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&localObj) == CMSRET_SUCCESS)
         {
            if (strstr(obj->alias, localObj->alias) != NULL)
            {
               found = TRUE;
               break;
            }

            /* free local agent controller trust credential that is not matched */
            cmsObj_free((void **) &localObj);
         }

         if (found == FALSE)
         {
            cmsLog_error("Could not find local agent controller trust credential, ret=%d", ret);
            return CMSRET_OBJECT_NOT_FOUND;
         }

         obj->enable = localObj->enable;
         if (localObj->role != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS
               (obj->role, localObj->role, mdmLibCtx.allocFlags);
         }
         if (localObj->credential != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS
               (obj->credential, localObj->credential, mdmLibCtx.allocFlags);
         }
         if (localObj->allowedUses != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS
               (obj->allowedUses, localObj->allowedUses, mdmLibCtx.allocFlags);
         }

         /* free local agent controller trust credential */
         cmsObj_free((void **) &localObj);
      }
         break;

      case MDMOID_USP_AGENT_CONTROLLERTRUST_CHALLENGE:
      {
         Dev2USPAgentControllertrustChallengeObject *obj =
            (Dev2USPAgentControllertrustChallengeObject *) uspObj;
         Dev2LocalagentControllertrustChallengeObject *localObj = NULL;

         /* look for Dev2LocalagentControllertrustChallengeObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_CHALLENGE,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&localObj) == CMSRET_SUCCESS)
         {
            if (strstr(obj->alias, localObj->alias) != NULL)
            {
               found = TRUE;
               break;
            }

            /* free local agent controller trust challenge that is not matched */
            cmsObj_free((void **) &localObj);
         }

         if (found == FALSE)
         {
            cmsLog_error("Could not find local agent controller trust challenge, ret=%d", ret);
            return CMSRET_OBJECT_NOT_FOUND;
         }

         obj->enable = localObj->enable;
         obj->retries = localObj->retries;
         obj->lockoutPeriod = localObj->lockoutPeriod;
         if (localObj->description != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->description, localObj->description, mdmLibCtx.allocFlags);
         }
         if (localObj->role != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->role, localObj->role, mdmLibCtx.allocFlags);
         }
         if (localObj->type != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->type, localObj->type, mdmLibCtx.allocFlags);
         }
         if (localObj->value != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->value, localObj->value, mdmLibCtx.allocFlags);
         }
         if (localObj->valueType != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->valueType, localObj->valueType, mdmLibCtx.allocFlags);
         }
         if (localObj->instruction != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->instruction, localObj->instruction, mdmLibCtx.allocFlags);
         }
         if (localObj->instructionType != NULL)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
               obj->instructionType, localObj->instructionType, mdmLibCtx.allocFlags);
         }

         /* free local agent controller trust challenge */
         cmsObj_free((void **) &localObj);
      }
         break;

      default:
         break;
   }

   return ret;
}


CmsRet stl_dev2USPAgentObject(_Dev2USPAgentObject *obj,
                              const InstanceIdStack *iidStack)
{
   return getInstance(MDMOID_USP_AGENT, iidStack, obj);
}


CmsRet stl_dev2USPAgentMtpObject(_Dev2USPAgentMtpObject *obj,
                                 const InstanceIdStack *iidStack)
{
   return getInstance(MDMOID_USP_AGENT_MTP, iidStack, obj);
}


CmsRet stl_dev2USPAgentMtpStompObject(_Dev2USPAgentMtpStompObject *obj,
                                      const InstanceIdStack *iidStack)
{
   return getInstance(MDMOID_USP_AGENT_MTP_STOMP, iidStack, obj);
}


CmsRet stl_dev2USPAgentMtpWebsocketObject(_Dev2USPAgentMtpWebsocketObject *obj,
                                          const InstanceIdStack *iidStack)
{
   return getInstance(MDMOID_USP_AGENT_MTP_WEBSOCKET, iidStack, obj);
}


CmsRet stl_dev2USPAgentMtpMqttObject(_Dev2USPAgentMtpMqttObject *obj,
                                     const InstanceIdStack *iidStack)
{
   return getInstance(MDMOID_USP_AGENT_MTP_MQTT, iidStack, obj);
}


CmsRet stl_dev2USPAgentCertificateObject(_Dev2USPAgentCertificateObject *obj,
                                const InstanceIdStack *iidStack)
{
   return getInstance(MDMOID_USP_AGENT_CERTIFICATE, iidStack, obj);
}


CmsRet stl_dev2USPAgentControllertrustObject(_Dev2USPAgentControllertrustObject *obj,
                                const InstanceIdStack *iidStack)
{
   return getInstance(MDMOID_USP_AGENT_CONTROLLERTRUST, iidStack, obj);
}


CmsRet stl_dev2USPAgentControllertrustRoleObject(_Dev2USPAgentControllertrustRoleObject *obj,
                                const InstanceIdStack *iidStack)
{
   return getInstance(MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE, iidStack, obj);
}


CmsRet stl_dev2USPAgentControllertrustRolePermissionObject(_Dev2USPAgentControllertrustRolePermissionObject *obj,
                                const InstanceIdStack *iidStack)
{
   return getInstance(MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE_PERMISSION, iidStack, obj);
}


CmsRet stl_dev2USPAgentControllertrustCredentialObject(_Dev2USPAgentControllertrustCredentialObject *obj,
                                const InstanceIdStack *iidStack)
{
   return getInstance(MDMOID_USP_AGENT_CONTROLLERTRUST_CREDENTIAL, iidStack, obj);
}


CmsRet stl_dev2USPAgentControllertrustChallengeObject(_Dev2USPAgentControllertrustChallengeObject *obj,
                                const InstanceIdStack *iidStack)
{
   return getInstance(MDMOID_USP_AGENT_CONTROLLERTRUST_CHALLENGE, iidStack, obj);
}


#endif /* DMP_USPAGENT_1 */
