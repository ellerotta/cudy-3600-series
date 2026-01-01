/***********************************************************************
# <:copyright-BRCM:2011:proprietary:standard
# 
#    Copyright (c) 2011 Broadcom 
#    All Rights Reserved
# 
#  This program is the proprietary software of Broadcom and/or its
#  licensors, and may only be used, duplicated, modified or distributed pursuant
#  to the terms and conditions of a separate, written license agreement executed
#  between you and Broadcom (an "Authorized License").  Except as set forth in
#  an Authorized License, Broadcom grants no license (express or implied), right
#  to use, or waiver of any kind with respect to the Software, and Broadcom
#  expressly reserves all rights in and to the Software and all intellectual
#  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
#  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
#  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
# 
#  Except as expressly set forth in the Authorized License,
# 
#  1. This program, including its structure, sequence and organization,
#     constitutes the valuable trade secrets of Broadcom, and you shall use
#     all reasonable efforts to protect the confidentiality thereof, and to
#     use this information only in connection with your use of Broadcom
#     integrated circuit products.
# 
#  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
#     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
#     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
#     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
#     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
#     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
#     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
#     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
#     PERFORMANCE OF THE SOFTWARE.
# 
#  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
#     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
#     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
#     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
#     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
#     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
#     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
#     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
#     LIMITED REMEDY.
# :>
************************************************************************/

#ifdef BRCM_VOICE_SUPPORT
#ifdef DMP_VOICE_SERVICE_2
#include <time.h>
#include "rcl.h"
#include "cms_util.h"
#include "rut_util.h"
#include "mdm.h"
#include "rut2_voice.h"
#include "dect_msg.h"
#include "sys/time.h"
#include "rut_wan.h"


#include <cms_log.h>
#include <cms_msg.h>

/* mwang_todo: these handler function should be put in more precise
 * DMP_<profilename> defines.
 */

#define BRIDGE_IF_NAME "br0"

/* According to various voice quality guides, 
 *  this voice quality threshold applies to all codecs. */
#define LOW_VOICE_QUALITY_THRESHOLD 36

/*****************************************************************************
**  FUNCTION:       sendMsgToVoice
**
**  PURPOSE:        Sends a message to voice application.
**
**  INPUT PARMS:    type     - message type
**                  wordData - word data
**
**  OUTPUT PARMS:
**
**  RETURNS:        CMSRET_SUCCESS if message has been sent successfully.
**                  Error otherwise.
**
*****************************************************************************/
static void sendMsgToVoice(CmsMsgType type, UINT32 wordData)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader msg = EMPTY_MSG_HEADER;
   void *msgHandle = cmsMdm_getThreadMsgHandle();

   msg.type = type;
   msg.src = cmsMsg_getHandleEid(msgHandle);
   msg.dst = EID_VOICE;
   msg.wordData = wordData;

   if ((ret = cmsMsg_send(msgHandle, &msg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send 0x%08x msg (0x%08x wordData) to voice, ret=%d",
                   type, wordData, ret);
   }
   else
   {
      cmsLog_debug("sent 0x%08x msg (0x%08x wordData) to voice, ret=%d",
                   type, wordData, ret);
   }
}

#ifdef SUPPORT_BAS2
/*****************************************************************************
**  FUNCTION:       sendMsgToBasClientVoiceWithData
**
**  PURPOSE:        Sends a message to voice BAS client application with 
**                   accompanying data.
**
**  INPUT PARMS:    type     - message type
**                  wordData - word data
**                  pData    - data in string format
**
**  OUTPUT PARMS:
**
**  RETURNS:        CMSRET_SUCCESS if message has been sent successfully.
**                  Error otherwise.
**
*****************************************************************************/
static void sendMsgToBasClientVoiceWithData(CmsMsgType type, UINT32 wordData, void *pData)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader* pMsg = NULL;
   void *pMsgBuf = NULL;
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   int sizeMsg;

   /* Create message of proper size */
   switch (wordData)
   {
      case CMSMSGVOICECONFIGCHANGED_LOW_VOICE_QUALITY:
         sizeMsg = sizeof(CmsMsgHeader) + sizeof(CmsMsgLowVoiceQualityEvent);
         break;

      default:
         sizeMsg = sizeof(CmsMsgHeader);
         break;
   }

   if ( (pMsgBuf = cmsMem_alloc( sizeMsg,
                                 ALLOC_ZEROIZE )) == NULL )
   {
      cmsLog_error("can't allocate memory");
      return;
   }

   /* prepared message header and body */
   pMsg  = (CmsMsgHeader *) pMsgBuf;

   pMsg->type = type;
   pMsg->src = cmsMsg_getHandleEid(msgHandle);
   pMsg->dst = EID_BAS_CLIENT_VOICE;
   pMsg->flags_event = 1;
   pMsg->wordData = wordData;
   pMsg->dataLength = sizeMsg - sizeof(CmsMsgHeader);

   /* Fill data if required */
   switch (wordData)
   {
      case CMSMSGVOICECONFIGCHANGED_LOW_VOICE_QUALITY:
      {
         memcpy((void *)(pMsg + 1), pData, sizeof(CmsMsgLowVoiceQualityEvent));         
      }
      break;

      default:
         break;
   }

   if ((ret = cmsMsg_send(msgHandle, pMsg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send 0x%08x msg (0x%08x wordData) to voice, ret=%d",
                   type, wordData, ret);
   }
   else
   {
      cmsLog_debug("sent 0x%08x msg (0x%08x wordData) to voice, ret=%d",
                   type, wordData, ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(pMsgBuf);
}
#endif /* SUPPORT_BAS2 */

/*****************************************************************************
**  FUNCTION:       sendMsgToVoiceWithData
**
**  PURPOSE:        Sends a message to voice application.
**
**  INPUT PARMS:    type  - message type
**                  wordData - word data
**                  pData - data in string format
**
**  OUTPUT PARMS:
**
**  RETURNS:        CMSRET_SUCCESS if message has been sent successfully.
**                  Error otherwise.
**
*****************************************************************************/
static void sendMsgToVoiceWithData(CmsMsgType type, UINT32 wordData, char *pData)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader* pMsg;
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   void *pMsgBuf;
   int sizeMsg;

   /* Create message of proper size */
   switch (type)
   {
      case CMS_MSG_VOICE_DECT_CALL_CTL_CMD:
         sizeMsg = sizeof(CmsMsgHeader) + sizeof(DectCallCtlCmdBody);
         break;

      case CMS_MSG_VOICE_DIAG:
         sizeMsg = sizeof(CmsMsgHeader) + sizeof(VoiceDiagMsgBody);
         break;

      case CMS_MSG_WAN_CONNECTION_UP:
      case CMS_MSG_WAN_CONNECTION_DOWN:
         sizeMsg = sizeof(CmsMsgHeader) + strlen((char *)pData) + 1;
         break;

      case CMS_MSG_VOICE_CONFIG_CHANGED:
         sizeMsg = sizeof(CmsMsgHeader) + strlen((char *)pData) + 1;
         break;

      default:
         sizeMsg = sizeof(CmsMsgHeader);
         break;
   }

   if ( (pMsgBuf = cmsMem_alloc( sizeMsg,
                                 ALLOC_ZEROIZE )) == NULL )
   {
      cmsLog_error("can't allocate memory");
      return;
   }

   /* prepared message header and body */
   pMsg  = (CmsMsgHeader *) pMsgBuf;

   pMsg->type = type;
   pMsg->src = cmsMsg_getHandleEid(msgHandle);
   pMsg->dst = EID_VOICE;
   pMsg->flags_event = 1;
   pMsg->wordData = wordData;
   pMsg->dataLength = sizeMsg - sizeof(CmsMsgHeader);

   /* Fill data if required */
   switch (type)
   {
      case CMS_MSG_VOICE_DECT_CALL_CTL_CMD:
      {
         DectCallCtlCmdBody* info = (DectCallCtlCmdBody *)(pMsg + 1);

         /* Convert comma separated tokens to parameters */
         int i = 0;
         char *saveptr = NULL;
         char *token;
         char *str = pData;

         for (i = 0; /* Forever */; i++, str = NULL)
         {
            token = strtok_r(str, ",", &saveptr);
            if (NULL == token)
            {
               break;
            }

            switch (i)
            {
               case 0:
                  info->cmd = atoi(token);
                  break;
               case 1:
                  info->lineId = atoi(token);
                  break;
               case 2:
                  info->connectionId = atoi(token);
                  break;
               case 3:
                  info->dstHandsetId = atoi(token);
                  break;
               case 4:
                  info->srcHandsetId = atoi(token);
                  break;
               default:
                  cmsLog_error("Something went wrong, this should never happen");
                  break;
            }
         }
      }
      break;

      case CMS_MSG_VOICE_DIAG:
      {
         VoiceDiagMsgBody *pInfo = (VoiceDiagMsgBody *)(pMsg + 1);
         char *token;
         char *str = pData;
         token = strtok(str, ",");
         if (NULL == token)
         {
            cmsLog_error("Cannot tokenize input diag string %s", pData);
            CMSMEM_FREE_BUF_AND_NULL_PTR(pMsgBuf);
            return;
         }
         pInfo->type = atoi(token);
         /* As we only add a ',' in processVoiceDiagCmd(), copy the rest of the string */
         str = pData + strlen(token) + 1;
         strncpy(pInfo->cmdLine, str, sizeof(pInfo->cmdLine) - 1); // Leave room for NULL char
      }
      break;

      case CMS_MSG_WAN_CONNECTION_UP:
      case CMS_MSG_WAN_CONNECTION_DOWN:
         /* Size has already been allocated above */
         strcpy((char *)(pMsg + 1), (char *)pData);
         break;

      case CMS_MSG_VOICE_CONFIG_CHANGED:
         /* Size has already been allocated above */
         strcpy((char *)(pMsg + 1), (char *)pData);
         break;

      default:
         break;
   }

   if ((ret = cmsMsg_send(msgHandle, pMsg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send 0x%08x msg to voice, ret=%d", type, ret);
   }
   else
   {
      cmsLog_debug("sent 0x%08x msg to voice, ret=%d", type, ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(pMsgBuf);
}

/*****************************************************************************
**  FUNCTION:       sendMsgToCbThread
**
**  PURPOSE:        Sends a message to voice callback thread.
**
**  INPUT PARMS:    type     - message type
**                  wordData - word data
**
**  OUTPUT PARMS:
**
**  RETURNS:        CMSRET_SUCCESS if message has been sent successfully.
**                  Error otherwise
**
*****************************************************************************/
static void sendMsgToCbThread(CmsMsgType type, UINT32 wordData)
{
#ifdef BRCM_BDK_BUILD
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader msg = EMPTY_MSG_HEADER;
   void *msgHandle = cmsMdm_getThreadMsgHandle();

   msg.type = type;
   msg.src = cmsMsg_getHandleEid(msgHandle);
   msg.dst = EID_VOICE_HAL_THREAD;
   msg.flags_event = 1;    /* This is an event message */
   msg.wordData = wordData;

   if ((ret = cmsMsg_send(msgHandle, &msg)) != CMSRET_SUCCESS)
      cmsLog_error("could not send msg to voice, ret=%d", ret);
   else
      cmsLog_debug("sent msg to CB thread, ret=%d", ret);
#endif /* BRCM_BDK_BUILD */
}

CmsRet rcl_voiceObject( _VoiceObject *newObj,
                const _VoiceObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   CmsRet ret = CMSRET_SUCCESS;

   if ((newObj != NULL && currObj == NULL)) /* new instance creation */
   {
       /* remove voiceServiceNumberOfEntries counter because
        * TR98 data model doesn't support counter
        */
   }
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      void *msgHandle = cmsMdm_getThreadMsgHandle();
      CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

      /* If loglevel changed just inform voice and return */
      if ( cmsUtl_strcmp(newObj->X_BROADCOM_COM_LoggingLevel,
                         currObj->X_BROADCOM_COM_LoggingLevel) )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GLOBLOGLEVEL);
         return CMSRET_SUCCESS;
      }
      else if ( cmsUtl_strcmp(newObj->X_BROADCOM_COM_LoggingDestination,
                              currObj->X_BROADCOM_COM_LoggingDestination))
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_MODLOGLEVEL);
         return CMSRET_SUCCESS;
      }
      else if ( cmsUtl_strcmp(newObj->X_BROADCOM_COM_ModuleLogLevels,
                              currObj->X_BROADCOM_COM_ModuleLogLevels) )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_MODLOGLEVEL);
         return CMSRET_SUCCESS;
      }
      else if ( cmsUtl_strcmp(newObj->X_BROADCOM_COM_CCTKTraceLevel,
                              currObj->X_BROADCOM_COM_CCTKTraceLevel) )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_MODLOGLEVEL);
         return CMSRET_SUCCESS;
      }
      else if ( cmsUtl_strcmp(newObj->X_BROADCOM_COM_CCTKTraceGroup,
                              currObj->X_BROADCOM_COM_CCTKTraceGroup) )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_MODLOGLEVEL);
         return CMSRET_SUCCESS;
      }
      else if(cmsUtl_strcmp(newObj->X_BROADCOM_COM_BoundIfName,
                            currObj->X_BROADCOM_COM_BoundIfName))
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_BOUNDIF);
         sendMsgToCbThread(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_BOUNDIF);
         return CMSRET_SUCCESS;
      }
      else if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_ManagementProtocol,
                             currObj->X_BROADCOM_COM_ManagementProtocol))
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_MGTPROT);
         return CMSRET_SUCCESS;
      }
      else if ( myEid != EID_VOICE )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
      }
   }
   if ((newObj == NULL && currObj != NULL)) /* delete */
   {
       /* remove voiceServiceNumberOfEntries counter because
        * TR98 data model doesn't support counter
        */
   }

   return ret;
}

CmsRet rcl_voiceMsgReqObject( _VoiceMsgReqObject *newObj,
                const _VoiceMsgReqObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /* If object is being modified */
   if ( newObj != NULL && currObj != NULL )
   {
      if (newObj->X_BROADCOM_COM_MsgType)
      {
         if (newObj->X_BROADCOM_COM_MsgType == CMS_MSG_VOICE_DIAG)
         {
            sendMsgToVoiceWithData(newObj->X_BROADCOM_COM_MsgType, 0, newObj->X_BROADCOM_COM_MsgData);
         }
         else
         {
            sendMsgToVoice(newObj->X_BROADCOM_COM_MsgType, 0);
         }

         newObj->X_BROADCOM_COM_MsgType = 0;
      }
      else if (newObj->X_BROADCOM_COM_OmciMibReset)
      {
         newObj->X_BROADCOM_COM_OmciMibReset = FALSE;
         /* Check whether voice is managed by OMCI */
         _VoiceObject *vObj;
         if ( cmsObj_get( MDMOID_VOICE, iidStack, 0, (void **) &vObj ) == CMSRET_SUCCESS )
         {
            if (!cmsUtl_strcmp(vObj->X_BROADCOM_COM_ManagementProtocol, MDMVS_OMCI))
            {
               /* Stop voice */
               sendMsgToVoice(CMS_MSG_VOICE_STOP, 0);
               /* Default voice */
               sendMsgToVoice(CMS_MSG_VOICE_DEFAULT, 0);
            }
            cmsObj_free((void **)&vObj);
         }
      }
      else if (newObj->X_BROADCOM_COM_OmciCfgUpldComplete)
      {
         newObj->X_BROADCOM_COM_OmciCfgUpldComplete = FALSE;
         /* Check whether voice is managed by OMCI */
         _VoiceObject *vObj;
         if ( cmsObj_get( MDMOID_VOICE, iidStack, 0, (void **) &vObj ) == CMSRET_SUCCESS )
         {
            if (!cmsUtl_strcmp(vObj->X_BROADCOM_COM_ManagementProtocol, MDMVS_OMCI))
            {
               /* Start voice */
               sendMsgToVoice(CMS_MSG_VOICE_START, 0);
               /* Send upload complete */
               sendMsgToVoice(CMS_MSG_VOICE_OMCI_UPLD_COMPLETE, 0);
            }
            cmsObj_free((void **)&vObj);
         }
      }
      else if (newObj->X_BROADCOM_COM_DectMsgType)
      {
         /* Send message to dectd - NOT SUPPORTED YET FOR TR104v2 */

         newObj->X_BROADCOM_COM_DectMsgType = 0;
      }
   }

   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceSysAccessObject( _VoiceSysAccessObject *newObj,
                const _VoiceSysAccessObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    /* If object is being modified */
    if ( newObj != NULL && currObj != NULL )
    {
       if (!currObj->X_BROADCOM_COM_IfName
           || !newObj->X_BROADCOM_COM_IfName
           || cmsUtl_strcmp(newObj->X_BROADCOM_COM_IfName,
                            currObj->X_BROADCOM_COM_IfName))
       {
          /* If interface has changed, don't do anything */
       }
       /* Check if any parameters are different */
       else if (currObj->X_BROADCOM_COM_IsPhyUp != newObj->X_BROADCOM_COM_IsPhyUp
                || currObj->X_BROADCOM_COM_IsIpv4Up != newObj->X_BROADCOM_COM_IsIpv4Up
                || currObj->X_BROADCOM_COM_IsIpv6Up != newObj->X_BROADCOM_COM_IsIpv6Up)
       {
          CmsMsgType msgType;
          UBOOL8 isLan = cmsUtl_strcmp(newObj->X_BROADCOM_COM_IfName, BRIDGE_IF_NAME)
                         ? FALSE : TRUE;

          /* Set message */
          if (newObj->X_BROADCOM_COM_IsPhyUp
              && (newObj->X_BROADCOM_COM_IsIpv4Up || newObj->X_BROADCOM_COM_IsIpv6Up)
             )
          {
             if (isLan)
             {
                msgType = CMS_MSG_LAN_CONNECTION_UP;
             }
             else
             {
                msgType = CMS_MSG_WAN_CONNECTION_UP;
             }
          }
          else
          {
             if (isLan)
             {
                msgType = CMS_MSG_LAN_CONNECTION_DOWN;
             }
             else
             {
                msgType = CMS_MSG_WAN_CONNECTION_DOWN;
             }
          }

          if (isLan)
          {
             sendMsgToVoice(msgType, 0);
          }
          else
          {
             sendMsgToVoiceWithData(msgType, 0, newObj->X_BROADCOM_COM_IfName);
          }
       }
       else if (cmsUtl_strcmp(currObj->X_BROADCOM_COM_Ipv4Addr,
                              newObj->X_BROADCOM_COM_Ipv4Addr)
                || cmsUtl_strcmp(currObj->X_BROADCOM_COM_Ipv4DnsServers,
                                 newObj->X_BROADCOM_COM_Ipv4DnsServers)
               )
       {
          sendMsgToVoice(CMS_MSG_DHCPC_STATE_CHANGED, 0);
       }
       else if (cmsUtl_strcmp(currObj->X_BROADCOM_COM_Ipv6Addr,
                              newObj->X_BROADCOM_COM_Ipv6Addr)
                || cmsUtl_strcmp(currObj->X_BROADCOM_COM_Ipv6DnsServers,
                                 newObj->X_BROADCOM_COM_Ipv6DnsServers)
               )
       {
          sendMsgToVoice(CMS_MSG_DHCP6C_STATE_CHANGED, 0);
       }
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCapObject( _VoiceCapObject *newObj,
                const _VoiceCapObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCapSipObject( _VoiceCapSipObject *newObj,
                const _VoiceCapSipObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCapSipClientObject( _VoiceCapSipClientObject *newObj,
                const _VoiceCapSipClientObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCapPotsObject( _VoiceCapPotsObject *newObj,
                const _VoiceCapPotsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceMtaObject( _VoiceMtaObject *newObj,
                const _VoiceMtaObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_voiceCapCodecsObject
**
**  PURPOSE:        Adds, modifies or deletes a voice capabilities object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_voiceCapCodecsObject( _VoiceCapCodecsObject *newObj,
                const _VoiceCapCodecsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    VoiceCapObject * vObj = NULL;
    InstanceIdStack localIidStack = EMPTY_INSTANCE_ID_STACK;

    localIidStack = *iidStack;
    POP_INSTANCE_ID( &localIidStack ); /* switch to Voice Service level */

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_get( MDMOID_VOICE_CAP, &localIidStack, 0, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->codecNumberOfEntries++;
            cmsObj_set((const void *)vObj, &localIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get object, ret =%d\n", ret);
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_get( MDMOID_VOICE_CAP, &localIidStack, 0, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->codecNumberOfEntries--;
            cmsObj_set((const void *)vObj, &localIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get object, ret =%d\n", ret);
        }
    }
 
    return ret;
}

CmsRet rcl_qualityIndicatorObject( _QualityIndicatorObject *newObj,
                const _QualityIndicatorObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceReservedPortsObject( _VoiceReservedPortsObject *newObj,
                const _VoiceReservedPortsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceServicePotsObject( _VoiceServicePotsObject *newObj,
                const _VoiceServicePotsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_potsRingerObject( _PotsRingerObject *newObj,
                const _PotsRingerObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_pOTSFxoObject
**
**  PURPOSE:        Adds, modifies or deletes a POTS FXO object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_pOTSFxoObject( _POTSFxoObject *newObj,
                const _POTSFxoObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    VoiceServicePotsObject *vObj = NULL;
    InstanceIdStack localIidStack = EMPTY_INSTANCE_ID_STACK;

    localIidStack = *iidStack;
    POP_INSTANCE_ID( &localIidStack ); /* switch to Voice Service level */

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_get( MDMOID_VOICE_SERVICE_POTS, &localIidStack, 0, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->FXONumberOfEntries++;
            cmsObj_set((const void *)vObj, &localIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get object, ret =%d\n", ret);
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_get( MDMOID_VOICE_SERVICE_POTS, &localIidStack, 0, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->FXONumberOfEntries--;
            cmsObj_set((const void *)vObj, &localIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get object, ret =%d\n", ret);
        }
    }
 
    return ret;
}

/*****************************************************************************
**  FUNCTION:       rcl_pOTSFxsObject
**
**  PURPOSE:        Adds, modifies or deletes a POTS FXS object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_pOTSFxsObject( _POTSFxsObject *newObj,
                const _POTSFxsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    VoiceServicePotsObject *vObj = NULL;
    InstanceIdStack localIidStack = EMPTY_INSTANCE_ID_STACK;

    localIidStack = *iidStack;
    POP_INSTANCE_ID( &localIidStack ); /* switch to Voice Service level */

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_get( MDMOID_VOICE_SERVICE_POTS, &localIidStack, 0, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->FXSNumberOfEntries++;
            cmsObj_set((const void *)vObj, &localIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get object, ret =%d\n", ret);
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_get( MDMOID_VOICE_SERVICE_POTS, &localIidStack, 0, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->FXSNumberOfEntries--;
            cmsObj_set((const void *)vObj, &localIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get object, ret =%d\n", ret);
        }
    }
 
    return ret;
}

CmsRet rcl_voiceProcessingObject( _VoiceProcessingObject *newObj,
                const _VoiceProcessingObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_diagTestsObject( _DiagTestsObject *newObj,
                const _DiagTestsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet   ret;

    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        void *msgHandle = cmsMdm_getThreadMsgHandle();
        CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

        if( cmsUtl_strcmp(newObj->diagnosticsState, currObj->diagnosticsState) )
        {
            if( !cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) && 
                 myEid != EID_VOICE )
            {
                /* setting diagnosticsState to "Requested" will trigger MLT
                 * testing by sending request message to voice application
                 */
                int  fxsNo;
                char buf[sizeof(CmsMsgHeader) + sizeof(VoiceDiagMsgBody)];
                
                CmsMsgHeader *msg = (CmsMsgHeader *) buf;
                VoiceDiagMsgBody *info = (VoiceDiagMsgBody *) &(buf[sizeof(CmsMsgHeader)]);

                memset(buf, 0, sizeof(buf));
                msg = (CmsMsgHeader *) buf;
                info = (VoiceDiagMsgBody *) &(buf[sizeof(CmsMsgHeader)]);
                /* setup messege header */
                msg->type = CMS_MSG_VOICE_DIAG;
                msg->src = cmsMsg_getHandleEid(msgHandle);
                msg->dst = EID_VOICE;
                msg->flags_request = 1;
                msg->dataLength = sizeof(VoiceDiagMsgBody);

                /* setup message body */
                rutVoice_mapL2ObjInstToNum( MDMOID_DIAG_TESTS, (InstanceIdStack *)iidStack, &fxsNo );

                msg->wordData = (UINT32)fxsNo;
                info->type = VOICE_DIAG_LINE_TEST_CMD;
                snprintf(info->cmdLine, sizeof(info->cmdLine), "%s", newObj->testSelector );

                if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
                {
                     cmsLog_error("could not send reset statistics message msg to Call Manager, ret=%d", ret);
                }
            }
        }
        else if ( !cmsUtl_strcmp(newObj->testSelector, currObj->testSelector) ||
             !cmsUtl_strcmp(newObj->testResult, currObj->testResult) )
        {
            /* according to TR104v2 spec, the value of diagnosticsState
             * should be reset to "None"
             */
            CMSMEM_REPLACE_STRING_FLAGS(newObj->diagnosticsState, MDMVS_NONE, mdmLibCtx.allocFlags);
        }

    }
    return CMSRET_SUCCESS;
}

CmsRet rcl_fXODiagTestsObject( _FXODiagTestsObject *newObj,
                const _FXODiagTestsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dECTObject( _DECTObject *newObj,
                const _DECTObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dECTPortableObject( _DECTPortableObject *newObj,
                const _DECTPortableObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dECTBaseObject( _DECTBaseObject *newObj,
                const _DECTBaseObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceServiceSipObject( _VoiceServiceSipObject *newObj,
                const _VoiceServiceSipObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        void *msgHandle = cmsMdm_getThreadMsgHandle();
        CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

        /* notify voice to restart */
        if ( myEid != EID_VOICE )
        {
            sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
        }
    }
    return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_sipClientObject
**
**  PURPOSE:        Adds, modifies or deletes a SIP client object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_sipClientObject( _SipClientObject *newObj,
                const _SipClientObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    VoiceServiceSipObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE_SERVICE_SIP, MDMOID_SIP_CLIENT, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->clientNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        void *msgHandle = cmsMdm_getThreadMsgHandle();
        CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

        /* notify voice to restart */
        if ( myEid != EID_VOICE )
        {
            sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
        }
        if (cmsUtl_strcmp(newObj->status, currObj->status))
        {
           /* Registration status has changed. Send corresponding message to
            * voice app, which will trigger MTA callback and/or perform
            * other applicable actions. Only applicable to BDK build */
           sendMsgToCbThread(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_REGSTATUS);
        }
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE_SERVICE_SIP, MDMOID_SIP_CLIENT, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->clientNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_sIPClientContactObject
**
**  PURPOSE:        Adds, modifies or deletes a SIP client object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_sIPClientContactObject( _SIPClientContactObject *newObj,
                const _SIPClientContactObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    _SipClientObject *vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        /* enable object automatically */
        newObj->enable = TRUE;

        if ( (ret = cmsObj_getAncestor( MDMOID_SIP_CLIENT, MDMOID_SIP_CLIENT_CONTACT, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->contactNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        void *msgHandle = cmsMdm_getThreadMsgHandle();
        CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

        /* notify voice to restart */
        if ( myEid != EID_VOICE )
        {
            sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
        }
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_SIP_CLIENT, MDMOID_SIP_CLIENT_CONTACT, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->contactNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    return CMSRET_SUCCESS;
}

#define UINT32_STR_MAX_SIZE  16
/*****************************************************************************
**  FUNCTION:       rcl_sIPNetworkObject
**
**  PURPOSE:        Adds, modifies or deletes a SIP network object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_sIPNetworkObject( _SIPNetworkObject *newObj,
                const _SIPNetworkObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    VoiceServiceSipObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE_SERVICE_SIP, MDMOID_SIP_CLIENT, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->networkNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        void *msgHandle = cmsMdm_getThreadMsgHandle();
        CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

        /* notify voice to restart */
        if ( myEid != EID_VOICE )
        {
            sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
        }

        /* X_BROADCOM_COM_NfMWISubDuration */
        if (newObj->X_BROADCOM_COM_NfMWISubDuration != currObj->X_BROADCOM_COM_NfMWISubDuration)
        {
            /* Get network instance */
            UINT32 netInst = PEEK_INSTANCE_ID(iidStack);
            if ( netInst > 0 )
            {
                char szNetInst[UINT32_STR_MAX_SIZE] = {0};
                sprintf( szNetInst, "%u", netInst );

                /* Restart MWI service */
                sendMsgToVoiceWithData( CMS_MSG_VOICE_CONFIG_CHANGED,
                                        CMSMSGVOICECONFIGCHANGED_SVCRESTART_MWI,
                                        szNetInst );
            }
        }
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE_SERVICE_SIP, MDMOID_SIP_CLIENT, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->networkNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
 
    return CMSRET_SUCCESS;
}

CmsRet rcl_sIPNetworkAnnouncementObject( _SIPNetworkAnnouncementObject *newObj,
                const _SIPNetworkAnnouncementObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;

    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        /* Check to see if FileName, AdminStatus, ServerAddressType, or ServerAddress
         * has changed.
         */
        if ((cmsUtl_strcmp(newObj->adminStatus, currObj->adminStatus)
             && newObj->adminStatus[0] != '\0')
            || (cmsUtl_strcmp(newObj->fileName, currObj->fileName)
                && newObj->fileName[0] != '\0')
            || (cmsUtl_strcmp(newObj->serverAddressType, currObj->serverAddressType)
                && newObj->serverAddressType[0] != '\0')
            || (cmsUtl_strcmp(newObj->serverAddress, currObj->serverAddress)
                && newObj->serverAddress[0] != '\0')
            || (cmsUtl_strcmp(newObj->announcementCtrl, currObj->announcementCtrl))
           )
        {
            /* Package message to send to call manager to attempt to TFTP download
             * voice announcement.
             */
            sendMsgToVoice(CMS_MSG_VOICE_ANNOUNCE_DOWNLOAD, 0);
        }
    }

    return ret;
}

CmsRet rcl_sIPNetworkResponseMapObject( _SIPNetworkResponseMapObject *newObj,
                const _SIPNetworkResponseMapObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_callControlObject( _CallControlObject *newObj,
                const _CallControlObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        void *msgHandle = cmsMdm_getThreadMsgHandle();
        CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

        /* notify voice to restart */
        if ( myEid != EID_VOICE )
        {
            sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
        }
    }
    return CMSRET_SUCCESS;
}

CmsRet rcl_callControlCallingFeaturesObject( _CallControlCallingFeaturesObject *newObj,
                const _CallControlCallingFeaturesObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_callingFeaturesSetObject
**
**  PURPOSE:        Adds, modifies or deletes a calling features object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_callingFeaturesSetObject( _CallingFeaturesSetObject *newObj,
                const _CallingFeaturesSetObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    CallControlCallingFeaturesObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL_CALLING_FEATURES, MDMOID_CALLING_FEATURES_SET, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->setNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    else if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        UINT32 flag, inst;
        char szCallFeatInst[UINT32_STR_MAX_SIZE] = {0};

        /* X_BROADCOM_COM_ClearVMWI default value is -1, other
         * value indicate to set/clear MWI request */
        if (newObj->X_BROADCOM_COM_ClearVMWI >= 0 )
        {
            flag = (newObj->X_BROADCOM_COM_ClearVMWI>0)? CMSMSGVOICECONFIGCHANGED_CLEARMWI : CMSMSGVOICECONFIGCHANGED_SETMWI;

            /* Get Calling Features set instance */
            inst = PEEK_INSTANCE_ID(iidStack);
            sprintf( szCallFeatInst, "%u", inst );

            /* Clear MWI */
            sendMsgToVoiceWithData( CMS_MSG_VOICE_CONFIG_CHANGED,
                                flag,
                                szCallFeatInst );

            /* reset ClearVMWI bit to -1 */
            newObj->X_BROADCOM_COM_ClearVMWI  = -1;
        }

        void *msgHandle = cmsMdm_getThreadMsgHandle();
        CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

        /* if the change is coming from voice, no need to notify itself */

        /* this change limits the total messages generated by dalVoice_SetDefaults()
         * API, when dalVoice_SetDefault is called from voice app context, the cms
         * message send and receive functions are running within same thread; then
         * following sequencial calls will happen:
         *
         *   dalVoice_SetParametersXXX() =>> rcl_voiceXXXObject =>>sendMsgToVoice()=>>cmsMsg_send
         *   dalVoice_SetParametersYYY() =>> rcl_voiceYYYObject =>>sendMsgToVoice()=>>cmsMsg_send
         *   dalVoice_SetParametersZZZ() =>> rcl_voiceZZZObject =>>sendMsgToVoice()=>>cmsMsg_send
         *   ...
         *   cmsMsg_receive();
         *   ...
         *
         * if cms message queue is flooded by cmsMsg_send calls, when the queue is full
         * then cmsMsg_send will blocked waiting queue to be empty; but cmsMsg_receive()
         * can't run until cmsMsg_send returns;
         */
        if ( myEid != EID_VOICE )
        {
            /* regular call feature changed */
            sendMsgToVoiceWithData( CMS_MSG_VOICE_CONFIG_CHANGED,
                                CMSMSGVOICECONFIGCHANGED_GENERAL,    /* General configuration change */
                                szCallFeatInst );
        }
    }
    else if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL_CALLING_FEATURES, MDMOID_CALLING_FEATURES_SET, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->setNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
 
    return CMSRET_SUCCESS;
}

CmsRet rcl_callControlNumberingPlanObject( _CallControlNumberingPlanObject *newObj,
                const _CallControlNumberingPlanObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_callControlPrefixInfoObject( _CallControlPrefixInfoObject *newObj,
                const _CallControlPrefixInfoObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_callControlIncomingMapObject
**
**  PURPOSE:        Adds, modifies or deletes an incoming map object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_callControlIncomingMapObject( _CallControlIncomingMapObject *newObj,
                const _CallControlIncomingMapObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    CallControlObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;
    void *msgHandle = cmsMdm_getThreadMsgHandle();
    CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL, MDMOID_CALL_CONTROL_INCOMING_MAP, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->incomingMapNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        /* notify voice to update routing */
        if ( myEid != EID_VOICE )
        {
            sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_ROUTING);
        }
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL, MDMOID_CALL_CONTROL_INCOMING_MAP, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->incomingMapNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
            /* notify voice to update routing */
            if ( myEid != EID_VOICE )
            {
                sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_ROUTING);
            }
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
 
    return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_callControlOutgoingMapObject
**
**  PURPOSE:        Adds, modifies or deletes an outgoing map object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_callControlOutgoingMapObject( _CallControlOutgoingMapObject *newObj,
                const _CallControlOutgoingMapObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    CallControlObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL, MDMOID_CALL_CONTROL_OUTGOING_MAP, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->outgoingMapNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL, MDMOID_CALL_CONTROL_OUTGOING_MAP, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->outgoingMapNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
 
    return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_callControlExtensionObject
**
**  PURPOSE:        Adds, modifies or deletes a call control extension object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_callControlExtensionObject( _CallControlExtensionObject *newObj,
                const _CallControlExtensionObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    CallControlObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL, MDMOID_CALL_CONTROL_EXTENSION, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->extensionNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        void *msgHandle = cmsMdm_getThreadMsgHandle();
        CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

        /* notify voice to restart */
        if ( myEid != EID_VOICE )
        {
            sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
        }
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL, MDMOID_CALL_CONTROL_EXTENSION, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->extensionNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
 
    return CMSRET_SUCCESS;
}

CmsRet rcl_extensionStatsObject( _ExtensionStatsObject *newObj,
                const _ExtensionStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_extensionIncomingCallsObject( _ExtensionIncomingCallsObject *newObj,
                const _ExtensionIncomingCallsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_extensionOutgoingCallsObject( _ExtensionOutgoingCallsObject *newObj,
                const _ExtensionOutgoingCallsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_extensionRtpStatsObject( _ExtensionRtpStatsObject *newObj,
                const _ExtensionRtpStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_extensionDspStatsObject( _ExtensionDspStatsObject *newObj,
                const _ExtensionDspStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_callControlLineObject
**
**  PURPOSE:        Adds, modifies or deletes a call control line object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_callControlLineObject( _CallControlLineObject *newObj,
                const _CallControlLineObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    CallControlObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL, MDMOID_CALL_CONTROL_LINE, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->lineNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        void *msgHandle = cmsMdm_getThreadMsgHandle();
        CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);
        if ( myEid != EID_VOICE )
        {
            sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
        }
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL, MDMOID_CALL_CONTROL_LINE, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->lineNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
 
    return CMSRET_SUCCESS;
}

CmsRet rcl_lineStatsObject( _LineStatsObject *newObj,
                const _LineStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        if ( newObj->resetStatistics )
        {
            void *msgHandle = cmsMdm_getThreadMsgHandle();

            /* Reset statistics flag set, reset it */
            newObj->resetStatistics = 0;

            CmsMsgHeader msg = EMPTY_MSG_HEADER;
            CmsRet ret;
            UINT32 lineInst;

            /* Get line instance */
            lineInst = PEEK_INSTANCE_ID(iidStack);

            /* Reset Statistics in Call Manager for specific line */
            msg.type = CMS_MSG_VOICE_STATISTICS_RESET;
            msg.src = cmsMsg_getHandleEid(msgHandle);
            msg.dst = EID_VOICE;
            msg.flags_request = 1;
            msg.wordData = lineInst;

            if ((ret = cmsMsg_send(msgHandle, &msg)) != CMSRET_SUCCESS)
            {
                cmsLog_error("could not send reset statistics message msg to Call Manager, ret=%d", ret);
            }
        }
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_lineIncomingCallsObject( _LineIncomingCallsObject *newObj,
                const _LineIncomingCallsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_lineOutgoingCallsObject( _LineOutgoingCallsObject *newObj,
                const _LineOutgoingCallsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_lineRtpStatsObject( _LineRtpStatsObject *newObj,
                const _LineRtpStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_lineDspStatsObject( _LineDspStatsObject *newObj,
                const _LineDspStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallLogObject( _VoiceCallLogObject *newObj,
                const _VoiceCallLogObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    VoiceObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE, MDMOID_VOICE_CALL_LOG, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->callLogNumberOfEntries++;	/**< CallLogNumberOfEntries */
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE, MDMOID_VOICE_CALL_LOG, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->callLogNumberOfEntries--;	/**< CallLogNumberOfEntries */
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
   return CMSRET_SUCCESS;
}

CmsRet rcl_callLogSessionObject( _CallLogSessionObject *newObj,
                const _CallLogSessionObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    _VoiceCallLogObject * vCallLogObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE_CALL_LOG, MDMOID_CALL_LOG_SESSION, &ancestorIidStack, (void **) &vCallLogObj )) == CMSRET_SUCCESS )
        {
            vCallLogObj->sessionNumberOfEntries++;	/**< SessionNumberOfEntries */
            cmsObj_set((const void *)vCallLogObj, &ancestorIidStack);
            cmsObj_free((void **)&vCallLogObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE_CALL_LOG, MDMOID_CALL_LOG_SESSION, &ancestorIidStack, (void **) &vCallLogObj )) == CMSRET_SUCCESS )
        {
            vCallLogObj->sessionNumberOfEntries--;	/**< SessionNumberOfEntries */
            cmsObj_set((const void *)vCallLogObj, &ancestorIidStack);
            cmsObj_free((void **)&vCallLogObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
   return CMSRET_SUCCESS;
}

CmsRet rcl_callLogSessionStatsObject( _CallLogSessionStatsObject *newObj,
                const _CallLogSessionStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   CmsRet ret = CMSRET_SUCCESS;
#ifdef SUPPORT_BAS2
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      if (newObj->MOSLQ != currObj->MOSLQ)
      {
          InstanceIdStack ancestorIidStack = *iidStack;

         /* New MOS score has been set. If low, send an event to BAS */
         if (newObj->MOSLQ < LOW_VOICE_QUALITY_THRESHOLD)
         {
            CmsMsgLowVoiceQualityEvent eventData;
            VoiceCallLogObject *vClObj=NULL;
            CallLogSessionObject *vSesObj=NULL;
            if ( (ret = cmsObj_getAncestor( MDMOID_CALL_LOG_SESSION, MDMOID_CALL_LOG_SESSION_STATS, &ancestorIidStack, (void **) &vSesObj )) == CMSRET_SUCCESS )
            {
               if ( (ret = cmsObj_getAncestor( MDMOID_VOICE_CALL_LOG, MDMOID_CALL_LOG_SESSION, &ancestorIidStack, (void **) &vClObj )) == CMSRET_SUCCESS )
               {
                  /* use ancestor call log object to populate event to BAS */
                  int callLogNumber = 0, lineNum = 0;
                  MdmPathDescriptor pathDesc;
                  
                  /* Obtain call log number from instance ID */
                  rutVoice_mapL2ObjInstToNum( MDMOID_VOICE_CALL_LOG, (InstanceIdStack *)&ancestorIidStack, &callLogNumber );
                  
                  /* Obtain line instance from full path in usedLine parameter, then line number from instance */ 
                  cmsMdm_fullPathToPathDescriptor(vClObj->usedLine, &pathDesc);
                  rutVoice_mapL2ObjInstToNum( MDMOID_CALL_CONTROL_LINE, (InstanceIdStack *)&pathDesc.iidStack, &lineNum );
                  
                  /* Follow the same numbering convention as in BAS voice data query responses. I.e. line 0 is line 1 for BAS, call log 0 is call log 1 etc */
                  eventData.lineNum = lineNum+1;
                  eventData.sessionNum = callLogNumber+1;

                  sendMsgToBasClientVoiceWithData(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_LOW_VOICE_QUALITY, &eventData);

                  cmsObj_free((void **)&vClObj);
               }
               else
               {
                  cmsLog_error("Unable to get ancestor object, ret =%d", ret);
               }
               cmsObj_free((void **)&vSesObj);
            }
            else
            {
               cmsLog_error("Unable to get ancestor object, ret =%d", ret);
            }
         }
      }
   }
#endif
   return ret;
}

CmsRet rcl_callLogSessionDestinationObject( _CallLogSessionDestinationObject *newObj,
                const _CallLogSessionDestinationObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_destinationVoiceQualityObject( _DestinationVoiceQualityObject *newObj,
                const _DestinationVoiceQualityObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_sessionDestinationDspObject( _SessionDestinationDspObject *newObj,
                const _SessionDestinationDspObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_destinationDspReceiveCodecObject( _DestinationDspReceiveCodecObject *newObj,
                const _DestinationDspReceiveCodecObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_destinationDspTransmitCodecObject( _DestinationDspTransmitCodecObject *newObj,
                const _DestinationDspTransmitCodecObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_sessionDestinationRtpObject( _SessionDestinationRtpObject *newObj,
                const _SessionDestinationRtpObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_sessionSourceObject( _SessionSourceObject *newObj,
                const _SessionSourceObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_sourceVoiceQualityObject( _SourceVoiceQualityObject *newObj,
                const _SourceVoiceQualityObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_sessionSourceDspObject( _SessionSourceDspObject *newObj,
                const _SessionSourceDspObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_sourceDSPReceiveCodecObject( _SourceDSPReceiveCodecObject *newObj,
                const _SourceDSPReceiveCodecObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_sourceDSPTransmitCodecObject( _SourceDSPTransmitCodecObject *newObj,
                const _SourceDSPTransmitCodecObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_sessionSourceRtpObject( _SessionSourceRtpObject *newObj,
                const _SessionSourceRtpObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_callLogSignalingPerformanceObject( _CallLogSignalingPerformanceObject *newObj,
                const _CallLogSignalingPerformanceObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_voIPProfileObject
**
**  PURPOSE:        Adds, modifies or deletes a VOIP profile object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_voIPProfileObject( _VoIPProfileObject *newObj,
                const _VoIPProfileObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    VoiceObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE, MDMOID_IP_PROFILE, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->voIPProfileNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE, MDMOID_IP_PROFILE, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->voIPProfileNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    return CMSRET_SUCCESS;
}

CmsRet rcl_voIPProfileRTPObject( _VoIPProfileRTPObject *newObj,
                const _VoIPProfileRTPObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voIPProfileFaxT38Object( _VoIPProfileFaxT38Object *newObj,
                const _VoIPProfileFaxT38Object *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voIPProfileRTPRedundancyObject( _VoIPProfileRTPRedundancyObject *newObj,
                const _VoIPProfileRTPRedundancyObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voIPProfileSRTPObject( _VoIPProfileSRTPObject *newObj,
                const _VoIPProfileSRTPObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voIPProfileRTCPObject( _VoIPProfileRTCPObject *newObj,
                const _VoIPProfileRTCPObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_codecProfileObject
**
**  PURPOSE:        Adds, modifies or deletes a codec profile object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_codecProfileObject( _CodecProfileObject *newObj,
                const _CodecProfileObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    VoiceObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE, MDMOID_CODEC_PROFILE, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->codecProfileNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE, MDMOID_CODEC_PROFILE, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->codecProfileNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);

            UINT32 vpInst;

            /* Get voice profile instance */
            vpInst = PEEK_INSTANCE_ID(&ancestorIidStack);

            /* Notify voice that the codec profile has changed. */
            sendMsgToVoice( CMS_MSG_VOICE_CODEC_PROFILE_DELETE, vpInst );
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    return CMSRET_SUCCESS;
}

CmsRet rcl_voiceServiceContactObject( _VoiceServiceContactObject *newObj,
                const _VoiceServiceContactObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_contactNumberObject( _ContactNumberObject *newObj,
                const _ContactNumberObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceDebugInfoObject( _VoiceDebugInfoObject *newObj,
                const _VoiceDebugInfoObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

#endif /* DMP_VOICE_SERVICE_2 */
#endif /* BRCM_VOICE_SUPPORT */
