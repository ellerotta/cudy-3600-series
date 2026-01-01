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


#include <dirent.h>

#include "cms.h"
#include "cms_msg_usp.h"
#include "bcm_ulog.h"
#include "cms_mem.h"
#include "cms_util.h"
#include "cms_tms.h"
#include "cms_core.h"
#include "rut_util.h"
#include "mdm.h"


/* This file contains RCLs for USPAgent which is used by ACS (CWMP)
 * to configure USP (Device.LocalAgent)'s configuration.
 */


CmsRet replaceUSPAgentWithLocalAgent
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
	  p = strstr(token, "USPAgent");
      if (p != NULL)
      {
         if ((p - token) < (int)(sizeof(item) - 1))
         {
            strncpy(item, token, p - token);
            len = strlen(item) + strlen("LocalAgent") +
                  strlen(p+strlen("USPAgent"));
            if (len < (int)sizeof(item)-1)
            {
               strcat(item, "LocalAgent");
               strcat(item, p + strlen("USPAgent"));
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


#define VAR_CERT_DIR   "/var/cert"
#define TEMP_CERT_FILE VAR_CERT_DIR"/usp-temp.cert"

static CmsRet sendCertificateChangeMsg(void)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   UspCertificateChangedMsgBody *msgPayload = NULL;

   reqMsg = cmsMem_alloc(sizeof(CmsMsgHeader)+sizeof(UspCertificateChangedMsgBody),
                         ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = (CmsMsgType) CMS_MSG_CERTIFICATE_CHANGE;
   reqMsg->src = EID_USP_MD;
   reqMsg->dst = EID_OBUSPA;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(UspCertificateChangedMsgBody);

   /* copy info from payload to msgPayload and send message */
   msgPayload = (UspCertificateChangedMsgBody*)(reqMsg + 1);
   cmsUtl_strcpy(msgPayload->fileName, TEMP_CERT_FILE);
   msgPayload->isDelete = FALSE;

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to send message to obuspa (ret=%d)", ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}


static CmsRet writeCertificateToFile(const char *certificate)
{
   CmsRet ret = CMSRET_SUCCESS;
   char cmd[BUFLEN_64] = {0};
   FILE *fpWrt = NULL;
   DIR* dir = NULL;

   /* create /var/cert directory if it does not exist */
   dir = opendir(VAR_CERT_DIR);
   if (dir)
   {
      closedir(dir);
   }
   else
   {
      snprintf(cmd, sizeof(cmd)-1, "mkdir -p %s", VAR_CERT_DIR);
      if ( system(cmd) == -1)
      {
         cmsLog_error("cmd <%s> failed.", cmd);
         return CMSRET_INTERNAL_ERROR;
     }
   }

   fpWrt = fopen(TEMP_CERT_FILE, "w");
   if (fpWrt == NULL)
   {
      cmsLog_error("Failed to open <%s> for writing.", TEMP_CERT_FILE);
      return CMSRET_INTERNAL_ERROR;
   }

   fputs(certificate, fpWrt);
   fclose(fpWrt);

   return ret;
}


static CmsRet addInstance
   (const MdmObjectId oid, const InstanceIdStack *uspIidStack, void *uspObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   char buff[BUFLEN_64+1]={0};
   InstanceIdStack iidStack;

   /* Do not add new LocalAgent instance at boot up */
   if (mdmShmCtx->inMdmInit == TRUE)
   {
      return ret;
   }

   if (uspIidStack == NULL || uspObj == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   INIT_INSTANCE_ID_STACK(&iidStack);

   switch (oid)
   {
      case MDMOID_USP_AGENT_MTP:
      {
         Dev2USPAgentMtpObject *obj = (Dev2USPAgentMtpObject *) uspObj;
         Dev2LocalagentMtpObject *localObj = NULL;

         /* add new instance of local agent MTP */
         ret = cmsObj_addInstance(MDMOID_DEV2_LOCALAGENT_MTP, &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not create new local agent MTP, ret=%d", ret);
            return ret;
         }

         /* get the instance of local agent MTP */
         ret = cmsObj_get(MDMOID_DEV2_LOCALAGENT_MTP, &iidStack, 0, (void **) &localObj);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get Dev2LocalagentMtpObject, ret=%d", ret);
            return ret;
         }

         /* At MTP instance addition, the alias is autogenerated
          * already; and it's not supposed to be modified once set.
          * But uspAgent uses the alias field to remember which one it
          * created by adding "-cwmp" at the end of it. There is no
          * mechanism to pass in the alias at instance add at the moment.
          * If problem arises, a new proprietary parameter can be added
          * for the propose of mapping USPAgent with LocalAgent's
          * instance created by an ACS.
          */
         snprintf(buff, sizeof(buff), "%s-cwmp", localObj->alias);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->alias, buff,
                                           mdmLibCtx.allocFlags);

         /* free local agent MTP */
         cmsObj_free((void **) &localObj);

         /* set usp agent MTP */
         ret = cmsObj_set(obj, uspIidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2USPAgentMtpObject, ret=%d", ret);
         }
      }
         break;

      case MDMOID_USP_AGENT_CERTIFICATE:
      {
         Dev2USPAgentCertificateObject *obj = (Dev2USPAgentCertificateObject *) uspObj;
         Dev2LocalagentCertificateObject *localObj = NULL;

         /* add new instance of local agent certificate */
         ret = cmsObj_addInstance(MDMOID_DEV2_LOCALAGENT_CERTIFICATE, &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not create new local agent certificate, ret=%d", ret);
            return ret;
         }

         /* get the instance of local agent certificate */
         ret = cmsObj_get(MDMOID_DEV2_LOCALAGENT_CERTIFICATE, &iidStack, 0, (void **) &localObj);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get Dev2LocalagentCertificateObject, ret=%d", ret);
            return ret;
         }

         /* At certificate instance addition, the alias is autogenerated
          * already; and it's not supposed to be modified once set.
          * But uspAgent uses the alias field to remember which one it
          * created by adding "-cwmp" at the end of it. There is no
          * mechanism to pass in the alias at instance add at the moment.
          * If problem arises, a new proprietary parameter can be added
          * for the propose of mapping USPAgent with LocalAgent's
          * instance created by an ACS.
          */
         snprintf(buff, sizeof(buff), "%s-cwmp", localObj->alias);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->alias, buff,
                                           mdmLibCtx.allocFlags);

         /* free local agent certificate */
         cmsObj_free((void **) &localObj);

         /* set usp agent certificate */
         ret = cmsObj_set(obj, uspIidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2USPAgentCertificateObject, ret=%d", ret);
         }
      }
         break;

      case MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE:
      {
         Dev2USPAgentControllertrustRoleObject *obj = (Dev2USPAgentControllertrustRoleObject *) uspObj;
         Dev2LocalagentControllertrustRoleObject *localObj = NULL;

         /* add new instance of local agent controller trust role */
         ret = cmsObj_addInstance(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE, &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not create new local agent controller trust role, ret=%d", ret);
            return ret;
         }

         /* get the instance of local agent controller trust role */
         ret = cmsObj_get(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE, &iidStack, 0, (void **) &localObj);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get Dev2LocalagentControllertrustRoleObject, ret=%d", ret);
            return ret;
         }

         /* At role instance addition, the alias is autogenerated
          * already; and it's not supposed to be modified once set.
          * But uspAgent uses the alias field to remember which one it
          * created by adding "-cwmp" at the end of it. There is no
          * mechanism to pass in the alias at instance add at the moment.
          * If problem arises, a new proprietary parameter can be added
          * for the propose of mapping USPAgent with LocalAgent's
          * instance created by an ACS.
          */
         snprintf(buff, sizeof(buff), "%s-cwmp", localObj->alias);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->alias, buff,
                                           mdmLibCtx.allocFlags);

         /* free local agent controller trust role */
         cmsObj_free((void **) &localObj);

         /* set usp agent controller trust role */
         ret = cmsObj_set(obj, uspIidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2USPAgentControllertrustRoleObject, ret=%d", ret);
         }
      }
         break;

      case MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE_PERMISSION:
      {
         UBOOL8 found = FALSE;
         Dev2USPAgentControllertrustRolePermissionObject *permObj = (Dev2USPAgentControllertrustRolePermissionObject *) uspObj;
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

         /* add new instance of local agent controller trust role permission */
         ret = cmsObj_addInstance(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE_PERMISSION,
                                  &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not create new local agent controller trust role permission, ret=%d", ret);
            return ret;
         }

         /* get the instance of local agent controller trust role permission */
         ret = cmsObj_get(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE_PERMISSION,
                          &iidStack, 0, (void **) &localPermObj);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get Dev2LocalagentControllertrustRolePermissionObject, ret=%d", ret);
            return ret;
         }

         /* At permission instance addition, the alias is autogenerated
          * already; and it's not supposed to be modified once set.
          * But uspAgent uses the alias field to remember which one it
          * created by adding "-cwmp" at the end of it. There is no
          * mechanism to pass in the alias at instance add at the moment.
          * If problem arises, a new proprietary parameter can be added
          * for the propose of mapping USPAgent with LocalAgent's
          * instance created by an ACS.
          */
         snprintf(buff, sizeof(buff), "%s-cwmp", localPermObj->alias);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(permObj->alias, buff,
                                           mdmLibCtx.allocFlags);

         /* free local agent controller trust role permission */
         cmsObj_free((void **) &localPermObj);

         /* set usp agent controller trust role permission */
         ret = cmsObj_set(permObj, uspIidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2USPAgentControllertrustRolePermissionObject, ret=%d", ret);
         }
      }
         break;

      case MDMOID_USP_AGENT_CONTROLLERTRUST_CREDENTIAL:
      {
         Dev2USPAgentControllertrustCredentialObject *obj =
            (Dev2USPAgentControllertrustCredentialObject *) uspObj;
         Dev2LocalagentControllertrustCredentialObject *localObj = NULL;

         /* add new instance of local agent controller trust credential */
         ret = cmsObj_addInstance(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_CREDENTIAL, &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not create new local agent controller trust credential, ret=%d", ret);
            return ret;
         }

         /* get the instance of local agent controller trust credential */
         ret = cmsObj_get(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_CREDENTIAL, &iidStack, 0, (void **) &localObj);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get Dev2LocalagentControllertrustCredentialObject, ret=%d", ret);
            return ret;
         }

         /* At credential instance addition, the alias is autogenerated
          * already; and it's not supposed to be modified once set.
          * But uspAgent uses the alias field to remember which one it
          * created by adding "-cwmp" at the end of it. There is no
          * mechanism to pass in the alias at instance add at the moment.
          * If problem arises, a new proprietary parameter can be added
          * for the propose of mapping USPAgent with LocalAgent's
          * instance created by an ACS.
          */
         snprintf(buff, sizeof(buff), "%s-cwmp", localObj->alias);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->alias, buff,
                                           mdmLibCtx.allocFlags);

         /* free local agent controller trust credential */
         cmsObj_free((void **) &localObj);

         /* set usp agent controller trust credential */
         ret = cmsObj_set(obj, uspIidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2USPAgentControllertrustCredentialObject, ret=%d", ret);
         }
      }
         break;

      case MDMOID_USP_AGENT_CONTROLLERTRUST_CHALLENGE:
      {
         Dev2USPAgentControllertrustChallengeObject *obj =
            (Dev2USPAgentControllertrustChallengeObject *) uspObj;
         Dev2LocalagentControllertrustChallengeObject *localObj = NULL;

         /* add new instance of local agent controller trust challenge */
         ret = cmsObj_addInstance(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_CHALLENGE, &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not create new local agent controller trust challenge, ret=%d", ret);
            return ret;
         }

         /* get the instance of local agent controller trust challenge */
         ret = cmsObj_get(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_CHALLENGE, &iidStack, 0, (void **) &localObj);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get Dev2LocalagentControllertrustChallengeObject, ret=%d", ret);
            return ret;
         }

         /* At challenge instance addition, the alias is autogenerated
          * already; and it's not supposed to be modified once set.
          * But uspAgent uses the alias field to remember which one it
          * created by adding "-cwmp" at the end of it. There is no
          * mechanism to pass in the alias at instance add at the moment.
          * If problem arises, a new proprietary parameter can be added
          * for the propose of mapping USPAgent with LocalAgent's
          * instance created by an ACS.
          */
         snprintf(buff, sizeof(buff), "%s-cwmp", localObj->alias);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->alias, buff,
                                           mdmLibCtx.allocFlags);

         /* free local agent controller trust challenge */
         cmsObj_free((void **) &localObj);

         /* set usp agent controller trust challenge */
         ret = cmsObj_set(obj, uspIidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2LocalagentControllertrustChallengeObject, ret=%d", ret);
         }
      }
         break;

      default:
         break;
   }

   return ret;
}


static CmsRet updateInstance
   (const MdmObjectId oid, const InstanceIdStack *uspIidStack, const void *uspObj)
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

         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->advertisedDeviceSubtypes,
            obj->advertisedDeviceSubtypes,
            mdmLibCtx.allocFlags);

         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->X_BROADCOM_COM_DualStackPreference,
            obj->X_BROADCOM_COM_DualStackPreference,
            mdmLibCtx.allocFlags);

         /* set local agent */
         ret = cmsObj_set(localObj, &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2LocalagentObject, ret=%d", ret);
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

         if (found == FALSE)
         {
            cmsLog_error("Could not find local agent MTP, ret=%d", ret);
            return CMSRET_OBJECT_NOT_FOUND;
         }

         localObj->enable = obj->enable;
         localObj->enableMDNS = obj->enableMDNS;

         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->protocol, obj->protocol,
            mdmLibCtx.allocFlags);

         /* set local agent MTP */
         ret = cmsObj_set(localObj, &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2LocalagentMtpObject, ret=%d", ret);
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
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                  localStompObj->reference, stompObj->reference,
                  mdmLibCtx.allocFlags);

               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                  localStompObj->destination, stompObj->destination,
                  mdmLibCtx.allocFlags);

               /* set local agent MTP STOMP */
               ret = cmsObj_set(localStompObj, &iidStack);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("Failed to set Dev2LocalagentMtpStompObject, ret=%d", ret);
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
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                  localMqttObj->reference, mqttObj->reference,
                  mdmLibCtx.allocFlags);
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                  localMqttObj->responseTopicConfigured,
                  mqttObj->responseTopicConfigured,
                  mdmLibCtx.allocFlags);
               localMqttObj->publishQoS = mqttObj->publishQoS;

               /* set local agent MTP MQTT */
               ret = cmsObj_set(localMqttObj, &iidStack);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("Failed to set Dev2LocalagentMtpMqttObject, ret=%d", ret);
               }

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
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                  localWsObj->interfaces, wsObj->interfaces,
                  mdmLibCtx.allocFlags);
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
                  localWsObj->path, wsObj->path,
                  mdmLibCtx.allocFlags);
               localWsObj->port = wsObj->port;
               localWsObj->enableEncryption = wsObj->enableEncryption;

               /* set local agent MTP WEBSOCKET */
               ret = cmsObj_set(localWsObj, &iidStack);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("Failed to set Dev2LocalagentMtpWebsocketObject, ret=%d", ret);
               }

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

         localObj->enable = obj->enable;
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->serialNumber, obj->serialNumber,
            mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->issuer, obj->issuer,
            mdmLibCtx.allocFlags);

         /* set local agent certificate */
         ret = cmsObj_set(localObj, &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2LocalagentCertificateObject, ret=%d", ret);
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

         replaceUSPAgentWithLocalAgent(obj->untrustedRole, buf, sizeof(buf)-1);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->untrustedRole, buf,
            mdmLibCtx.allocFlags);
         replaceUSPAgentWithLocalAgent(obj->bannedRole, buf, sizeof(buf)-1);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->bannedRole, buf,
            mdmLibCtx.allocFlags);
         localObj->TOFUAllowed = obj->TOFUAllowed;
         localObj->TOFUInactivityTimer = obj->TOFUInactivityTimer;

         /* set local agent controller trust */
         ret = cmsObj_set(localObj, &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2LocalagentControllertrustObject, ret=%d", ret);
         }

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

         localObj->enable = obj->enable;
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->name, obj->name, mdmLibCtx.allocFlags);

         /* set local agent controller trust role */
         ret = cmsObj_set(localObj, &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2LocalagentControllertrustRoleObject, ret=%d", ret);
         }

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

         REPLACE_STRING_IF_NOT_EQUAL_FLAGS
            (localPermObj->targets, permObj->targets, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS
            (localPermObj->param, permObj->param, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS
            (localPermObj->obj, permObj->obj, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS
            (localPermObj->instantiatedObj, permObj->instantiatedObj, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS
            (localPermObj->commandEvent, permObj->commandEvent, mdmLibCtx.allocFlags);
         localPermObj->enable = permObj->enable;
         localPermObj->order = permObj->order;

         /* set local agent controller trust role permission */
         ret = cmsObj_set(localPermObj, &mtpIidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2LocalagentControllertrustRolePermissionObject, ret=%d", ret);
         }

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

         localObj->enable = obj->enable;
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->role, obj->role, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->credential, obj->credential, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->allowedUses, obj->allowedUses, mdmLibCtx.allocFlags);

         /* set local agent controller trust credential */
         ret = cmsObj_set(localObj, &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2LocalagentControllertrustCredentialObject, ret=%d", ret);
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

         localObj->enable = obj->enable;
         localObj->retries = obj->retries;
         localObj->lockoutPeriod = obj->lockoutPeriod;
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->description, obj->description, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->role, obj->role, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->type, obj->type, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->value, obj->value, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->valueType, obj->valueType, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->instruction, obj->instruction, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(
            localObj->instructionType, obj->instructionType, mdmLibCtx.allocFlags);

         /* set local agent controller trust challenge */
         ret = cmsObj_set(localObj, &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2LocalagentControllertrustChallengeObject, ret=%d", ret);
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


static CmsRet deleteInstance
   (const MdmObjectId oid, const InstanceIdStack *uspIidStack, const char *alias)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack;

   if (uspIidStack == NULL || alias == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   INIT_INSTANCE_ID_STACK(&iidStack);

   switch (oid)
   {
      case MDMOID_USP_AGENT_MTP:
      {
         Dev2LocalagentMtpObject *localObj = NULL;
         UBOOL8 found = FALSE;

         /* look for Dev2LocalagentMtpObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_MTP,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&localObj) == CMSRET_SUCCESS)
         {
            if (strstr(alias, localObj->alias) != NULL)
            {
               found = TRUE;
               ret = cmsObj_deleteInstance(MDMOID_DEV2_LOCALAGENT_MTP, &iidStack);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("Failed to delete Dev2LocalagentMtpObject, ret=%d", ret);
               }
            }

            /* free local agent MTP */
            cmsObj_free((void **) &localObj);
         }

         if (found == FALSE)
         {
            cmsLog_error("Could not find local agent mtp, ret=%d", ret);
            ret = CMSRET_OBJECT_NOT_FOUND;
         }
      }
         break;

      case MDMOID_USP_AGENT_CERTIFICATE:
      {
         Dev2USPAgentCertificateObject *localObj = NULL;
         UBOOL8 found = FALSE;

         /* look for Dev2USPAgentCertificateObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CERTIFICATE,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&localObj) == CMSRET_SUCCESS)
         {
            if (strstr(alias, localObj->alias) != NULL)
            {
               found = TRUE;
               ret = cmsObj_deleteInstance(MDMOID_DEV2_LOCALAGENT_CERTIFICATE, &iidStack);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("Failed to delete Dev2USPAgentCertificateObject, ret=%d", ret);
               }
            }

            /* free local agent certificate */
            cmsObj_free((void **) &localObj);
         }

         if (found == FALSE)
         {
            cmsLog_error("Could not find local agent certificate, ret=%d", ret);
            ret = CMSRET_OBJECT_NOT_FOUND;
         }
      }
         break;

      case MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE:
      {
         Dev2LocalagentControllertrustRoleObject *localObj = NULL;
         UBOOL8 found = FALSE;

         /* look for Dev2LocalagentControllertrustRoleObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&localObj) == CMSRET_SUCCESS)
         {
            if (strstr(alias, localObj->alias) != NULL)
            {
               found = TRUE;
               ret = cmsObj_deleteInstance(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE, &iidStack);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("Failed to delete Dev2LocalagentControllertrustRoleObject, ret=%d", ret);
               }
            }

            /* free local agent controller trust role */
            cmsObj_free((void **) &localObj);
         }

         if (found == FALSE)
         {
            cmsLog_error("Could not find local agent controller trust role, ret=%d", ret);
            ret = CMSRET_OBJECT_NOT_FOUND;
         }
      }
         break;

      case MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE_PERMISSION:
      {
         UBOOL8 found = FALSE;
         Dev2USPAgentControllertrustRoleObject *rolelObj = NULL;
         Dev2LocalagentControllertrustRoleObject *localRoleObj = NULL;
         Dev2LocalagentControllertrustRolePermissionObject *localPermObj = NULL;
         InstanceIdStack mtpIidStack = *uspIidStack;

         /* get Dev2USPAgentControllertrustRoleObject parent of Dev2USPAgentControllertrustRolePermissionObject */
         ret = cmsObj_getAncestor(MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE,
                                  MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE_PERMISSION,
                                  &mtpIidStack,
                                  (void **)&rolelObj);

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
            if (strstr(rolelObj->alias, localRoleObj->alias) != NULL)
            {
               found = TRUE;
            }

            /* free local agent controller trust role */
            cmsObj_free((void **) &localRoleObj);
         }

         /* free usp agent controller trust role */
         cmsObj_free((void **) &rolelObj);

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
            if (strstr(alias, localPermObj->alias) != NULL)
            {
               found = TRUE;
               ret = cmsObj_deleteInstance(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE_PERMISSION, &mtpIidStack);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("Failed to delete MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE_PERMISSION, ret=%d", ret);
               }
            }

            /* free local agent controller trust role permission */
            cmsObj_free((void **) &localPermObj);
         }

         if (found == FALSE)
         {
            cmsLog_error("Could not find local agent controller trust role permission, ret=%d", ret);
            ret = CMSRET_OBJECT_NOT_FOUND;
         }
      }
         break;

      case MDMOID_USP_AGENT_CONTROLLERTRUST_CREDENTIAL:
      {
         Dev2LocalagentControllertrustCredentialObject *localObj = NULL;
         UBOOL8 found = FALSE;

         /* look for Dev2LocalagentControllertrustCredentialObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_CREDENTIAL,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&localObj) == CMSRET_SUCCESS)
         {
            if (strstr(alias, localObj->alias) != NULL)
            {
               found = TRUE;
               ret = cmsObj_deleteInstance(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_CREDENTIAL, &iidStack);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("Failed to delete Dev2LocalagentControllertrustCredentialObject, ret=%d", ret);
               }
            }

            /* free local agent controller trust credential */
            cmsObj_free((void **) &localObj);
         }

         if (found == FALSE)
         {
            cmsLog_error("Could not find local agent controller trust credential, ret=%d", ret);
            ret = CMSRET_OBJECT_NOT_FOUND;
         }
      }
         break;

      case MDMOID_USP_AGENT_CONTROLLERTRUST_CHALLENGE:
      {
         Dev2LocalagentControllertrustChallengeObject *localObj = NULL;
         UBOOL8 found = FALSE;

         /* look for Dev2LocalagentControllertrustChallengeObject matching with alias */
         while (found == FALSE &&
                cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_CHALLENGE,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&localObj) == CMSRET_SUCCESS)
         {
            if (strstr(alias, localObj->alias) != NULL)
            {
               found = TRUE;
               ret = cmsObj_deleteInstance(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_CHALLENGE, &iidStack);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("Failed to delete Dev2LocalagentControllertrustChallengeObject, ret=%d", ret);
               }
            }

            /* free local agent controller trust challenge */
            cmsObj_free((void **) &localObj);
         }

         if (found == FALSE)
         {
            cmsLog_error("Could not find local agent controller trust challenge, ret=%d", ret);
            ret = CMSRET_OBJECT_NOT_FOUND;
         }
      }
         break;

      default:
         break;
   }

   return ret;
}


CmsRet rcl_dev2USPAgentObject(_Dev2USPAgentObject *newObj,
                              const _Dev2USPAgentObject *currObj,
                              const InstanceIdStack *iidStack,
                              char **errorParam __attribute__((unused)),
                              CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (newObj && currObj)
   {
      ret = updateInstance(MDMOID_USP_AGENT, iidStack, newObj);

      if (ret == CMSRET_SUCCESS)
      {
         if (cmsUtl_strcmp(newObj->addCertificate,
                           currObj->addCertificate) != 0)
         {
            ret = writeCertificateToFile(newObj->addCertificate);
            if (ret == CMSRET_SUCCESS)
            {
               ret = sendCertificateChangeMsg();
            }
         }
      }
   }

   return ret;
}


CmsRet rcl_dev2USPAgentMtpObject(_Dev2USPAgentMtpObject *newObj,
                                 const _Dev2USPAgentMtpObject *currObj,
                                 const InstanceIdStack *iidStack,
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      ret = addInstance(MDMOID_USP_AGENT_MTP, iidStack, newObj);

      if (ret == CMSRET_SUCCESS)
      {
         rutUtil_modifyNumCwmpAgentMtpEntry_dev2(iidStack, 1);
      }
   }
   else if (newObj && currObj)
   {
      if (currObj->alias != NULL)
      {
         ret = updateInstance(MDMOID_USP_AGENT_MTP, iidStack, newObj);
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      ret = deleteInstance(MDMOID_USP_AGENT_MTP, iidStack, currObj->alias);

      if (ret == CMSRET_SUCCESS)
      {
         rutUtil_modifyNumCwmpAgentMtpEntry_dev2(iidStack, -1);
      }
   }

   return ret;
}


CmsRet rcl_dev2USPAgentMtpStompObject(_Dev2USPAgentMtpStompObject *newObj,
                                const _Dev2USPAgentMtpStompObject *currObj,
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (newObj && currObj)
   {
      ret = updateInstance(MDMOID_USP_AGENT_MTP_STOMP, iidStack, newObj);
   }

   return ret;
}


CmsRet rcl_dev2USPAgentMtpWebsocketObject(_Dev2USPAgentMtpWebsocketObject *newObj,
                               const _Dev2USPAgentMtpWebsocketObject *currObj,
                               const InstanceIdStack *iidStack,
                               char **errorParam __attribute__((unused)),
                               CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (newObj && currObj)
   {
      ret = updateInstance(MDMOID_USP_AGENT_MTP_WEBSOCKET, iidStack, newObj);
   }

   return ret;
}


CmsRet rcl_dev2USPAgentMtpMqttObject(_Dev2USPAgentMtpMqttObject *newObj __attribute__((unused)),
                                const _Dev2USPAgentMtpMqttObject *currObj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (newObj && currObj)
   {
      ret = updateInstance(MDMOID_USP_AGENT_MTP_MQTT, iidStack, newObj);
   }

   return ret;
}


CmsRet rcl_dev2USPAgentCertificateObject(_Dev2USPAgentCertificateObject *newObj,
                                const _Dev2USPAgentCertificateObject *currObj,
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      ret = addInstance(MDMOID_USP_AGENT_CERTIFICATE, iidStack, newObj);

      if (ret == CMSRET_SUCCESS)
      {
         rutUtil_modifyNumCwmpAgentCertificateEntry_dev2(iidStack, 1);
      }
   }
   else if (newObj && currObj)
   {
      ret = updateInstance(MDMOID_USP_AGENT_CERTIFICATE, iidStack, newObj);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      ret = deleteInstance(MDMOID_USP_AGENT_CERTIFICATE, iidStack, currObj->alias);

      if (ret == CMSRET_SUCCESS)
      {
         rutUtil_modifyNumCwmpAgentCertificateEntry_dev2(iidStack, -1);
      }
   }

   return ret;
}


CmsRet rcl_dev2USPAgentControllertrustObject(_Dev2USPAgentControllertrustObject *newObj __attribute__((unused)),
                                const _Dev2USPAgentControllertrustObject *currObj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (newObj && currObj)
   {
      ret = updateInstance(MDMOID_USP_AGENT_CONTROLLERTRUST, iidStack, newObj);
   }

   return ret;
}


CmsRet rcl_dev2USPAgentControllertrustRoleObject(_Dev2USPAgentControllertrustRoleObject *newObj,
                                const _Dev2USPAgentControllertrustRoleObject *currObj,
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      ret = addInstance(MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE, iidStack, newObj);

      if (ret == CMSRET_SUCCESS)
      {
         rutUtil_modifyNumCwmpRoleEntry_dev2(iidStack, 1);
      }
   }
   else if (newObj && currObj)
   {
      if (currObj->alias != NULL)
      {
         ret = updateInstance(MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE, iidStack, newObj);
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      ret = deleteInstance(MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE, iidStack, currObj->alias);

      if (ret == CMSRET_SUCCESS)
      {
         rutUtil_modifyNumCwmpRoleEntry_dev2(iidStack, -1);
      }
   }

   return ret;
}


CmsRet rcl_dev2USPAgentControllertrustRolePermissionObject(_Dev2USPAgentControllertrustRolePermissionObject *newObj,
                                const _Dev2USPAgentControllertrustRolePermissionObject *currObj,
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      ret = addInstance(MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE_PERMISSION, iidStack, newObj);

      if (ret == CMSRET_SUCCESS)
      {
         rutUtil_modifyNumCwmpRolePermissionEntry_dev2(iidStack, 1);
      }
   }
   else if (newObj && currObj)
   {
      if (currObj->alias != NULL)
      {
         ret = updateInstance(MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE_PERMISSION, iidStack, newObj);
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      ret = deleteInstance(MDMOID_USP_AGENT_CONTROLLERTRUST_ROLE_PERMISSION, iidStack, currObj->alias);

      if (ret == CMSRET_SUCCESS)
      {
         rutUtil_modifyNumCwmpRolePermissionEntry_dev2(iidStack, -1);
      }
   }

   return ret;
}


CmsRet rcl_dev2USPAgentControllertrustCredentialObject(_Dev2USPAgentControllertrustCredentialObject *newObj,
                                const _Dev2USPAgentControllertrustCredentialObject *currObj,
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      ret = addInstance(MDMOID_USP_AGENT_CONTROLLERTRUST_CREDENTIAL, iidStack, newObj);

      if (ret == CMSRET_SUCCESS)
      {
         rutUtil_modifyNumCwmpControllerTrustCredentialEntry_dev2(iidStack, 1);
      }
   }
   else if (newObj && currObj)
   {
      if (currObj->alias != NULL)
      {
         ret = updateInstance(MDMOID_USP_AGENT_CONTROLLERTRUST_CREDENTIAL, iidStack, newObj);
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      ret = deleteInstance(MDMOID_USP_AGENT_CONTROLLERTRUST_CREDENTIAL, iidStack, currObj->alias);

      if (ret == CMSRET_SUCCESS)
      {
         rutUtil_modifyNumCwmpControllerTrustCredentialEntry_dev2(iidStack, -1);
      }
   }

   return ret;
}


CmsRet rcl_dev2USPAgentControllertrustChallengeObject(_Dev2USPAgentControllertrustChallengeObject *newObj,
                                const _Dev2USPAgentControllertrustChallengeObject *currObj,
                                const InstanceIdStack *iidStack,
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      ret = addInstance(MDMOID_USP_AGENT_CONTROLLERTRUST_CHALLENGE, iidStack, newObj);

      if (ret == CMSRET_SUCCESS)
      {
         rutUtil_modifyNumCwmpControllerTrustChallengeEntry_dev2(iidStack, 1);
      }
   }
   else if (newObj && currObj)
   {
      if (currObj->alias != NULL)
      {
         ret = updateInstance(MDMOID_USP_AGENT_CONTROLLERTRUST_CHALLENGE, iidStack, newObj);
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      ret = deleteInstance(MDMOID_USP_AGENT_CONTROLLERTRUST_CHALLENGE, iidStack, currObj->alias);

      if (ret == CMSRET_SUCCESS)
      {
         rutUtil_modifyNumCwmpControllerTrustChallengeEntry_dev2(iidStack, -1);
      }
   }

   return ret;
}


#endif /* DMP_USPAGENT_1 */
