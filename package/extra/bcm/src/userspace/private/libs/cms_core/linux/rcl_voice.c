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

#include <time.h>
#include "rcl.h"
#include "cms_util.h"
#include "rut_util.h"
#include "mdm.h"
#include "sys/time.h"
#include "rut_wan.h"


#ifdef BRCM_VOICE_SUPPORT
#include "dect_msg.h"
#include "rut_voice.h"
#include <cms_log.h>
#include "bcm_dsphal.h"

/* mwang_todo: these handler function should be put in more precise
 * DMP_<profilename> defines.
 */

#define BRIDGE_IF_NAME "br0"

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
**  FUNCTION:       sendMsgToBasClientVoice
**
**  PURPOSE:        Sends a message to voice BAS client application.
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
static void sendMsgToBasClientVoice(CmsMsgType type, UINT32 wordData)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader msg = EMPTY_MSG_HEADER;
   void *msgHandle = cmsMdm_getThreadMsgHandle();

   msg.type = type;
   msg.src = cmsMsg_getHandleEid(msgHandle);
   msg.dst = EID_BAS_CLIENT_VOICE;
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
   void *pMsgBuf;
   void *msgHandle = cmsMdm_getThreadMsgHandle();
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

      default:
         sizeMsg = sizeof(CmsMsgHeader);
         break;
   }

   if ( (pMsgBuf = cmsMem_alloc( sizeMsg,
                                 ALLOC_ZEROIZE )) == NULL)
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

/*****************************************************************************
**  FUNCTION:       sendMsgToDect
**
**  PURPOSE:        Sends a message to dectd application.
**
**  INPUT PARMS:    type  - message type
**                  pData - data in string format
**
**  OUTPUT PARMS:
**
**  RETURNS:        CMSRET_SUCCESS if message has been sent successfully.
**                  Error otherwise.
**
*****************************************************************************/
static void sendMsgToDect(CmsMsgType type, char *pData)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader* pMsg;
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   void *pMsgBuf;
   int sizeMsg;

   /* Create message of proper size */
   switch (type)
   {
      case CMS_MSG_VOICE_DECT_MEM_SET:
      case CMS_MSG_VOICE_DECT_MEM_GET:
      case CMS_MSG_VOICE_DECT_MODE_SET:
         sizeMsg = sizeof(CmsMsgHeader) + sizeof(VoiceDiagMsgBody);
         break;

      case CMS_MSG_VOICE_DECT_CM_EVENT:
         sizeMsg = sizeof(CmsMsgHeader) + sizeof(DectCMEventMsgBody);
         break;

      case CMS_MSG_VOICE_DECT_AC_SET:
         sizeMsg = sizeof(CmsMsgHeader) + strlen(pData);
         break;

      default:
         sizeMsg = sizeof(CmsMsgHeader);
         break;
   }

   if ( (pMsgBuf = cmsMem_alloc( sizeMsg,
                                 ALLOC_ZEROIZE )) == NULL)
   {
      cmsLog_error("can't allocate memory");
      return;
   }

   /* prepared message header and body */
   pMsg  = (CmsMsgHeader *) pMsgBuf;

   pMsg->type = type;
   pMsg->src = cmsMsg_getHandleEid(msgHandle);
   pMsg->dst = EID_DECT;
   pMsg->flags_event = 1;
   pMsg->dataLength = sizeMsg - sizeof(CmsMsgHeader);

   /* Fill data if required */
   switch (type)
   {
      case CMS_MSG_VOICE_DECT_MEM_SET:
      case CMS_MSG_VOICE_DECT_MEM_GET:
      case CMS_MSG_VOICE_DECT_MODE_SET:
      {
         VoiceDiagMsgBody* info = (VoiceDiagMsgBody *)(pMsg + 1);

         switch (type)
         {
            case CMS_MSG_VOICE_DECT_MEM_SET:
               info->type = VOICE_DIAG_DECT_MEM_SET;
               break;

            case CMS_MSG_VOICE_DECT_MEM_GET:
               info->type = VOICE_DIAG_DECT_MEM_GET;
               break;

            case CMS_MSG_VOICE_DECT_MODE_SET:
               info->type = VOICE_DIAG_DECT_MODE_SET;
               break;

            default:
               cmsLog_error("Something went wrong, this should never happen");
               break;
         }

         strncpy(info->cmdLine, pData, sizeof(info->cmdLine)-1);
         info->cmdLine[sizeof(info->cmdLine)-1] = '\0';
      }
      break;

      case CMS_MSG_VOICE_DECT_CM_EVENT:
      {
         DectCMEventMsgBody* info = (DectCMEventMsgBody *)(pMsg + 1);

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
                  info->event = atoi(token);
                  break;
               case 1:
                  info->lineId = atoi(token);
                  break;
               case 2:
                  info->data[0] = atoi(token);
                  break;
               default:
                  cmsLog_error("Something went wrong, this should never happen");
                  break;
            }
         }

         /* Fill remaining items */
         info->handsetId = 1;
         info->connectionId = 1;
         info->dataLength = 1;
      }
      break;

      case CMS_MSG_VOICE_DECT_AC_SET:
         strncpy( (char *)(pMsg + 1), pData, pMsg->dataLength );
         break;

      case CMS_MSG_VOICE_DECT_HS_DELETE:
      case CMS_MSG_VOICE_DECT_HS_PING:
         pMsg->wordData = atoi(pData);
         break;

      default:
         break;
   }

   if ((ret = cmsMsg_send(msgHandle, pMsg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send 0x%08x msg to dectd, ret=%d", type, ret);
   }
   else
   {
      cmsLog_debug("sent 0x%08x msg to dectd, ret=%d", type, ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(pMsgBuf);
}

CmsRet rcl_voiceObject( _VoiceObject *newObj,
                const _VoiceObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

   if ( myEid == EID_VOICE )
   {
      return CMSRET_SUCCESS;
   }

   /* If object is being modified */
   if ( newObj != NULL && currObj != NULL )
   {
      /* If loglevel changed just inform voice and return */
      if(cmsUtl_strcmp(newObj->X_BROADCOM_COM_LoggingLevel, currObj->X_BROADCOM_COM_LoggingLevel))
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GLOBLOGLEVEL);
         return CMSRET_SUCCESS;
      }
      else if(cmsUtl_strcmp(newObj->X_BROADCOM_COM_LoggingDestination, currObj->X_BROADCOM_COM_LoggingDestination))
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_MODLOGLEVEL);
         return CMSRET_SUCCESS;
      }
      else if(cmsUtl_strcmp(newObj->X_BROADCOM_COM_ModuleLogLevels, currObj->X_BROADCOM_COM_ModuleLogLevels))
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_MODLOGLEVEL);
         return CMSRET_SUCCESS;
      }
      else if(cmsUtl_strcmp(newObj->X_BROADCOM_COM_CCTKTraceLevel, currObj->X_BROADCOM_COM_CCTKTraceLevel))
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_MODLOGLEVEL);
         return CMSRET_SUCCESS;
      }
      else if(cmsUtl_strcmp(newObj->X_BROADCOM_COM_CCTKTraceGroup, currObj->X_BROADCOM_COM_CCTKTraceGroup))
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_MODLOGLEVEL);
         return CMSRET_SUCCESS;
      }
      else if(cmsUtl_strcmp(newObj->X_BROADCOM_COM_BoundIfName, currObj->X_BROADCOM_COM_BoundIfName))
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_BOUNDIF);
         sendMsgToCbThread(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_BOUNDIF);
         return CMSRET_SUCCESS;
      }
      else if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_ManagementProtocol, currObj->X_BROADCOM_COM_ManagementProtocol))
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_MGTPROT);
         return CMSRET_SUCCESS;
      }
#ifdef SUPPORT_BAS2
      else if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_VoiceAppState,
                             currObj->X_BROADCOM_COM_VoiceAppState))
      {
         sendMsgToBasClientVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
         return CMSRET_SUCCESS;
      }
#endif
   }
   else if (newObj == NULL && currObj != NULL)
   {
      /* wow, someone is deleting the voiceservice object?  This shouldn't happen.
       * should I stop voice? */

      cmsLog_error("voiceObject is being deleted!");
   }
   else if (currObj == NULL && newObj != NULL)
   {
      /* New Instance creation */
#if defined(DMP_EPON_VOICE_OAM)
      char opticalWanType[CMS_IFNAME_LENGTH];
#endif /* DMP_EPON_VOICE_OAM */
      
#ifdef DMP_EPON_VOICE_OAM   
      if( rutWan_getOpticalWanType( &opticalWanType[0] ) == CMSRET_SUCCESS )
      {
         if( cmsUtl_strcmp(opticalWanType, MDMVS_EPON) == 0 )
         {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_ManagementProtocol, MDMVS_OAM, mdmLibCtx.allocFlags);
         }
      }
#endif /* DMP_EPON_VOICE_OAM */

   }

   return CMSRET_SUCCESS;
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
         /* Send message to voice */
         if (CMS_MSG_VOICE_DECT_CALL_CTL_CMD == newObj->X_BROADCOM_COM_MsgType 
          || CMS_MSG_VOICE_DIAG              == newObj->X_BROADCOM_COM_MsgType )
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
         /* Send message to dectd */
         sendMsgToDect(newObj->X_BROADCOM_COM_DectMsgType, newObj->X_BROADCOM_COM_MsgData);

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

CmsRet rcl_voiceCapObject( _VoiceCapObject *newObj ,
                const _VoiceCapObject *currObj ,
                const InstanceIdStack *iidStack __attribute__((unused)) ,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /* This object only created on bootup or initialized from XML file */

   if ((newObj != NULL && currObj == NULL)) /* new instance creation */
   {
      int maxLine, maxSess;
      char regionList[BUFLEN_256];
      char sigProt[BUFLEN_256];

      rutVoice_getMaxLine( &maxLine );
      rutVoice_getSupportedAlpha2Locales( regionList, BUFLEN_256-1 );
      rutVoice_getSigProt( sigProt, BUFLEN_256 );
      rutVoice_getMaxSessPerLine( &maxSess );

      /* newObj->maxProfileCount set by voice app */
      newObj->maxLineCount = maxLine;
      newObj->maxSessionsPerLine = maxSess;
      /* newObj->maxSessionCount set by voice app */
      CMSMEM_REPLACE_STRING_FLAGS(newObj->signalingProtocols,sigProt,mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(newObj->regions,regionList,mdmLibCtx.allocFlags);
      newObj->RTCP = 0;
      newObj->SRTP = 0;
      /* newObj->SRTPKeyingMethods; */
      /* newObj->SRTPEncryptionKeySizes; */
      newObj->RTPRedundancy = 0;
      newObj->DSCPCoupled = 0;
      newObj->ethernetTaggingCoupled = 0;
      newObj->PSTNSoftSwitchOver = 0;
      newObj->faxT38  = 1;
      newObj->faxPassThrough = 0;
      newObj->modemPassThrough = 0;
      newObj->toneGeneration = 0;
      /* newObj->toneDescriptionsEditable = 0; */
      /* newObj->patternBasedToneGeneration = 0; */
      /* newObj->fileBasedToneGeneration = 0; */
      /* newObj->toneFileFormats; */
      newObj->ringGeneration = 0;
      /* newObj->ringDescriptionsEditable = 0; */
      /* newObj->patternBasedRingGeneration = 0; */
      /* newObj->ringPatternEditable = 0; */
      /* newObj->fileBasedRingGeneration = 0; */
      /* newObj->ringFileFormats; */
      newObj->digitMap = 1;
      newObj->numberingPlan = 0;
      newObj->buttonMap = 0;
      newObj->voicePortTests = 0;
   }

   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCapSipObject( _VoiceCapSipObject *newObj ,
                const _VoiceCapSipObject *currObj ,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /* This object only created on bootup or initialized from XML file */
   char role[BUFLEN_256];
   char extensions[BUFLEN_256];
   char transports[BUFLEN_256];
   char uriSchemes[BUFLEN_256];

#ifdef SIPLOAD
   rutVoice_getSipRole( role, BUFLEN_256 );
   rutVoice_getSipExtensions( extensions, BUFLEN_256 );
   rutVoice_getSipTransports( transports, BUFLEN_256 );
   rutVoice_getSipUriSchemes( uriSchemes, BUFLEN_256 );
#endif

   CMSMEM_REPLACE_STRING_FLAGS(newObj->role,role,mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(newObj->extensions,extensions,mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(newObj->transports,transports,mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(newObj->URISchemes,uriSchemes,mdmLibCtx.allocFlags);
   newObj->eventSubscription = 0;
   newObj->responseMap = 0;
   /* newObj->TLSAuthenticationProtocols; */
   /* newObj->TLSAuthenticationKeySizes; */
   /* newObj->TLSEncryptionProtocols; */
   /* newObj->TLSEncryptionKeySizes; */
   /* newObj->TLSKeyExchangeProtocols; */
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCapMgcpObject( _VoiceCapMgcpObject *newObj __attribute__((unused)),
                const _VoiceCapMgcpObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

#if 0
CmsRet rcl_voiceCapH323Object( _VoiceCapH323Object *newObj __attribute__((unused)),
                const _VoiceCapH323Object *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif

CmsRet rcl_voiceCapCodecsObject( _VoiceCapCodecsObject *newObj ,
                const _VoiceCapCodecsObject *currObj ,
                const InstanceIdStack *iidStack ,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj == NULL)) /* new instance creation */
   {
      /* Do nothing */
   }
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceProfObject( _VoiceProfObject *newObj,
                const _VoiceProfObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

   if ((newObj != NULL && currObj == NULL)) /* new instance creation */
   {
      VoiceObject * vObj = NULL;
      CmsRet ret;
      InstanceIdStack ancestorIidStack = *iidStack;     

      int spNum = 0;
      int vpInst = 0;
      vpInst = INSTANCE_ID_AT_DEPTH(iidStack, 1);

      /* Check to see if another voice profile object can be created */
      if ( (rutVoice_assignSpNumToVpInst( vpInst, &spNum )) == CMSRET_SUCCESS )
      {
         newObj->X_BROADCOM_COM_SPNum = spNum;
      }
      else
      {
         cmsLog_error("Unable to create new Voice Profile object");
         return CMSRET_INTERNAL_ERROR;
      }

      /* newObj->maxSessions will be set by the voice app */

      /* Update VoiceService.i.voiceProfileNumberOfEntries field */
      /* Get ancestor object */
      if ( (ret = cmsObj_getAncestor( MDMOID_VOICE, MDMOID_VOICE_PROF, &ancestorIidStack, (void **) &vObj )) != CMSRET_SUCCESS )
      {
         cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
         return ret;
      }
      else
      {
         vObj->voiceProfileNumberOfEntries += 1;
         
         /* Set VoiceService.i.voiceProfileNumberOfEntries field */
         if ((ret = cmsObj_set( vObj, &ancestorIidStack )) != CMSRET_SUCCESS )
         {
            cmsLog_error("Unable to set value to mdm, ret = %d\n", ret);
            cmsObj_free((void **) &vObj);
            return ret;
         }

         cmsObj_free((void **) &vObj);         
      }

      
   }
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
      if ( myEid != EID_VOICE )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
      }
   }
   if ((newObj == NULL && currObj != NULL)) /* delete */
   {

      VoiceObject * vObj = NULL;
      CmsRet ret;


      int vpInst = INSTANCE_ID_AT_DEPTH(iidStack, 1); /* vp instance to delete */


      /* Retrieve the voice service instance that is the parent of the voice profile instance to delete */
      InstanceIdStack ancestorIidStack = *iidStack;


      rutVoice_updateSpNum( vpInst );

      /* If change is coming from VOICE, no need to notify VOICE */
      if ( myEid != EID_VOICE )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
      }
      

      if ( (ret = cmsObj_getAncestor( MDMOID_VOICE, MDMOID_VOICE_PROF, &ancestorIidStack, (void **) &vObj )) != CMSRET_SUCCESS )
      {
         cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
         return ret;
      }
      else
      {
         vObj->voiceProfileNumberOfEntries -= 1;
         
         /* Set VoiceService.i.voiceProfileNumberOfEntries field */
         if ((ret = cmsObj_set( vObj, &ancestorIidStack )) != CMSRET_SUCCESS )
         {
            cmsLog_error("Unable to set value to mdm, ret = %d\n", ret);
            cmsObj_free((void **) &vObj);
            return ret;
         }

         cmsObj_free((void **) &vObj);         
      }

   }
   /* Send message to voice to update routing table */
   sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_ROUTING);
   return CMSRET_SUCCESS;
}


CmsRet rcl_voiceProfProviderObject( _VoiceProfProviderObject *newObj ,
                const _VoiceProfProviderObject *currObj ,
                const InstanceIdStack *iidStack ,
                char **errorParam ,
                CmsRet *errorCode )
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_voiceProfSipObject( _VoiceProfSipObject *newObj,
                const _VoiceProfSipObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      void *msgHandle = cmsMdm_getThreadMsgHandle();
      CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

      /* If change is coming from VOICE, no need to notify VOICE */
      if ( myEid != EID_VOICE )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
      }
      /* Send message to voice to update routing table */
      sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_ROUTING);
   }
   return CMSRET_SUCCESS;
}


CmsRet rcl_voiceProfSipSubscribeObject( _VoiceProfSipSubscribeObject *newObj ,
                const _VoiceProfSipSubscribeObject *currObj ,
                const InstanceIdStack *iidStack ,
                char **errorParam ,
                CmsRet *errorCode )
{
   return CMSRET_SUCCESS;
}

#if 0
CmsRet rcl_voiceProfSipResponseObject( _VoiceProfSipResponseObject *newObj __attribute__((unused)),
                const _VoiceProfSipResponseObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
CmsRet rcl_voiceProfMgcpObject( _VoiceProfMgcpObject *newObj __attribute__((unused)),
                const _VoiceProfMgcpObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#if 0
CmsRet rcl_voiceProfH323Object( _VoiceProfH323Object *newObj __attribute__((unused)),
                const _VoiceProfH323Object *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif

CmsRet rcl_voiceProfRtpObject( _VoiceProfRtpObject *newObj,
                const _VoiceProfRtpObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      void *msgHandle = cmsMdm_getThreadMsgHandle();
      CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

      /* If change is coming from VOICE, no need to notify VOICE */
      if ( myEid != EID_VOICE )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
      }
   }
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceProfRtpRtcpObject( _VoiceProfRtpRtcpObject *newObj __attribute__((unused)),
                const _VoiceProfRtpRtcpObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceProfRtpSrtpObject( _VoiceProfRtpSrtpObject *newObj __attribute__((unused)),
                const _VoiceProfRtpSrtpObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceProfRtpRedundancyObject( _VoiceProfRtpRedundancyObject *newObj __attribute__((unused)),
                const _VoiceProfRtpRedundancyObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#if 0
CmsRet rcl_voiceProfLineNumberingPlanObject( _VoiceProfLineNumberingPlanObject *newObj __attribute__((unused)),
                const _VoiceProfLineNumberingPlanObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceProfNumberingPlanPrefixInfoObject( _VoiceProfNumberingPlanPrefixInfoObject *newObj __attribute__((unused)),
                const _VoiceProfNumberingPlanPrefixInfoObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceProfToneObject( _VoiceProfToneObject *newObj __attribute__((unused)),
                const _VoiceProfToneObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceProfToneEventObject( _VoiceProfToneEventObject *newObj __attribute__((unused)),
                const _VoiceProfToneEventObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceProfToneDescriptionObject( _VoiceProfToneDescriptionObject *newObj __attribute__((unused)),
                const _VoiceProfToneDescriptionObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceProfTonePatternObject( _VoiceProfTonePatternObject *newObj __attribute__((unused)),
                const _VoiceProfTonePatternObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceProfButtonMapObject( _VoiceProfButtonMapObject *newObj __attribute__((unused)),
                const _VoiceProfButtonMapObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceProfButtonMapButtonObject( _VoiceProfButtonMapButtonObject *newObj __attribute__((unused)),
                const _VoiceProfButtonMapButtonObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif

CmsRet rcl_voiceProfFaxT38Object( _VoiceProfFaxT38Object *newObj,
                const _VoiceProfFaxT38Object *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

   if ((newObj != NULL && currObj == NULL)) /* new instance creation */
   {
      /* Create line objects */
      CmsRet localRet;
      InstanceIdStack localIidStack = EMPTY_INSTANCE_ID_STACK;
      void *lineObj = NULL;

      /*
       * Figure out if there are Line objects under me.  If not, then create the
       * standard set of Line objects.
       */
      localRet = cmsObj_getNextInSubTreeFlags( MDMOID_VOICE_LINE,
                                               iidStack,
                                               &localIidStack,
                                               OGF_NO_VALUE_UPDATE,
                                               &lineObj );

      if ( localRet == CMSRET_SUCCESS )
      {
         /*
          * If I was able to get a Line object under me, then there is no
          * need to create Line object.  This happens during startup time.
          */
         cmsLog_debug("Line objects already exist");
         cmsObj_free((void **) &lineObj);
      }
      else
      {
         localIidStack = *iidStack;
         localRet = cmsObj_addInstance( MDMOID_VOICE_LINE, &localIidStack );

         if (localRet != CMSRET_SUCCESS)
         {
            cmsLog_error("addObjectInstance: Failed, ret=%d", localRet);
            return localRet;
         }
      }

      /* If change is coming from VOICE, no need to notify VOICE */
      if ( myEid != EID_VOICE )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
      }
   }
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
      if ( myEid != EID_VOICE )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
      }
   }
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineObject( _VoiceLineObject *newObj,
                const _VoiceLineObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

   if ((newObj != NULL && currObj == NULL)) /* new instance creation */
   {
      CmsRet ret;
      VoiceProfObject *vpObj;
      InstanceIdStack ancestorIidStack = *iidStack;

      rutVoice_assignCMAcnt();

      /* Update VoiceProfile.i.NumberOfLines field */
      /* Get ancestor object */
      if ( (ret = cmsObj_getAncestor( MDMOID_VOICE_PROF, MDMOID_VOICE_LINE, &ancestorIidStack, (void **) &vpObj )) != CMSRET_SUCCESS )
      {
         cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
         return ret;
      }
      else
      {
         vpObj->numberOfLines += 1;

         /* Set VoiceProfile.i.NumberofLines field */
         if ((ret = cmsObj_set( vpObj, &ancestorIidStack )) != CMSRET_SUCCESS )
         {
            cmsLog_error("Unable to set value to mdm, ret = %d\n", ret);
            cmsObj_free((void **) &vpObj);
            return ret;
         }

         cmsObj_free((void **) &vpObj);
      }

      /* If change is coming from VOICE, no need to notify VOICE */
      if ( myEid != EID_VOICE )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
      }
   }
   else if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
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
#ifdef SUPPORT_BAS2
           sendMsgToBasClientVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_REGSTATUS);
#endif
      }
   }
   else if ((newObj == NULL && currObj != NULL )) /* delete */
   {
      int line, vp;
      VoiceProfObject *vpObj = NULL;
      CmsRet ret;

      /* Retrieve the line instance to delete */
      line = INSTANCE_ID_AT_DEPTH(iidStack, 2);

      /* Retrieve the voice profile instance that is the parent of the line instance to delete */
      InstanceIdStack ancestorIidStack = *iidStack;

      /* Get ancestor object */
      if ( (ret = cmsObj_getAncestor( MDMOID_VOICE_PROF, MDMOID_VOICE_LINE, &ancestorIidStack, (void **) &vpObj )) != CMSRET_SUCCESS )
      {
         cmsLog_error("Unable to get ancestor object");
         return ret;
      }
      else
      {
         vp = INSTANCE_ID_AT_DEPTH(&ancestorIidStack, 1);

         vpObj->numberOfLines -= 1;

         /* Set VoiceProfile.i.NumberofLines field */
         if ((ret = cmsObj_set( vpObj, &ancestorIidStack )) != CMSRET_SUCCESS )
         {
            cmsLog_error("Unable to set value to mdm, ret = %d\n", ret);
            cmsObj_free((void **) &vpObj);
            return ret;
         }

         rutVoice_updateCMAcntNum( vp, line );

         if ( myEid != EID_VOICE )
         {
            sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
         }

         cmsObj_free((void **) &vpObj);
      }
      /* Send message to voice to update routing table */
      sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_ROUTING);
   }

   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineSipObject( _VoiceLineSipObject *newObj,
                const _VoiceLineSipObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      void *msgHandle = cmsMdm_getThreadMsgHandle();
      CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

      /* If change is coming from VOICE, no need to notify VOICE */
      if ( myEid != EID_VOICE )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
      }
      /* Send message to voice to update routing table */
      sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_ROUTING);
   }
   return CMSRET_SUCCESS;
}

#if 0
CmsRet rcl_voiceLineSipEventSubscribeObject( _VoiceLineSipEventSubscribeObject *newObj __attribute__((unused)),
                const _VoiceLineSipEventSubscribeObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceLineMgcpObject( _VoiceLineMgcpObject *newObj,
                const _VoiceLineMgcpObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
      }
   }
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceLineH323Object( _VoiceLineH323Object *newObj __attribute__((unused)),
                const _VoiceLineH323Object *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceLineRingerObject( _VoiceLineRingerObject *newObj __attribute__((unused)),
                const _VoiceLineRingerObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceLineRingerEventObject( _VoiceLineRingerEventObject *newObj __attribute__((unused)),
                const _VoiceLineRingerEventObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceLineRingerDescriptionObject( _VoiceLineRingerDescriptionObject *newObj __attribute__((unused)),
                const _VoiceLineRingerDescriptionObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceLineRingerPatternObject( _VoiceLineRingerPatternObject *newObj __attribute__((unused)),
                const _VoiceLineRingerPatternObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif

CmsRet rcl_voiceLineCallingFeaturesObject( _VoiceLineCallingFeaturesObject *newObj,
                const _VoiceLineCallingFeaturesObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{

   if ((newObj != NULL && currObj == NULL))      /* new instance creation */
   {
      int maxSess;

      rutVoice_getMaxSessPerLine( &maxSess );

      /* Set the maximum conferencing sessions supported by the endpoint */
      newObj->maxSessions = maxSess;
   }
   else if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      void *msgHandle = cmsMdm_getThreadMsgHandle();
      CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

      /* If change is coming from VOICE, no need to notify VOICE */
      if ( myEid != EID_VOICE )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
      }
   }
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineProcessingObject( _VoiceLineProcessingObject *newObj __attribute__((unused)),
                const _VoiceLineProcessingObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      void *msgHandle = cmsMdm_getThreadMsgHandle();
      CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

      /* If change is coming from VOICE, no need to notify VOICE */
      if ( myEid != EID_VOICE )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
      }
   }
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineCodecObject( _VoiceLineCodecObject *newObj __attribute__((unused)),
                const _VoiceLineCodecObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineCodecListObject( _VoiceLineCodecListObject *newObj,
                const _VoiceLineCodecListObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj == NULL)) /* new instance creation */
   {
      /* Do nothing */
   }
   else if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      void *msgHandle = cmsMdm_getThreadMsgHandle();
      CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

      /* If change is coming from VOICE, no need to notify VOICE */
      if ( myEid != EID_VOICE )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
      }
   }
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineStatsObject( _VoiceLineStatsObject *newObj ,
                const _VoiceLineStatsObject *currObj ,
                const InstanceIdStack *iidStack ,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   void *msgHandle = cmsMdm_getThreadMsgHandle();

   /* If resetStatistics is true, reset this stats object */
   if ( newObj != NULL && currObj != NULL && newObj->resetStatistics )
   {
      CmsMsgHeader msg = EMPTY_MSG_HEADER;
      CmsRet ret;
      MdmObjectId tempId;
      UINT16 tempSeq;
      UINT32 lineInst;

      /* Save MDM Object ID and sequence number before memset */
      tempId = newObj->_oid;
      tempSeq = newObj->_sequenceNum;

      /* Reset */
      memset( newObj, 0, sizeof(_VoiceLineStatsObject) );
      newObj->_oid = tempId;
      newObj->_sequenceNum = tempSeq;

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
   else if ((newObj != NULL && currObj == NULL)) /* new instance creation */
   {
      /* Do nothing */
   }

   return CMSRET_SUCCESS;
}

CmsRet rcl_voicePhyIntfObject( _VoicePhyIntfObject *newObj ,
                const _VoicePhyIntfObject *currObj ,
                const InstanceIdStack *iidStack ,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj == NULL)) /* new instance creation */
   {
      char description[BUFLEN_32];
      char phyPort[BUFLEN_16];
      int iiD = PEEK_INSTANCE_ID(iidStack);
      enum dh_type type = DSPHAL_TYPE_NONE;

      newObj->interfaceID = iiD - 1;
      sprintf( phyPort,"%01d",(iiD-1) );

      CMSMEM_REPLACE_STRING_FLAGS(newObj->phyPort,phyPort,mdmLibCtx.allocFlags);

      if( rutVoice_getPhysEndptType( (iiD-1), (int *)&type ) != CMSRET_SUCCESS )
      {
          type = DSPHAL_TYPE_FXS;
      }

      memset(description, 0, sizeof(description));
      if( type == DSPHAL_TYPE_FXS ){
         sprintf( description,"FXS%d",(iiD-1) );
      }
      else if( type == DSPHAL_TYPE_FXO ){
         sprintf( description,"FXO");
      }
      else if( type == DSPHAL_TYPE_DECT ){
         sprintf( description,"DECT");
      }
      else if( type == DSPHAL_TYPE_ALSA ){
         sprintf( description,"NOSIG");     /* description type NOSIG is used for DSPHAL_TYPE_ALSA */ 
      }
      else{
         sprintf( description,"NOSIG");
      }

      CMSMEM_REPLACE_STRING_FLAGS(newObj->description,description,mdmLibCtx.allocFlags);

      return CMSRET_SUCCESS;
   }
   else if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      void *msgHandle = cmsMdm_getThreadMsgHandle();
      CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

      /* If change is coming from VOICE, no need to notify VOICE */
      if ( myEid != EID_VOICE )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
      }
   }
   /* Send message to voice to update routing table */
   sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_ROUTING);

   return ( CMSRET_SUCCESS );
}

CmsRet rcl_voicePhyIntfTestsObject( _VoicePhyIntfTestsObject *newObj,
                const _VoicePhyIntfTestsObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      void *msgHandle = cmsMdm_getThreadMsgHandle();
      CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

      /* If change is coming from VOICE, no need to notify VOICE */
      if ( myEid != EID_VOICE )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
      }
   }
   /* Send message to voice to update routing table */
   sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_ROUTING);
   return CMSRET_SUCCESS;
}

CmsRet rcl_voicePstnObject( _VoicePstnObject *newObj,
                const _VoicePstnObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      void *msgHandle = cmsMdm_getThreadMsgHandle();
      CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

      /* If change is coming from VOICE, no need to notify VOICE */
      if ( myEid != EID_VOICE )
      {
         sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_GENERAL);
      }
      /* Send message to voice to update routing table */
      sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_ROUTING);
   }
   return CMSRET_SUCCESS;
}

#if defined( DMP_X_BROADCOM_COM_NTR_1 )
CmsRet rcl_voiceNtrObject( _VoiceNtrObject *newObj,
                const _VoiceNtrObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
   }
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceNtrHistoryObject(_VoiceNtrHistoryObject *newObj,
                const _VoiceNtrHistoryObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
   }
   return CMSRET_SUCCESS;
}

#endif /* DMP_X_BROADCOM_COM_NTR_1 */


#ifdef DMP_X_BROADCOM_COM_DECTENDPOINT_1

CmsRet rcl_voiceDectSystemSettingObject( _VoiceDectSystemSettingObject *newObj,
                                const _VoiceDectSystemSettingObject *currObj,
                                const InstanceIdStack *iidStack,
                                char **errorParam,
                                CmsRet *errorCode)
{
   /* Nothing needs to be done since a change to the DECT interface object
   ** MDM data would have been triggered by the VOICE application applying
   ** its own processing - any other source of the change would be erroneous.
   */
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceDECTHandsetObject( _VoiceDECTHandsetObject *newObj,
                                   const _VoiceDECTHandsetObject *currObj,
                                   const InstanceIdStack *iidStack,
                                   char **errorParam,
                                   CmsRet *errorCode)
{
   int i, j, handsetId, maxVp, maxLines, serviceId, vpInst;
   InstanceIdStack vpIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack lineIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack localIidStack = EMPTY_INSTANCE_ID_STACK;
   void   *obj;
   CmsRet ret;
   char   name[20];
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

   if ( newObj != NULL &&  currObj == NULL ) /* Creation of object instance */
   {
      if(myEid == EID_DECT) /* Only reset the object's parameters when DECT is the one adding it. */
      {
         newObj->X_BROADCOM_COM_MANIC = 0;
         newObj->X_BROADCOM_COM_IPUI = 0;
         newObj->X_BROADCOM_COM_MODIC = 0;
         newObj->X_BROADCOM_COM_ID = 0;

         time_t calendar_time;
         struct tm *local_time = NULL;
         char date_string[32];

         /* Get the local time information */
         calendar_time = time(NULL);
         if (calendar_time != -1)
         {
            local_time = localtime (&calendar_time);
         }

         if ( local_time )
         {
            strftime(date_string, sizeof(date_string), "%Y-%m-%dT%H:%M:%S", local_time);
         }
         else
         {
            snprintf(date_string, sizeof(date_string), "2010-01-01T00:00:00Z");
         }

         CMSMEM_REPLACE_STRING_FLAGS( newObj->subscriptionTime, date_string, mdmLibCtx.allocFlags );
         CMSMEM_REPLACE_STRING_FLAGS( newObj->status, "unreachable", mdmLibCtx.allocFlags );
         CMSMEM_REPLACE_STRING_FLAGS( newObj->X_BROADCOM_COM_Call_Interception, MDMVS_YES, mdmLibCtx.allocFlags );
      }
   }
   else if( newObj != NULL && currObj != NULL ) /* Modification of an object instance */
   {
      /* If the current handset id is unassigned, or if we are making a
       * modification to the handset id, then make sure to add the handset to
       * the attached handset list for every line.
       */
      if( currObj->X_BROADCOM_COM_ID == 0 && currObj->X_BROADCOM_COM_ID != newObj->X_BROADCOM_COM_ID )
      {
         handsetId = newObj->X_BROADCOM_COM_ID;
         handsetId = (handsetId == 0 || handsetId >= 32 ) ? 0 : (handsetId - 1);

         /* create internal name for this handset if it doesn't exist */
         if( newObj->X_BROADCOM_COM_Name == NULL )
         {
            sprintf(name, "handset %d", handsetId + 1);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_Name, name, mdmLibCtx.allocFlags);
         }

         /* get current voice service instance (should be 1) */
         serviceId = INSTANCE_ID_AT_DEPTH( iidStack, 0 );
         if(CMSRET_SUCCESS != rutVoice_getNumSrvProv( &maxVp ) || maxVp <= 0)
         {
            return CMSRET_INTERNAL_ERROR;
         }
         PUSH_INSTANCE_ID( &vpIidStack, serviceId );

         /* Get the voice profile for each service provider */
         for(i = 0; i < maxVp; i++)
         {
            rutVoice_mapSpNumToVpInst( i, &vpInst );
            PUSH_INSTANCE_ID( &vpIidStack, vpInst );
            ret = cmsObj_get( MDMOID_VOICE_PROF, &vpIidStack, OGF_NO_VALUE_UPDATE, &obj );

            if( ret != CMSRET_SUCCESS )
            {
               return CMSRET_INTERNAL_ERROR;
            }

            if( ((_VoiceProfObject *) obj)->numberOfLines <= 0 )
            {
               cmsObj_free(&obj);
               cmsLog_debug("Line instance not exist in voice profile %d\n", i);
               return CMSRET_INTERNAL_ERROR;
            }
            else
            {
               maxLines = ((_VoiceProfObject *) obj)->numberOfLines;
            }
            cmsObj_free(&obj);

            /* check line is already attached with dect handset */
            if ( rutIsLineAttachedWithDectHS( handsetId ) )
            {
               break;
            }

            /* Get the voice line for each account in the voice profile */
            for(j = 0; j < maxLines; j++)
            {
               if(CMSRET_SUCCESS != (ret = cmsObj_getNextInSubTreeFlags(MDMOID_VOICE_LINE, &vpIidStack, &lineIidStack, OGF_NO_VALUE_UPDATE, &obj)))
               {
                  cmsLog_error("Can't retrieve Line instance (ret = %d)", ret);
                  return CMSRET_INTERNAL_ERROR;
               }
               cmsObj_free(&obj);

               /* Get attached handset list and add the new handset to the list */
               if(CMSRET_SUCCESS != cmsObj_getNextInSubTree(MDMOID_VOICE_LINE_ATTACHED_HANDSET, &lineIidStack, &localIidStack, &obj))
               {
                  cmsLog_error("Can't retrieve attached handset list");
                  return CMSRET_INTERNAL_ERROR;
               }
               else
               {
                  ((_VoiceLineAttachedHandsetObject *)obj)->element |=  ( 1 << handsetId );
                  ((_VoiceLineAttachedHandsetObject *)obj)->totalNumber++;
                  cmsObj_set(obj, &localIidStack);
                  cmsObj_free(&obj);
               }
            }
         }

         /* Send Message to Voice Indicating that registered DECT handset list has changed */
         CmsMsgHeader msg = EMPTY_MSG_HEADER;
         msg.type = CMS_MSG_VOICE_DECT_REGHSETLIST_UPDATE;
         msg.src = cmsMsg_getHandleEid(msgHandle);
         msg.dst = EID_VOICE;
         msg.flags_event = 1;
         msg.flags_bounceIfNotRunning = 1;
         msg.wordData = newObj->X_BROADCOM_COM_ID;

         if ((ret = cmsMsg_send(msgHandle, &msg)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not send reset statistics message msg to Call Manager, ret=%d", ret);
         }
      }
      else if( newObj->X_BROADCOM_COM_Delete )
      {
         /* Send Message to Voice Indicating that de-registered DECT handset list has changed */
         CmsMsgHeader msg = EMPTY_MSG_HEADER;

         msg.type = CMS_MSG_VOICE_DECT_HS_DELETE;
         msg.src = EID_CONSOLED;
         msg.dst = EID_DECT;
         msg.flags_event = 1;
         msg.flags_bounceIfNotRunning = 1;
         msg.wordData = newObj->X_BROADCOM_COM_ID;

         if ((ret = cmsMsg_send(msgHandle, &msg)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not send reset statistics message msg to Call Manager, ret=%d", ret);
         }

         newObj->X_BROADCOM_COM_Delete = 0;
      }
   }
   else if( newObj == NULL && currObj != NULL ) /* deletion of an object instance */
   {
      if( currObj->X_BROADCOM_COM_ID != 0 )
      {
         handsetId = currObj->X_BROADCOM_COM_ID;
         handsetId = (handsetId == 0 || handsetId >= 32 ) ? 0 : (handsetId - 1);
         /* get current voice service instance, should be 1 */
         serviceId = INSTANCE_ID_AT_DEPTH(iidStack, 0);
         if(CMSRET_SUCCESS != rutVoice_getNumSrvProv( &maxVp ) || maxVp <= 0)
         {
            return CMSRET_INTERNAL_ERROR;
         }
         PUSH_INSTANCE_ID(&vpIidStack, serviceId );

         for(i = 0; i < maxVp; i++)
         {
            /* get voice profile for each service provider */
            rutVoice_mapSpNumToVpInst( i, &vpInst );
            PUSH_INSTANCE_ID(&vpIidStack, vpInst );
            ret = cmsObj_get(MDMOID_VOICE_PROF, &vpIidStack, OGF_NO_VALUE_UPDATE, &obj);

            if( ret == CMSRET_SUCCESS )
            {
               if( ((_VoiceProfObject *) obj)->numberOfLines <= 0 )
               {
                  cmsObj_free(&obj);
                  cmsLog_debug("Line instance not exist in voice profile %d\n", i);
                  return CMSRET_INTERNAL_ERROR;
               }
               else
               {
                  maxLines = ((_VoiceProfObject *) obj)->numberOfLines;
               }
               cmsObj_free(&obj);
            }
            else
            {
               return CMSRET_INTERNAL_ERROR;
            }

            /* get line profile for each account in voice profile */
            for(j = 0; j < maxLines; j++)
            {
               if(CMSRET_SUCCESS != (ret = cmsObj_getNextInSubTreeFlags(MDMOID_VOICE_LINE, &vpIidStack, &lineIidStack, OGF_NO_VALUE_UPDATE, &obj)))
               {
                  cmsLog_debug("Can't retrieve Line instance ret = %d", ret);
                  return CMSRET_INTERNAL_ERROR;
               }
               cmsObj_free(&obj);

               /* get attached handset list */
               if(CMSRET_SUCCESS != cmsObj_getNextInSubTree(MDMOID_VOICE_LINE_ATTACHED_HANDSET, &lineIidStack, &localIidStack, &obj))
               {
                  cmsLog_debug("Can't retrieve attached handset list \n");
                  return CMSRET_INTERNAL_ERROR;
               }
               else
               {
                  if(((_VoiceLineAttachedHandsetObject *)obj)->element & ( 1 << handsetId ))
                  {
                     ((_VoiceLineAttachedHandsetObject *)obj)->element &=  ~( 1 << handsetId );
                     if(((_VoiceLineAttachedHandsetObject *)obj)->totalNumber > 0 )
                     {
                        ((_VoiceLineAttachedHandsetObject *)obj)->totalNumber--;
                     }

                     cmsObj_set(obj, &localIidStack);
                  }
                  cmsObj_free(&obj);
               }
            }
         }

         /* Send Message to Voice Indicating that registered DECT handset list has changed */
         CmsMsgHeader msg = EMPTY_MSG_HEADER;
         msg.type = CMS_MSG_VOICE_DECT_REGHSETLIST_UPDATE;
         msg.src = cmsMsg_getHandleEid(msgHandle);
         msg.dst = EID_VOICE;
         msg.flags_event = 1;
         msg.flags_bounceIfNotRunning = 1;
         msg.wordData = currObj->X_BROADCOM_COM_ID;

         if ((ret = cmsMsg_send(msgHandle, &msg)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not send reset statistics message msg to Call Manager, ret=%d", ret);
         }
      }
   }
   /* Send message to voice to update routing table */
   sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_ROUTING);
   rutVoice_dectListUpdate( DECT_DELAY_LIST_INTERNAL_NAMES, 0 );

   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceDectPinCodeObject( _VoiceDectPinCodeObject *newObj,
                const _VoiceDectPinCodeObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceDectClockMasterObject( _VoiceDectClockMasterObject *newObj,
                const _VoiceDectClockMasterObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceDectResetBaseObject( _VoiceDectResetBaseObject *newObj,
                const _VoiceDectResetBaseObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   int     maxHset, i;
   void   *obj;
   CmsRet  ret;
   InstanceIdStack ancestorIidStack = *iidStack;
   InstanceIdStack localIidStack    = EMPTY_INSTANCE_ID_STACK;

   cmsLog_debug("rcl function  \n");
   if ((newObj != NULL && currObj != NULL) && (newObj->element > 0)) /* modify */
   {

      if(cmsObj_getAncestorFlags( MDMOID_VOICE_DECT_SYSTEM_SETTING, MDMOID_VOICE_DECT_RESET_BASE, &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &obj ) == CMSRET_SUCCESS )
      {
         cmsLog_debug("rcl function  2\n");
         /* clear system setting */
         CMSMEM_FREE_BUF_AND_NULL_PTR(((_VoiceDectSystemSettingObject *)obj)->X_BROADCOM_COM_LinkDate);
         ((_VoiceDectSystemSettingObject *)obj)->X_BROADCOM_COM_Type = 0;
         ((_VoiceDectSystemSettingObject *)obj)->X_BROADCOM_COM_DectId = 0;
         ((_VoiceDectSystemSettingObject *)obj)->X_BROADCOM_COM_MANIC = 0;
         ((_VoiceDectSystemSettingObject *)obj)->X_BROADCOM_COM_MODIC = 0;
         ((_VoiceDectSystemSettingObject *)obj)->X_BROADCOM_COM_MaxNumberOfHandsets = 0; /**< X_BROADCOM_COM_MaxNumberOfHandsets */
         ((_VoiceDectSystemSettingObject *)obj)->X_BROADCOM_COM_ServiceEnabled = 1; /**< X_BROADCOM_COM_ServiceEnabled */

         cmsObj_set(&obj, &ancestorIidStack);
         cmsObj_free( &obj );

         cmsLog_debug("rcl function 3 \n");
         /* clear register handset */
         rutVoice_getMaxDectHset( &maxHset );
         cmsLog_debug("rcl function 4 \n");
         for(i = 0; i < maxHset; i++)
         {
            INIT_INSTANCE_ID_STACK(&localIidStack);
            ret = cmsObj_getNextInSubTree(MDMOID_DECT_HANDSET, &ancestorIidStack, &localIidStack, &obj);
            if(ret == CMSRET_SUCCESS )
            {
               cmsObj_free( &obj );
               cmsObj_deleteInstance(MDMOID_DECT_HANDSET, &localIidStack);
            }
         }
         /* keep original value, should be 0 */
         newObj->element = currObj->element;
         cmsLog_debug("rcl function 5 \n");
      }
   }
   /* Send message to voice to update routing table */
   sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_ROUTING);
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceDectFirmwareVersionObject( _VoiceDectFirmwareVersionObject *newObj,
                const _VoiceDectFirmwareVersionObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceDectHardwareVersionObject( _VoiceDectHardwareVersionObject *newObj,
                const _VoiceDectHardwareVersionObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceDectEEPROMVersionObject( _VoiceDectEEPROMVersionObject *newObj,
                const _VoiceDectEEPROMVersionObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_voiceLineSettingObject( _VoiceLineSettingObject *newObj,
                const _VoiceLineSettingObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineNameObject( _VoiceLineNameObject *newObj,
                const _VoiceLineNameObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   if( newObj != NULL && currObj == NULL) /* newly created object */
   {
      if(newObj->element == NULL)
      {
         InstanceIdStack ancestorIidStack = *iidStack;
         char name[20];
         unsigned int lineId = 0;

         /* Get the line number */
         lineId = INSTANCE_ID_AT_DEPTH(&ancestorIidStack, 2);
         snprintf( name, 20, "Line %u", lineId );

         CMSMEM_REPLACE_STRING_FLAGS( newObj->element, name, mdmLibCtx.allocFlags );
      }
   }
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineDectLineIdObject( _VoiceLineDectLineIdObject *newObj,
                const _VoiceLineDectLineIdObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   /* Send message to voice to update routing table */
   sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_ROUTING);
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineAttachedHandsetObject( _VoiceLineAttachedHandsetObject *newObj,
                const _VoiceLineAttachedHandsetObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   /* Send message to voice to update routing table */
   sendMsgToVoice(CMS_MSG_VOICE_CONFIG_CHANGED, CMSMSGVOICECONFIGCHANGED_ROUTING);
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineMelodyObject( _VoiceLineMelodyObject *newObj,
                const _VoiceLineMelodyObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineDialingPrefixObject( _VoiceLineDialingPrefixObject *newObj,
                const _VoiceLineDialingPrefixObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineVolumnObject( _VoiceLineVolumnObject *newObj,
                const _VoiceLineVolumnObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineBlockedNumberObject( _VoiceLineBlockedNumberObject *newObj,
                const _VoiceLineBlockedNumberObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineMultiCallModeObject( _VoiceLineMultiCallModeObject *newObj,
                const _VoiceLineMultiCallModeObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineIntrusionCallObject( _VoiceLineIntrusionCallObject *newObj,
                const _VoiceLineIntrusionCallObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineCLIRObject( _VoiceLineCLIRObject *newObj,
                const _VoiceLineCLIRObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceDectSupportListObject( _VoiceDectSupportListObject *newObj,
                const _VoiceDectSupportListObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceContactNameObject( _VoiceContactNameObject *newObj,
                const _VoiceContactNameObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceContactFirstNameObject( _VoiceContactFirstNameObject *newObj,
                const _VoiceContactFirstNameObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceContactLineIdObject( _VoiceContactLineIdObject *newObj,
                const _VoiceContactLineIdObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_voiceContactListObject( _VoiceContactListObject *newObj,
                const _VoiceContactListObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   if ( newObj != NULL &&  currObj == NULL ) /* Creation of object instance */
   {
      int  i;
      CmsRet localRet;

      /* added 3 contactNumber instances */
      for(i = 1; i <= 3; i ++)
      {
         InstanceIdStack localIidStack = *iidStack;

         PUSH_INSTANCE_ID( &localIidStack, i );
         localRet = cmsObj_addInstance(MDMOID_VOICE_CONTACT_NUMBER, &localIidStack);
         if (localRet != CMSRET_SUCCESS)
         {
            cmsLog_error("addObjectInstance: Failed, ret=%d", localRet);
            return localRet;
         }
      } 
   }
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceContactMelodyObject( _VoiceContactMelodyObject *newObj,
                const _VoiceContactMelodyObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceContactNumberObject( _VoiceContactNumberObject *newObj,
                const _VoiceContactNumberObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallTypeObject( _VoiceCallTypeObject *newObj,
                const _VoiceCallTypeObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallNameObject( _VoiceCallNameObject *newObj,
                const _VoiceCallNameObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallNumberObject( _VoiceCallNumberObject *newObj,
                const _VoiceCallNumberObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallDateTimeObject( _VoiceCallDateTimeObject *newObj,
                const _VoiceCallDateTimeObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallLineNameObject( _VoiceCallLineNameObject *newObj,
                const _VoiceCallLineNameObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallLineIdObject( _VoiceCallLineIdObject *newObj,
                const _VoiceCallLineIdObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallNumberOfMissedCallsObject( _VoiceCallNumberOfMissedCallsObject *newObj,
                const _VoiceCallNumberOfMissedCallsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallNewFlagObject( _VoiceCallNewFlagObject *newObj,
                const _VoiceCallNewFlagObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallListObject( _VoiceCallListObject *newObj,
                const _VoiceCallListObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

#endif /* DMP_X_BROADCOM_COM_DECTENDPOINT_1 */

#endif /* BRCM_VOICE_SUPPORT */
